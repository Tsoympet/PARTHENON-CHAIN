#include <CL/cl.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include "../stratum.h"

namespace miner {

struct WorkItem {
    std::array<uint8_t, 76> headerPrefix{}; // header without nonce
    std::array<uint32_t, 8> target{};
    uint32_t nonceStart{0};
    uint32_t nonceCount{1 << 24};
};

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
            if (clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CPU, 0, nullptr, &deviceCount) != CL_SUCCESS)
                continue;
            if (deviceCount == 0) continue;
            std::vector<cl_device_id> devices(deviceCount);
            clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CPU, deviceCount, devices.data(), nullptr);
            for (auto dev : devices) {
                devices_.push_back(dev);
                platformForDevice_[dev] = pid;
            }
        }
        if (devices_.empty()) throw std::runtime_error("No OpenCL devices found");
    }

    size_t deviceCount() const { return devices_.size(); }

    cl_device_id device(size_t idx) const { return devices_.at(idx); }

    cl_context contextFor(cl_device_id dev)
    {
        auto it = contexts_.find(dev);
        if (it != contexts_.end()) return it->second;
        cl_platform_id pid = platformForDevice_.at(dev);
        cl_int err = 0;
        cl_context ctx = clCreateContext(nullptr, 1, &dev, nullptr, nullptr, &err);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to create OpenCL context");
        contexts_[dev] = ctx;
        return ctx;
    }

    cl_command_queue queueFor(cl_device_id dev)
    {
        auto it = queues_.find(dev);
        if (it != queues_.end()) return it->second;
        cl_int err = 0;
        cl_command_queue q = clCreateCommandQueue(contextFor(dev), dev, 0, &err);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to create OpenCL queue");
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
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to create OpenCL program");
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

static std::string LoadKernel()
{
    std::ifstream f("miners/gpu-opencl/kernel.cl");
    if (!f) throw std::runtime_error("Failed to open kernel.cl");
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

uint32_t MineOnDevice(cl_device_id dev, const WorkItem& work, std::array<uint32_t, 9>& solution, OpenCLContext& ctx)
{
    std::string source = LoadKernel();
    cl_program program = ctx.buildProgram(dev, source);
    cl_int err = 0;
    cl_kernel kernel = clCreateKernel(program, "sha256d_kernel", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create kernel");

    cl_context clctx = ctx.contextFor(dev);
    cl_command_queue queue = ctx.queueFor(dev);

    cl_mem headerBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, work.headerPrefix.size(), (void*)work.headerPrefix.data(), &err);
    cl_mem targetBuf = clCreateBuffer(clctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, work.target.size() * sizeof(uint32_t), (void*)work.target.data(), &err);
    cl_mem solutionBuf = clCreateBuffer(clctx, CL_MEM_WRITE_ONLY, solution.size() * sizeof(uint32_t), nullptr, &err);
    int zero = 0;
    cl_mem foundBuf = clCreateBuffer(clctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &zero, &err);

    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &headerBuf);
    err |= clSetKernelArg(kernel, 1, sizeof(uint32_t), &work.nonceStart);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &targetBuf);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &solutionBuf);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &foundBuf);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to set kernel args");

    size_t global = work.nonceCount;
    size_t local = 256;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue kernel");
    clFinish(queue);

    int found = 0;
    clEnqueueReadBuffer(queue, foundBuf, CL_TRUE, 0, sizeof(int), &found, 0, nullptr, nullptr);
    if (found) {
        clEnqueueReadBuffer(queue, solutionBuf, CL_TRUE, 0, solution.size() * sizeof(uint32_t), solution.data(), 0, nullptr, nullptr);
    }

    clReleaseMemObject(headerBuf);
    clReleaseMemObject(targetBuf);
    clReleaseMemObject(solutionBuf);
    clReleaseMemObject(foundBuf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);

    return found ? solution[0] : 0;
}

uint32_t MineOpenCL(const WorkItem& work, std::array<uint32_t, 9>& solution)
{
    try {
        OpenCLContext ctx;
        uint32_t result = 0;
        for (size_t i = 0; i < ctx.deviceCount(); ++i) {
            result = MineOnDevice(ctx.device(i), work, solution, ctx);
            if (result) break;
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "OpenCL miner falling back to CPU: " << e.what() << std::endl;
        return 0;
    }
}

void Benchmark()
{
    WorkItem work{};
    work.nonceCount = 1 << 20;
    for (int i = 0; i < 8; ++i) work.target[i] = 0x0000ffff;
    std::array<uint32_t, 9> solution{};
    auto start = std::chrono::steady_clock::now();
    uint32_t found = MineOpenCL(work, solution);
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double hashes = static_cast<double>(work.nonceCount);
    std::cout << "OpenCL benchmark: " << hashes / (ms / 1000.0) / 1e6 << " MH/s";
    if (found) std::cout << " found nonce=" << found;
    std::cout << std::endl;
}

} // namespace miner

int main()
{
    try {
        miner::Benchmark();
    } catch (const std::exception& e) {
        std::cerr << "OpenCL miner error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
