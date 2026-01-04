#include <cuda.h>
#include <cuda_runtime.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" void sha256d_kernel(const uint8_t* header76, uint32_t nonceStart, const uint32_t* target, uint32_t* solution, int* found);

namespace miner {

struct WorkItem {
    std::array<uint8_t, 76> headerPrefix{}; // header without nonce
    std::array<uint32_t, 8> target{};
    uint32_t nonceStart{0};
    uint32_t nonceCount{1 << 24};
};

static void CheckCuda(cudaError_t err)
{
    if (err != cudaSuccess) {
        throw std::runtime_error(cudaGetErrorString(err));
    }
}

uint32_t MineOnDevice(int device, const WorkItem& work, std::array<uint32_t, 9>& solution)
{
    CheckCuda(cudaSetDevice(device));
    uint8_t* d_header{nullptr};
    uint32_t* d_target{nullptr};
    uint32_t* d_solution{nullptr};
    int* d_found{nullptr};

    CheckCuda(cudaMalloc(&d_header, work.headerPrefix.size()));
    CheckCuda(cudaMalloc(&d_target, work.target.size() * sizeof(uint32_t)));
    CheckCuda(cudaMalloc(&d_solution, solution.size() * sizeof(uint32_t)));
    CheckCuda(cudaMalloc(&d_found, sizeof(int)));

    CheckCuda(cudaMemcpy(d_header, work.headerPrefix.data(), work.headerPrefix.size(), cudaMemcpyHostToDevice));
    CheckCuda(cudaMemcpy(d_target, work.target.data(), work.target.size() * sizeof(uint32_t), cudaMemcpyHostToDevice));
    int zero = 0;
    CheckCuda(cudaMemcpy(d_found, &zero, sizeof(int), cudaMemcpyHostToDevice));

    dim3 block(256);
    dim3 grid((work.nonceCount + block.x - 1) / block.x);
    sha256d_kernel<<<grid, block>>>(d_header, work.nonceStart, d_target, d_solution, d_found);
    CheckCuda(cudaDeviceSynchronize());

    int found = 0;
    CheckCuda(cudaMemcpy(&found, d_found, sizeof(int), cudaMemcpyDeviceToHost));
    if (found) {
        CheckCuda(cudaMemcpy(solution.data(), d_solution, solution.size() * sizeof(uint32_t), cudaMemcpyDeviceToHost));
    }

    cudaFree(d_header);
    cudaFree(d_target);
    cudaFree(d_solution);
    cudaFree(d_found);
    return found ? solution[0] : 0;
}

void Benchmark()
{
    int devices = 0;
    if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) {
        std::cerr << "No CUDA devices available" << std::endl;
        return;
    }

    WorkItem work{};
    work.nonceCount = 1 << 22;
    for (int i = 0; i < 8; ++i) work.target[i] = 0x0000ffff;

    auto start = std::chrono::steady_clock::now();
    std::array<uint32_t, 9> solution{};
    for (int d = 0; d < devices; ++d) {
        MineOnDevice(d, work, solution);
    }
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double hashes = static_cast<double>(work.nonceCount) * devices;
    std::cout << "CUDA benchmark: " << hashes / (ms / 1000.0) / 1e6 << " MH/s across " << devices << " devices" << std::endl;
}

} // namespace miner

int main()
{
    try {
        miner::Benchmark();
    } catch (const std::exception& e) {
        std::cerr << "CUDA miner error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

