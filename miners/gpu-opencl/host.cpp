#include <CL/cl.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
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

namespace {

struct MinerConfig {
    std::string stratumUrl;
    std::string user;
    std::string pass;
    std::vector<int> devices; // Which OpenCL devices to use
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

void CheckCL(cl_int err, const char* msg = "OpenCL error")
{
    if (err != CL_SUCCESS) {
        std::ostringstream oss;
        oss << msg << ": error code " << err;
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
    std::memcpy(out, &header.version, 4);
    std::memcpy(out + 4, header.prevBlockHash.data(), 32);
    std::memcpy(out + 36, header.merkleRoot.data(), 32);
    std::memcpy(out + 68, &header.time, 4);
    std::memcpy(out + 72, &header.bits, 4);
}

void TargetToArray(const uint256& target, uint32_t* arr)
{
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

class OpenCLContext {
public:
    OpenCLContext()
    {
        cl_uint platformCount = 0;
        clGetPlatformIDs(0, nullptr, &platformCount);
        if (platformCount == 0) throw std::runtime_error("No OpenCL platforms found");
        std::vector<cl_platform_id> platforms(platformCount);
        clGetPlatformIDs(platformCount, platforms.data(), nullptr);

        for (auto pid : platforms) {
            cl_uint deviceCount = 0;
            // Query GPU and accelerator devices (CPU devices excluded for performance)
            // To include CPU devices, add CL_DEVICE_TYPE_CPU to the device type mask
            if (clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, 0, nullptr, &deviceCount) != CL_SUCCESS)
                continue;
            if (deviceCount == 0) continue;
            std::vector<cl_device_id> devices(deviceCount);
            clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, deviceCount, devices.data(), nullptr);
            for (auto dev : devices) {
                devices_.push_back(dev);
                platformForDevice_[dev] = pid;
            }
        }
        if (devices_.empty()) throw std::runtime_error("No OpenCL devices found");
    }

    size_t deviceCount() const { return devices_.size(); }
    cl_device_id device(size_t idx) const { return devices_.at(idx); }

    std::string deviceName(size_t idx) const
    {
        char name[256];
        clGetDeviceInfo(devices_.at(idx), CL_DEVICE_NAME, sizeof(name), name, nullptr);
        return std::string(name);
    }

    cl_context contextFor(cl_device_id dev)
    {
        auto it = contexts_.find(dev);
        if (it != contexts_.end()) return it->second;
        cl_int err = 0;
        cl_context ctx = clCreateContext(nullptr, 1, &dev, nullptr, nullptr, &err);
        CheckCL(err, "Failed to create OpenCL context");
        contexts_[dev] = ctx;
        return ctx;
    }

    cl_command_queue queueFor(cl_device_id dev)
    {
        auto it = queues_.find(dev);
        if (it != queues_.end()) return it->second;
        cl_int err = 0;
        cl_command_queue q = clCreateCommandQueue(contextFor(dev), dev, 0, &err);
        CheckCL(err, "Failed to create OpenCL queue");
        queues_[dev] = q;
        return q;
    }

    cl_program buildProgram(cl_device_id dev, const std::string& source)
    {
        cl_int err = 0;
        const char* src = source.c_str();
        size_t len = source.size();
        cl_context ctx = contextFor(dev);
        cl_program program = clCreateProgramWithSource(ctx, 1, &src, &len, &err);
        CheckCL(err, "Failed to create OpenCL program");
        err = clBuildProgram(program, 1, &dev, "-cl-std=CL1.2", nullptr, nullptr);
        if (err != CL_SUCCESS) {
            size_t logSize = 0;
            clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
            std::string log(logSize, '\0');
            clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
            throw std::runtime_error("OpenCL build failed: " + log);
        }
        return program;
    }

private:
    std::vector<cl_device_id> devices_;
    std::map<cl_device_id, cl_platform_id> platformForDevice_;
    std::map<cl_device_id, cl_context> contexts_;
    std::map<cl_device_id, cl_command_queue> queues_;
};

std::string LoadKernel()
{
    // Try multiple possible paths
    std::vector<std::string> paths = {
        "miners/gpu-opencl/kernel.cl",
        "gpu-opencl/kernel.cl",
        "kernel.cl",
        "../miners/gpu-opencl/kernel.cl",
        "../../miners/gpu-opencl/kernel.cl"
    };
    
    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f) {
            return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }
    }
    throw std::runtime_error("Failed to find kernel.cl in any expected location");
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

void RunBenchmark(const MinerConfig& cfg, OpenCLContext& ctx)
{
    auto devices = cfg.devices.empty() ? std::vector<int>{0} : cfg.devices;
    
    std::cout << "Running benchmark on " << devices.size() << " OpenCL device(s)..." << std::endl;
    
    BlockHeader header{};
    header.bits = consensus::Main().nGenesisBits;
    header.version = 1;
    header.time = static_cast<uint32_t>(std::time(nullptr));
    
    uint256 target = DeriveTargetFromBits(header.bits);
    
    std::atomic<uint64_t> totalHashes{0};
    std::atomic<bool> stop{false};
    
    std::string kernelSource = LoadKernel();
    
    auto worker = [&](int deviceIdx) {
        try {
            cl_device_id dev = ctx.device(deviceIdx);
            cl_program program = ctx.buildProgram(dev, kernelSource);
            cl_int err = 0;
            cl_kernel kernel = clCreateKernel(program, "sha256d_kernel", &err);
            CheckCL(err, "Failed to create kernel");
            
            cl_context clctx = ctx.contextFor(dev);
            cl_command_queue queue = ctx.queueFor(dev);
            
            uint8_t header76[76];
            SerializeHeaderTo76Bytes(header, header76);
            
            uint32_t targetArr[8];
            TargetToArray(target, targetArr);
            
            uint32_t batchSize = cfg.intensity ? (cfg.intensity * 1024) : (1 << 20);
            
            cl_mem headerBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 76, header76, &err);
            cl_mem targetBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 8 * sizeof(uint32_t), targetArr, &err);
            cl_mem solutionBuf = clCreateBuffer(clctx, CL_MEM_WRITE_ONLY, 9 * sizeof(uint32_t), nullptr, &err);
            int zero = 0;
            cl_mem foundBuf = clCreateBuffer(clctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &zero, &err);
            
            uint32_t nonce = deviceIdx * (1U << 28);
            
            while (!stop.load(std::memory_order_relaxed)) {
                int zero = 0;
                clEnqueueWriteBuffer(queue, foundBuf, CL_TRUE, 0, sizeof(int), &zero, 0, nullptr, nullptr);
                
                clSetKernelArg(kernel, 0, sizeof(cl_mem), &headerBuf);
                clSetKernelArg(kernel, 1, sizeof(uint32_t), &nonce);
                clSetKernelArg(kernel, 2, sizeof(cl_mem), &targetBuf);
                clSetKernelArg(kernel, 3, sizeof(cl_mem), &solutionBuf);
                clSetKernelArg(kernel, 4, sizeof(cl_mem), &foundBuf);
                
                size_t global = batchSize;
                size_t local = 256;
                clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
                clFinish(queue);
                
                totalHashes.fetch_add(batchSize, std::memory_order_relaxed);
                nonce += batchSize;
            }
            
            clReleaseMemObject(headerBuf);
            clReleaseMemObject(targetBuf);
            clReleaseMemObject(solutionBuf);
            clReleaseMemObject(foundBuf);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        } catch (const std::exception& e) {
            std::cerr << "Device " << deviceIdx << " error: " << e.what() << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    for (size_t i = 0; i < devices.size(); ++i) {
        int dev = devices[i];
        if (dev >= 0 && dev < static_cast<int>(ctx.deviceCount())) {
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

bool MineJob(const MinerJob& job, const MinerConfig& cfg, StratumPool* pool, 
             OpenCLContext& ctx, std::atomic<bool>& shouldStop)
{
    const auto& params = consensus::Main();
    
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
    std::string kernelSource = LoadKernel();
    
    auto worker = [&](int deviceIdx, int threadIdx) {
        try {
            cl_device_id dev = ctx.device(deviceIdx);
            cl_program program = ctx.buildProgram(dev, kernelSource);
            cl_int err = 0;
            cl_kernel kernel = clCreateKernel(program, "sha256d_kernel", &err);
            CheckCL(err, "Failed to create kernel");
            
            cl_context clctx = ctx.contextFor(dev);
            cl_command_queue queue = ctx.queueFor(dev);
            
            cl_mem headerBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 76, (void*)header76, &err);
            cl_mem targetBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 8 * sizeof(uint32_t), (void*)targetArr, &err);
            cl_mem solutionBuf = clCreateBuffer(clctx, CL_MEM_WRITE_ONLY, 9 * sizeof(uint32_t), nullptr, &err);
            int zero = 0;
            cl_mem foundBuf = clCreateBuffer(clctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &zero, &err);
            
            uint32_t nonce = threadIdx * (1U << 28);
            
            while (!found.load(std::memory_order_relaxed) && !shouldStop.load(std::memory_order_relaxed)) {
                int zero = 0;
                clEnqueueWriteBuffer(queue, foundBuf, CL_TRUE, 0, sizeof(int), &zero, 0, nullptr, nullptr);
                
                clSetKernelArg(kernel, 0, sizeof(cl_mem), &headerBuf);
                clSetKernelArg(kernel, 1, sizeof(uint32_t), &nonce);
                clSetKernelArg(kernel, 2, sizeof(cl_mem), &targetBuf);
                clSetKernelArg(kernel, 3, sizeof(cl_mem), &solutionBuf);
                clSetKernelArg(kernel, 4, sizeof(cl_mem), &foundBuf);
                
                size_t global = batchSize;
                size_t local = 256;
                clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
                clFinish(queue);
                
                int foundFlag = 0;
                clEnqueueReadBuffer(queue, foundBuf, CL_TRUE, 0, sizeof(int), &foundFlag, 0, nullptr, nullptr);
                
                if (foundFlag) {
                    uint32_t solution[9];
                    clEnqueueReadBuffer(queue, solutionBuf, CL_TRUE, 0, 9 * sizeof(uint32_t), solution, 0, nullptr, nullptr);
                    
                    found.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> g(submitMutex);
                    std::cout << "[device " << deviceIdx << "] found nonce: " << solution[0] << "\n";
                    
                    if (pool) {
                        pool->SubmitResult(job, solution[0]);
                    }
                    break;
                }
                
                nonce += batchSize;
            }
            
            clReleaseMemObject(headerBuf);
            clReleaseMemObject(targetBuf);
            clReleaseMemObject(solutionBuf);
            clReleaseMemObject(foundBuf);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
        } catch (const std::exception& e) {
            std::cerr << "Device " << deviceIdx << " mining error: " << e.what() << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    int threadIdx = 0;
    for (size_t i = 0; i < devices.size(); ++i) {
        int dev = devices[i];
        if (dev >= 0 && dev < static_cast<int>(ctx.deviceCount())) {
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
        
        OpenCLContext ctx;
        
        // Auto-detect devices if none specified
        if (cfg.devices.empty()) {
            size_t deviceCount = ctx.deviceCount();
            if (deviceCount > 0) {
                std::cout << "Auto-detected " << deviceCount << " OpenCL device(s)\n";
                for (size_t i = 0; i < deviceCount; ++i) {
                    std::cout << "  Device " << i << ": " << ctx.deviceName(i) << "\n";
                }
                cfg.devices.push_back(0); // Use first device by default
            } else {
                std::cerr << "No OpenCL devices found\n";
                return 1;
            }
        }
        
        if (cfg.benchmark) {
            RunBenchmark(cfg, ctx);
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
                    if (MineJob(job, cfg, &client, ctx, shouldStop)) {
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
        std::cerr << "OpenCL miner error: " << e.what() << std::endl;
        return 1;
    }
}
