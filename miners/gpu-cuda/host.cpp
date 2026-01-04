#include <cuda.h>
#include <cuda_runtime.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../../layer1-core/block/block.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/pow/difficulty.h"
#include "../stratum.h"

extern "C" void sha256d_kernel(const uint8_t* header76, uint32_t nonceStart, const uint32_t* target, uint32_t* solution, int* found);

namespace {

struct MinerConfig {
    std::string stratumUrl;
    std::string user;
    std::string pass;
    std::vector<int> devices; // Which CUDA devices to use
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

void CheckCuda(cudaError_t err, const char* msg = "CUDA error")
{
    if (err != cudaSuccess) {
        std::ostringstream oss;
        oss << msg << ": " << cudaGetErrorString(err);
        throw std::runtime_error(oss.str());
    }
}

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

void SerializeHeaderTo76Bytes(const BlockHeader& header, uint8_t* out)
{
    // Serialize first 76 bytes of header (everything except nonce)
    std::memcpy(out, &header.version, 4);
    std::memcpy(out + 4, header.prevBlockHash.data(), 32);
    std::memcpy(out + 36, header.merkleRoot.data(), 32);
    std::memcpy(out + 68, &header.time, 4);
    std::memcpy(out + 72, &header.bits, 4);
}

void TargetToArray(const uint256& target, uint32_t* arr)
{
    // Convert target to array of uint32_t for GPU
    for (int i = 0; i < 8; ++i) {
        arr[i] = (static_cast<uint32_t>(target[i*4]) << 24) |
                 (static_cast<uint32_t>(target[i*4+1]) << 16) |
                 (static_cast<uint32_t>(target[i*4+2]) << 8) |
                 static_cast<uint32_t>(target[i*4+3]);
    }
}

uint256 DeriveTargetFromBits(uint32_t bits)
{
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00ffffff;
    uint256 target{};
    
    if (exponent <= 3) {
        mantissa >>= (8 * (3 - exponent));
        target[29] = (mantissa >> 16) & 0xff;
        target[30] = (mantissa >> 8) & 0xff;
        target[31] = mantissa & 0xff;
    } else {
        size_t offset = 32 - exponent;
        if (offset < 32) {
            target[offset] = (mantissa >> 16) & 0xff;
            if (offset + 1 < 32) target[offset + 1] = (mantissa >> 8) & 0xff;
            if (offset + 2 < 32) target[offset + 2] = mantissa & 0xff;
        }
    }
    return target;
}

MinerConfig ParseArgs(int argc, char** argv)
{
    MinerConfig cfg{};
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stratum-url" && i + 1 < argc) cfg.stratumUrl = argv[++i];
        else if (arg == "--stratum-user" && i + 1 < argc) cfg.user = argv[++i];
        else if (arg == "--stratum-pass" && i + 1 < argc) cfg.pass = argv[++i];
        else if (arg == "--devices" && i + 1 < argc) {
            std::string devStr = argv[++i];
            std::istringstream ss(devStr);
            std::string token;
            while (std::getline(ss, token, ',')) {
                cfg.devices.push_back(std::stoi(token));
            }
        }
        else if (arg == "--benchmark" || arg == "--bench") cfg.benchmark = true;
        else if (arg == "--benchmark-seconds" && i + 1 < argc) cfg.benchmarkSeconds = std::stoi(argv[++i]);
        else if (arg == "--allow-remote") cfg.allowRemote = true;
        else if (arg == "--min-target-bits" && i + 1 < argc) cfg.minTargetBits = std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--config" && i + 1 < argc) cfg.configPath = argv[++i];
        else if (arg == "--intensity" && i + 1 < argc) cfg.intensity = std::stoul(argv[++i]);
        else if (arg == "--worker" && i + 1 < argc) cfg.worker = argv[++i];
        else if (arg == "--stratum-v2") cfg.preferStratumV2 = true;
        else if (arg == "--rpc-auth-token" && i + 1 < argc) cfg.rpcAuthToken = argv[++i];
    }
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
        cfg.allowRemote = tree.get("allow_remote", cfg.allowRemote);
        cfg.minTargetBits = tree.get("min_target_bits", cfg.minTargetBits);
        cfg.benchmarkSeconds = tree.get("benchmark_seconds", cfg.benchmarkSeconds);
        cfg.intensity = tree.get("intensity", cfg.intensity);
        cfg.benchmark = tree.get("benchmark", cfg.benchmark);
        cfg.preferStratumV2 = tree.get("stratum_v2", cfg.preferStratumV2);
        cfg.rpcAuthToken = tree.get("rpc_auth_token", cfg.rpcAuthToken);
        
