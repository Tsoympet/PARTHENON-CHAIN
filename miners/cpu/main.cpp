#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <fstream>
#include <ctime>

#include <boost/asio.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "../../layer1-core/block/block.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/pow/difficulty.h"

namespace {

using boost::asio::ip::tcp;

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

std::string ToHex(const uint256& h)
{
    std::ostringstream oss;
    for (auto b : h)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

struct MinerJob {
    BlockHeader header{};
    uint256 target{};
    std::string jobId;
};

uint32_t ParseLE32(const std::string& hex, size_t offset)
{
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        auto byte = static_cast<uint32_t>(std::stoul(hex.substr((offset + i) * 2, 2), nullptr, 16));
        out |= byte << (8 * i);
    }
    return out;
}

MinerJob ParseHeaderJob(const std::string& headerHex, const std::string& targetHex)
{
    if (headerHex.size() != 160)
        throw std::invalid_argument("expected 80-byte header hex");
    MinerJob job{};
    job.header.version = ParseLE32(headerHex, 0);
    job.header.prevBlockHash = FromHex(headerHex.substr(8, 64));
    job.header.merkleRoot = FromHex(headerHex.substr(72, 64));
    job.header.time = ParseLE32(headerHex, 136 / 2);
    job.header.bits = ParseLE32(headerHex, 144 / 2);
    job.header.nonce = ParseLE32(headerHex, 152 / 2);
    job.target = targetHex.empty() ? uint256{} : FromHex(targetHex);
    return job;
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

class StratumClient {
public:
    StratumClient(const std::string& url, const std::string& user, const std::string& pass, bool allowRemote)
        : ctx_(), socket_(ctx_), user_(user), pass_(pass)
    {
        auto pos = url.find("://");
        std::string hostport = pos == std::string::npos ? url : url.substr(pos + 3);
        auto colon = hostport.find(":");
        if (colon == std::string::npos)
            throw std::invalid_argument("stratum url must include host:port");
        host_ = hostport.substr(0, colon);
        port_ = hostport.substr(colon + 1);

        if (!allowRemote && host_ != "127.0.0.1" && host_ != "localhost")
            throw std::runtime_error("remote stratum connections require --allow-remote");
    }

    void Connect()
    {
        tcp::resolver resolver(ctx_);
        auto endpoints = resolver.resolve(host_, port_);
        boost::asio::connect(socket_, endpoints);
        Subscribe();
        Authorize();
    }

    MinerJob AwaitJob()
    {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket_, buf, '\n');
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);
        if (line.find("mining.notify") == std::string::npos)
            throw std::runtime_error("unexpected stratum message: " + line);

        auto header = ExtractField(line, "header");
        auto target = ExtractField(line, "target");
        MinerJob job = ParseHeaderJob(header, target);
        job.jobId = ExtractField(line, "job");
        return job;
    }

    void SubmitResult(const MinerJob& job)
    {
        std::ostringstream payload;
        payload << "{\"id\":4,\"method\":\"mining.submit\",\"params\":[\"" << user_ << "\",\"" << job.jobId
                << "\",\"" << ToHex(job.header.prevBlockHash) << "\",\"" << std::hex << job.header.nonce << "\"]}\n";
        boost::asio::write(socket_, boost::asio::buffer(payload.str()));
    }

private:
    void Subscribe()
    {
        static const std::string subscribe = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[]}\n";
        boost::asio::write(socket_, boost::asio::buffer(subscribe));
    }

    void Authorize()
    {
        std::ostringstream auth;
        auth << "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"" << user_ << "\",\"" << pass_ << "\"]}\n";
        boost::asio::write(socket_, boost::asio::buffer(auth.str()));
    }

    static std::string ExtractField(const std::string& json, const std::string& key)
    {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos)
            return {};
        auto colon = json.find(":", pos);
        auto quote = json.find("\"", colon + 1);
        auto end = json.find("\"", quote + 1);
        if (quote == std::string::npos || end == std::string::npos)
            return {};
        return json.substr(quote + 1, end - quote - 1);
    }

    boost::asio::io_context ctx_;
    tcp::socket socket_;
    std::string host_;
    std::string port_;
    std::string user_;
    std::string pass_;
};

struct MinerConfig {
    std::string stratumUrl;
    std::string user;
    std::string pass;
    uint32_t threads{1};
    bool benchmark{false};
    int benchmarkSeconds{10};
    bool allowRemote{false};
    uint32_t minTargetBits{0};
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

void PrintHash(const uint256& h)
{
    for (auto b : h)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
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
        else if (arg == "--benchmark") cfg.benchmark = true;
        else if (arg == "--benchmark-seconds" && i + 1 < argc) cfg.benchmarkSeconds = std::stoi(argv[++i]);
        else if (arg == "--allow-remote") cfg.allowRemote = true;
        else if (arg == "--min-target-bits" && i + 1 < argc) cfg.minTargetBits = std::stoul(argv[++i], nullptr, 16);
    }
    if (cfg.threads == 0) cfg.threads = 1;
    return cfg;
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
        while (!stop.load(std::memory_order_relaxed)) {
            BlockHash(local);
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
    std::cout << "Benchmark: " << rate / 1e6 << " MH/s across " << cfg.threads << " threads\n";
}

bool MineJob(const MinerJob& baseJob, const MinerConfig& cfg, StratumClient* client)
{
    const auto& params = consensus::Main();
    std::atomic<bool> found{false};
    std::atomic<uint32_t> nonceCounter{baseJob.header.nonce};
    std::mutex submitMutex;

    auto worker = [&](int idx) {
        MinerJob job = baseJob;
        while (!found.load(std::memory_order_relaxed)) {
            uint32_t startNonce = nonceCounter.fetch_add(1024, std::memory_order_relaxed);
            for (uint32_t n = 0; n < 1024 && !found.load(std::memory_order_relaxed); ++n) {
                job.header.nonce = startNonce + n;
                auto hash = BlockHash(job.header);
                bool meets = (ToInteger(job.target) == 0)
                    ? powalgo::CheckProofOfWork(hash, ClampBits(job.header.bits, cfg.minTargetBits), params)
                    : MeetsExplicitTarget(hash, job.target);
                if (meets) {
                    found.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> g(submitMutex);
                    std::cout << "[thread " << idx << "] found nonce: " << job.header.nonce << "\nHash: 0x";
                    PrintHash(hash);
                    std::cout << "\n";
                    if (client)
                        client->SubmitResult(job);
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
    if (cfg.benchmark) {
        RunBenchmark(cfg);
        return 0;
    }

    if (!cfg.stratumUrl.empty()) {
        StratumClient client(cfg.stratumUrl, cfg.user, cfg.pass, cfg.allowRemote);
        client.Connect();
        while (true) {
            MinerJob job = client.AwaitJob();
            job.header.bits = ClampBits(job.header.bits, cfg.minTargetBits);
            std::cout << "Received job " << job.jobId << " from " << cfg.stratumUrl << "\n";
            if (MineJob(job, cfg, &client))
                std::cout << "Solution submitted for job " << job.jobId << "\n";
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
