#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <immintrin.h>
#include <iostream>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../../layer1-core/block/block.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/pow/difficulty.h"
#include "../stratum.h"

namespace {

std::string ToHex(const uint256& h)
{
    std::ostringstream oss;
    for (auto b : h)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

uint256 FromHex(const std::string& hex)
{
    if (hex.size() % 2 != 0)
        throw std::invalid_argument("expected even-length hex string");
    uint256 out{};
    size_t bytes = hex.size() / 2;
    if (bytes > out.size())
        throw std::invalid_argument("hex string too long for uint256");
    size_t offset = out.size() - bytes;
    for (size_t i = 0; i < bytes; ++i) {
        auto byte = std::stoul(hex.substr(i * 2, 2), nullptr, 16);
        out[offset + i] = static_cast<uint8_t>(byte);
    }
    return out;
}

uint32_t ParseLE32(const std::string& hex, size_t offset)
{
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        auto byte = static_cast<uint32_t>(std::stoul(hex.substr((offset + i) * 2, 2), nullptr, 16));
        out |= byte << (8 * i);
    }
    return out;
}

boost::multiprecision::cpp_int ToInteger(const uint256& h)
{
    boost::multiprecision::cpp_int result = 0;
    for (uint8_t byte : h) {
        result <<= 8;
        result |= byte;
    }
    return result;
}

bool MeetsExplicitTarget(const uint256& hash, const uint256& target)
{
    if (ToInteger(target) == 0)
        return false;
    return ToInteger(hash) <= ToInteger(target);
}

struct MinerConfig {
    std::string stratumUrl;
    std::string user;
    std::string pass;
    uint32_t threads{1};
    bool benchmark{false};
    int benchmarkSeconds{10};
    bool allowRemote{false};
    uint32_t minTargetBits{0};
    std::string configPath;
    uint32_t intensity{0};
    std::string worker;
    bool preferStratumV2{false};
    std::string rpcAuthToken;
};

uint32_t ClampBits(uint32_t bits, uint32_t minBits)
{
    if (minBits == 0)
        return bits;
    using boost::multiprecision::cpp_int;
    auto CompactToTarget = [](uint32_t nBits) {
        uint32_t exponent = nBits >> 24;
        uint32_t mantissa = nBits & 0x007fffff;
        cpp_int target = mantissa;
        if (exponent <= 3) target >>= 8 * (3 - exponent); else target <<= 8 * (exponent - 3);
        return target;
    };

    auto target = CompactToTarget(bits);
    auto minTarget = CompactToTarget(minBits);
    if (minTarget != 0 && target > minTarget)
        return minBits;
    return bits;
}

inline uint32_t RandomizeNonceSeed()
{
    std::random_device rd;
    std::array<uint32_t, 4> entropy{};
    for (auto& e : entropy) e = rd();
    return std::accumulate(entropy.begin(), entropy.end(), 0u);
}

struct MidstateWorkspace {
    uint32_t firstBlockState[8]{};
    uint32_t tailWords[16]{};
};

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t big0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t big1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t sm0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t sm1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

constexpr uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

inline void Compress(uint32_t state[8], uint32_t w[64])
{
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
    for (int i=0;i<64;++i) {
        uint32_t t1 = h + big1(e) + ch(e,f,g) + K[i] + w[i];
        uint32_t t2 = big0(a) + maj(a,b,c);
        h=g; g=f; f=e; e=d + t1; d=c; c=b; b=a; a=t1 + t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

MidstateWorkspace BuildWorkspace(const BlockHeader& header)
{
    MidstateWorkspace ws{};
    uint32_t w0[64] = {0};
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&header);
    for (int i = 0; i < 16; ++i) {
        w0[i] = (static_cast<uint32_t>(data[i*4]) << 24) | (static_cast<uint32_t>(data[i*4+1]) << 16) |
                (static_cast<uint32_t>(data[i*4+2]) << 8) | (static_cast<uint32_t>(data[i*4+3]));
    }
    for (int i = 16; i < 64; ++i)
        w0[i] = sm1(w0[i-2]) + w0[i-7] + sm0(w0[i-15]) + w0[i-16];

    uint32_t state[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    Compress(state, w0);

    for (int i = 0; i < 8; ++i)
        ws.firstBlockState[i] = state[i];

    ws.tailWords[0] = (static_cast<uint32_t>(data[64]) << 24) | (static_cast<uint32_t>(data[65]) << 16) |
                      (static_cast<uint32_t>(data[66]) << 8) | (static_cast<uint32_t>(data[67]));
    ws.tailWords[1] = (static_cast<uint32_t>(data[68]) << 24) | (static_cast<uint32_t>(data[69]) << 16) |
                      (static_cast<uint32_t>(data[70]) << 8) | (static_cast<uint32_t>(data[71]));
    ws.tailWords[2] = (static_cast<uint32_t>(data[72]) << 24) | (static_cast<uint32_t>(data[73]) << 16) |
                      (static_cast<uint32_t>(data[74]) << 8) | (static_cast<uint32_t>(data[75]));
    ws.tailWords[3] = (static_cast<uint32_t>(data[76]) << 24) | (static_cast<uint32_t>(data[77]) << 16) |
                      (static_cast<uint32_t>(data[78]) << 8) | (static_cast<uint32_t>(data[79]));
    ws.tailWords[4] = 0x80000000;
    for (int i = 5; i < 15; ++i) ws.tailWords[i] = 0;
    ws.tailWords[15] = 80 * 8;
    return ws;
}

inline void FillTail(uint32_t tail[64], const MidstateWorkspace& ws)
{
    for (int i = 0; i < 16; ++i)
        tail[i] = ws.tailWords[i];
    for (int i = 16; i < 64; ++i)
        tail[i] = sm1(tail[i-2]) + tail[i-7] + sm0(tail[i-15]) + tail[i-16];
}

inline void HashFromMidstate(const MidstateWorkspace& ws, uint32_t nonce, uint32_t outState[8])
{
    uint32_t tail[64];
    FillTail(tail, ws);
    tail[3] = __builtin_bswap32(nonce);
    uint32_t state[8];
    for (int i = 0; i < 8; ++i) state[i] = ws.firstBlockState[i];
    Compress(state, tail);

    uint32_t w2[64] = {0};
    for (int i=0;i<8;++i) w2[i] = state[i];
    w2[8] = 0x80000000;
    w2[15] = 32 * 8;
    for (int i=16;i<64;++i)
        w2[i] = sm1(w2[i-2]) + w2[i-7] + sm0(w2[i-15]) + w2[i-16];
    uint32_t state2[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    Compress(state2, w2);

    uint32_t w3[64] = {0};
    w3[8] = 0x80000000;
    w3[15] = 32 * 8;
    for (int i=16;i<64;++i)
        w3[i] = sm1(w3[i-2]) + w3[i-7] + sm0(w3[i-15]) + w3[i-16];
    Compress(state2, w3);
    for (int i=0;i<8;++i) outState[i] = state2[i];
}

MinerConfig ParseArgs(int argc, char** argv)
{
    MinerConfig cfg{};
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stratum-url" && i + 1 < argc) cfg.stratumUrl = argv[++i];
        else if (arg == "--stratum-user" && i + 1 < argc) cfg.user = argv[++i];
        else if (arg == "--stratum-pass" && i + 1 < argc) cfg.pass = argv[++i];
        else if (arg == "--threads" && i + 1 < argc) cfg.threads = std::stoul(argv[++i]);
        else if (arg == "--benchmark" || arg == "--bench") { cfg.benchmark = true; }
        else if (arg == "--benchmark-seconds" && i + 1 < argc) cfg.benchmarkSeconds = std::stoi(argv[++i]);
        else if (arg == "--allow-remote") cfg.allowRemote = true;
        else if (arg == "--min-target-bits" && i + 1 < argc) cfg.minTargetBits = std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--config" && i + 1 < argc) cfg.configPath = argv[++i];
        else if (arg == "--intensity" && i + 1 < argc) cfg.intensity = std::stoul(argv[++i]);
        else if (arg == "--worker" && i + 1 < argc) cfg.worker = argv[++i];
        else if (arg == "--stratum-v2") cfg.preferStratumV2 = true;
        else if (arg == "--rpc-auth-token" && i + 1 < argc) cfg.rpcAuthToken = argv[++i];
    }
    if (cfg.threads == 0) cfg.threads = 1;
    return cfg;
}

void LoadConfig(MinerConfig& cfg)
{
    if (cfg.configPath.empty())
        return;
    try {
        boost::property_tree::ptree tree;
        boost::property_tree::read_json(cfg.configPath, tree);
        cfg.stratumUrl = tree.get("stratum_url", cfg.stratumUrl);
        cfg.user = tree.get("user", cfg.user);
        cfg.pass = tree.get("pass", cfg.pass);
        cfg.worker = tree.get("worker", cfg.worker);
        cfg.threads = tree.get("threads", cfg.threads);
        cfg.allowRemote = tree.get("allow_remote", cfg.allowRemote);
        cfg.minTargetBits = tree.get("min_target_bits", cfg.minTargetBits);
        cfg.benchmarkSeconds = tree.get("benchmark_seconds", cfg.benchmarkSeconds);
        cfg.intensity = tree.get("intensity", cfg.intensity);
        cfg.benchmark = tree.get("benchmark", cfg.benchmark);
        cfg.preferStratumV2 = tree.get("stratum_v2", cfg.preferStratumV2);
        cfg.rpcAuthToken = tree.get("rpc_auth_token", cfg.rpcAuthToken);
    } catch (const std::exception& e) {
        std::cerr << "Failed to read config: " << e.what() << "\n";
    }
}

void RunBenchmark(const MinerConfig& cfg)
{
    BlockHeader header{};
    header.bits = consensus::Main().nGenesisBits;
    header.version = 1;
    header.time = static_cast<uint32_t>(std::time(nullptr));
    header.prevBlockHash = uint256{};
    header.merkleRoot = uint256{};

    std::atomic<uint64_t> hashes{0};
    std::atomic<bool> stop{false};
    auto worker = [&]() {
        BlockHeader local = header;
        MidstateWorkspace ws = BuildWorkspace(local);
        while (!stop.load(std::memory_order_relaxed)) {
            uint32_t state[8];
            HashFromMidstate(ws, local.nonce, state);
            ++hashes;
            ++local.nonce;
        }
    };

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < cfg.threads; ++i)
        threads.emplace_back(worker);

    std::this_thread::sleep_for(std::chrono::seconds(cfg.benchmarkSeconds));
    stop.store(true);
    for (auto& t : threads) t.join();

    double rate = static_cast<double>(hashes.load()) / cfg.benchmarkSeconds;
    std::cout << "Benchmark: " << rate / 1e6 << " MH/s across " << cfg.threads << " threads";
    if (cfg.intensity)
        std::cout << " (intensity " << cfg.intensity << ")";
    std::cout << "\n";
}

bool MineJob(const MinerJob& baseJob, const MinerConfig& cfg, StratumPool* pool)
{
    const auto& params = consensus::Main();
    std::atomic<bool> found{false};
    uint32_t nonceSeed = RandomizeNonceSeed() ^ baseJob.header.nonce;
    std::atomic<uint32_t> nonceCounter{nonceSeed};
    MidstateWorkspace ws = BuildWorkspace(baseJob.header);
    std::mutex submitMutex;

    uint32_t stride = cfg.intensity ? cfg.intensity : 1024;
#if defined(__AVX2__)
    stride *= 2; // Favor larger batches to keep vector units fed
#endif

    auto worker = [&](int idx) {
        MinerJob job = baseJob;
        while (!found.load(std::memory_order_relaxed)) {
            uint32_t startNonce = nonceCounter.fetch_add(stride, std::memory_order_relaxed);
            __builtin_prefetch(&startNonce, 0, 3);
#pragma GCC unroll 8
            for (uint32_t n = 0; n < stride && !found.load(std::memory_order_relaxed); ++n) {
                job.header.nonce = startNonce + n;
                uint32_t state[8];
                HashFromMidstate(ws, job.header.nonce, state);
                uint256 hash{};
                for (int i = 0; i < 8; ++i) {
                    hash[i*4] = static_cast<uint8_t>((state[i] >> 24) & 0xff);
                    hash[i*4+1] = static_cast<uint8_t>((state[i] >> 16) & 0xff);
                    hash[i*4+2] = static_cast<uint8_t>((state[i] >> 8) & 0xff);
                    hash[i*4+3] = static_cast<uint8_t>(state[i] & 0xff);
                }
                bool meets = (ToInteger(job.target) == 0)
                    ? powalgo::CheckProofOfWork(hash, ClampBits(job.header.bits, cfg.minTargetBits), params)
                    : MeetsExplicitTarget(hash, job.target);
                if (meets) {
                    found.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> g(submitMutex);
                    std::cout << "[thread " << idx << "] found nonce: " << job.header.nonce << "\n";
                    if (pool)
                        pool->SubmitResult(job, job.header.nonce);
                    return;
                }
            }
        }
    };

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < cfg.threads; ++i)
        threads.emplace_back(worker, static_cast<int>(i));
    for (auto& t : threads) t.join();
    return found.load();
}

} // namespace