        // Load devices array
        if (auto devArray = tree.get_child_optional("devices")) {
            cfg.devices.clear();
            for (const auto& item : *devArray) {
                cfg.devices.push_back(item.second.get_value<int>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to read config: " << e.what() << "\n";
    }
}

void RunBenchmark(const MinerConfig& cfg)
{
    int deviceCount = 0;
    CheckCuda(cudaGetDeviceCount(&deviceCount), "Failed to get device count");
    
    if (deviceCount == 0) {
        std::cerr << "No CUDA devices available\n";
        return;
    }
    
    auto devices = cfg.devices.empty() ? std::vector<int>{0} : cfg.devices;
    
    std::cout << "Running benchmark on " << devices.size() << " CUDA device(s)..." << std::endl;
    
    BlockHeader header{};
    header.bits = consensus::Main().nGenesisBits;
    header.version = 1;
    header.time = static_cast<uint32_t>(std::time(nullptr));
    
    uint256 target = DeriveTargetFromBits(header.bits);
    
    std::atomic<uint64_t> totalHashes{0};
    std::atomic<bool> stop{false};
    
    auto worker = [&](int deviceId) {
        try {
            CheckCuda(cudaSetDevice(deviceId));
            
            uint8_t header76[76];
            SerializeHeaderTo76Bytes(header, header76);
            
            uint32_t targetArr[8];
            TargetToArray(target, targetArr);
            
            uint32_t batchSize = cfg.intensity ? (cfg.intensity * 1024) : (1 << 20);
            
            uint8_t* d_header = nullptr;
            uint32_t* d_target = nullptr;
            uint32_t* d_solution = nullptr;
            int* d_found = nullptr;
            
            CheckCuda(cudaMalloc(&d_header, 76));
            CheckCuda(cudaMalloc(&d_target, 8 * sizeof(uint32_t)));
            CheckCuda(cudaMalloc(&d_solution, 9 * sizeof(uint32_t)));
            CheckCuda(cudaMalloc(&d_found, sizeof(int)));
            
            CheckCuda(cudaMemcpy(d_header, header76, 76, cudaMemcpyHostToDevice));
            CheckCuda(cudaMemcpy(d_target, targetArr, 8 * sizeof(uint32_t), cudaMemcpyHostToDevice));
            
            uint32_t nonce = deviceId * (1U << 28);
            
            while (!stop.load(std::memory_order_relaxed)) {
                int zero = 0;
                CheckCuda(cudaMemcpy(d_found, &zero, sizeof(int), cudaMemcpyHostToDevice));
                
                dim3 block(256);
                dim3 grid((batchSize + block.x - 1) / block.x);
                sha256d_kernel<<<grid, block>>>(d_header, nonce, d_target, d_solution, d_found);
                CheckCuda(cudaDeviceSynchronize());
                
                totalHashes.fetch_add(batchSize, std::memory_order_relaxed);
                nonce += batchSize;
            }
            
            cudaFree(d_header);
            cudaFree(d_target);
            cudaFree(d_solution);
            cudaFree(d_found);
        } catch (const std::exception& e) {
            std::cerr << "Device " << deviceId << " error: " << e.what() << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    for (int dev : devices) {
        if (dev >= 0 && dev < deviceCount) {
            threads.emplace_back(worker, dev);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(cfg.benchmarkSeconds));
    stop.store(true);
    
    for (auto& t : threads) t.join();
    
    double rate = static_cast<double>(totalHashes.load()) / cfg.benchmarkSeconds;
    std::cout << "Benchmark: " << rate / 1e6 << " MH/s across " << devices.size() << " device(s)";
    if (cfg.intensity)
        std::cout << " (intensity " << cfg.intensity << ")";
    std::cout << "\n";
}

bool MineJob(const MinerJob& job, const MinerConfig& cfg, StratumPool* pool, std::atomic<bool>& shouldStop)
{
    const auto& params = consensus::Main();
    
    int deviceCount = 0;
    CheckCuda(cudaGetDeviceCount(&deviceCount), "Failed to get device count");
    
    auto devices = cfg.devices.empty() ? std::vector<int>{0} : cfg.devices;
    
    uint8_t header76[76];
    SerializeHeaderTo76Bytes(job.header, header76);
    
    uint256 target = job.target;
    if (ToInteger(target) == 0) {
        target = DeriveTargetFromBits(ClampBits(job.header.bits, cfg.minTargetBits));
    }
    
    uint32_t targetArr[8];
    TargetToArray(target, targetArr);
    
    std::atomic<bool> found{false};
    std::mutex submitMutex;
    
    uint32_t batchSize = cfg.intensity ? (cfg.intensity * 1024) : (1 << 20);
    
    auto worker = [&](int deviceId, int threadIdx) {
        try {
            CheckCuda(cudaSetDevice(deviceId));
            
            uint8_t* d_header = nullptr;
            uint32_t* d_target = nullptr;
            uint32_t* d_solution = nullptr;
            int* d_found = nullptr;
            
            CheckCuda(cudaMalloc(&d_header, 76));
            CheckCuda(cudaMalloc(&d_target, 8 * sizeof(uint32_t)));
            CheckCuda(cudaMalloc(&d_solution, 9 * sizeof(uint32_t)));
            CheckCuda(cudaMalloc(&d_found, sizeof(int)));
            
            CheckCuda(cudaMemcpy(d_header, header76, 76, cudaMemcpyHostToDevice));
            CheckCuda(cudaMemcpy(d_target, targetArr, 8 * sizeof(uint32_t), cudaMemcpyHostToDevice));
            
            uint32_t nonce = threadIdx * (1U << 28);
            
            while (!found.load(std::memory_order_relaxed) && !shouldStop.load(std::memory_order_relaxed)) {
                int zero = 0;
                CheckCuda(cudaMemcpy(d_found, &zero, sizeof(int), cudaMemcpyHostToDevice));
                
                dim3 block(256);
                dim3 grid((batchSize + block.x - 1) / block.x);
                sha256d_kernel<<<grid, block>>>(d_header, nonce, d_target, d_solution, d_found);
                CheckCuda(cudaDeviceSynchronize());
                
                int foundFlag = 0;
                CheckCuda(cudaMemcpy(&foundFlag, d_found, sizeof(int), cudaMemcpyDeviceToHost));
                
                if (foundFlag) {
                    uint32_t solution[9];
                    CheckCuda(cudaMemcpy(solution, d_solution, 9 * sizeof(uint32_t), cudaMemcpyDeviceToHost));
                    
                    found.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> g(submitMutex);
                    std::cout << "[device " << deviceId << "] found nonce: " << solution[0] << "\n";
                    
                    if (pool) {
                        pool->SubmitResult(job, solution[0]);
                    }
                    break;
                }
                
                nonce += batchSize;
            }
            
            cudaFree(d_header);
            cudaFree(d_target);
            cudaFree(d_solution);
            cudaFree(d_found);
        } catch (const std::exception& e) {
            std::cerr << "Device " << deviceId << " mining error: " << e.what() << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    int threadIdx = 0;
    for (int dev : devices) {
        if (dev >= 0 && dev < deviceCount) {
            threads.emplace_back(worker, dev, threadIdx++);
        }
    }
    
    for (auto& t : threads) t.join();
    
    return found.load();
}

} // namespace

int main(int argc, char** argv)
{
    try {
        MinerConfig cfg = ParseArgs(argc, argv);
        LoadConfig(cfg);
        
        // Auto-detect devices if none specified
        if (cfg.devices.empty()) {
            int deviceCount = 0;
            cudaGetDeviceCount(&deviceCount);
            if (deviceCount > 0) {
                std::cout << "Auto-detected " << deviceCount << " CUDA device(s)\n";
                for (int i = 0; i < deviceCount; ++i) {
                    cudaDeviceProp prop;
                    cudaGetDeviceProperties(&prop, i);
                    std::cout << "  Device " << i << ": " << prop.name << "\n";
                }
                cfg.devices.push_back(0); // Use first device by default
            } else {
                std::cerr << "No CUDA devices found\n";
                return 1;
            }
        }
        
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
            std::atomic<bool> shouldStop{false};
            
            while (true) {
                if (auto jobOpt = client.AwaitJob()) {
                    MinerJob job = *jobOpt;
                    job.header.bits = ClampBits(job.header.bits, cfg.minTargetBits);
                    std::cout << "Received job " << job.jobId << " (diff " << client.CurrentDifficulty() 
                              << ") from " << cfg.stratumUrl << "\n";
                    
                    shouldStop.store(false);
                    if (MineJob(job, cfg, &client, shouldStop)) {
                        std::cout << "Solution submitted for job " << job.jobId << "\n";
                    }
                }
                
                auto now = std::chrono::steady_clock::now();
                if (now - lastPing > std::chrono::seconds(30)) {
                    client.SendKeepalive();
                    lastPing = now;
                }
            }
        } else {
            std::cerr << "Usage: " << argv[0] << " --benchmark [--devices 0,1] [--intensity N]\n"
                      << "   or: " << argv[0] << " --stratum-url URL --stratum-user USER --stratum-pass PASS\n"
                      << "   or: " << argv[0] << " --config CONFIG.json\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "CUDA miner error: " << e.what() << std::endl;
        return 1;
    }
}