int main(int argc, char** argv)
{
    MinerConfig cfg = ParseArgs(argc, argv);
    LoadConfig(cfg);
    if (cfg.benchmark) {
        RunBenchmark(cfg);
        return 0;
    }

    if (!cfg.stratumUrl.empty()) {
        StratumPool::Options opts{};
        opts.url = cfg.stratumUrl;
        opts.user = cfg.user.empty() ? cfg.worker : cfg.user;
        opts.pass = cfg.rpcAuthToken.empty() ? cfg.pass : cfg.rpcAuthToken;
        opts.allowRemote = cfg.allowRemote;
        opts.protocol = cfg.preferStratumV2 ? StratumProtocol::V2 : StratumProtocol::V1;
        opts.onSecurityEvent = [](const std::string& msg) {
            std::cerr << "[security] " << msg << "\n";
            std::terminate();
        };

        StratumPool client(opts);
        client.Connect();
        auto lastPing = std::chrono::steady_clock::now();
        while (true) {
            if (auto jobOpt = client.AwaitJob()) {
                MinerJob job = *jobOpt;
                job.header.bits = ClampBits(job.header.bits, cfg.minTargetBits);
                std::cout << "Received job " << job.jobId << " (diff " << client.CurrentDifficulty() << ") from " << cfg.stratumUrl << "\n";
                if (MineJob(job, cfg, &client))
                    std::cout << "Solution submitted for job " << job.jobId << "\n";
            }
            auto now = std::chrono::steady_clock::now();
            if (now - lastPing > std::chrono::seconds(30)) {
                client.SendKeepalive();
                lastPing = now;
            }
        }
    }

    if (argc < 6) {
        std::cerr << "Usage (standalone): " << argv[0] << " <version> <prevhash> <merkleroot> <time> <bits> [threads]\n"
                  << "Or provide --stratum-url/--stratum-user/--stratum-pass for pool mode." << std::endl;
        return 1;
    }

    MinerJob job{};
    job.header.version = static_cast<uint32_t>(std::stoul(argv[1]));
    job.header.prevBlockHash = FromHex(argv[2]);
    job.header.merkleRoot = FromHex(argv[3]);
    job.header.time = static_cast<uint32_t>(std::stoul(argv[4]));
    job.header.bits = static_cast<uint32_t>(std::stoul(argv[5], nullptr, 16));
    job.header.nonce = 0;
    cfg.threads = (argc >= 7) ? static_cast<uint32_t>(std::stoul(argv[6])) : cfg.threads;

    job.header.bits = ClampBits(job.header.bits, cfg.minTargetBits);
    MineJob(job, cfg, nullptr);
    return 0;
}
