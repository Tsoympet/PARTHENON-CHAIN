# PARTHENON CHAIN Miners (PC/Desktop)

This directory contains **PC/Desktop miners** for PARTHENON CHAIN (Drachma).

## Available PC Miners

### 1. CPU Miner
- **Location**: `cpu/main.cpp`
- **Optimization**: Multi-threaded with AVX2 SIMD instructions
- **Threads**: 4-32+ CPU cores
- **Batch size**: 1024-4096+ hashes per batch
- **Continuous operation**: No sleep intervals
- **Target**: x86_64 desktop/server systems

### 2. CUDA GPU Miner
- **Location**: `gpu-cuda/host.cpp`
- **Optimization**: NVIDIA CUDA GPU acceleration
- **Parallelism**: Millions of parallel hash operations
- **Target**: NVIDIA GPUs with CUDA support

### 3. OpenCL GPU Miner
- **Location**: `gpu-opencl/host.cpp`
- **Optimization**: OpenCL GPU acceleration
- **Parallelism**: Millions of parallel hash operations
- **Target**: AMD/Intel GPUs with OpenCL support

## Mobile Mining (Separate Implementation)

**IMPORTANT**: Mobile mining uses a **completely different implementation** located in:
```
/mobile-client/src/services/mining/MobileMiningService.ts
```

### Why Separate Implementations?

The PC and mobile miners are fundamentally different due to:

| Aspect | PC Miners (This Directory) | Mobile Miner |
|--------|---------------------------|--------------|
| **Language** | C++ | TypeScript (React Native) |
| **Threading** | Multi-threaded (4-32+ cores) | Single-threaded or minimal |
| **Batch Size** | 1024-4096+ hashes | 10-100 hashes |
| **Sleep Intervals** | None (continuous) | 100ms between batches |
| **Optimizations** | AVX2/CUDA/OpenCL | ARM processors |
| **Power Management** | None | Battery & temperature aware |
| **Platform** | x86_64 desktop/server | ARM mobile devices |
| **Hash Rate** | 1K-1M+ H/s | 5-200 H/s |

### DO NOT Mix Implementations

- **PC miners** (this directory) should NOT be used on mobile devices
- **Mobile miner** should NOT be used on PC/desktop systems
- Each implementation is optimized for its target platform
- Code is NOT shared between implementations

## Configuration

PC miners use `miner_config.example.json`:

```json
{
  "stratum_url": "stratum+tcp://pool.example.com:3333",
  "threads": 4,              // Multi-core CPU mining
  "intensity": 256,          // Large batches for PC
  "devices": [0, 1]          // GPU device selection
}
```

Mobile mining uses `mobile-client/mobile_miner_config.example.json`:

```json
{
  "hashBatchSize": 100,           // Small batches for mobile
  "sleepBetweenBatches": 100,     // Sleep to prevent overheating
  "enableOnCharging": true,       // Battery awareness
  "maxTemperature": 40            // Thermal protection
}
```

## Documentation

- **PC Miners**: See `/doc/MINING-GUIDE.md` and `/doc/miners/README.md`
- **Mobile Mining**: See `/mobile-client/MOBILE_MINING.md`

## Building PC Miners

```bash
# From repository root
make

# CPU miner (always built if Boost available)
./drachma_cpu_miner --help

# CUDA miner (if CUDA toolkit detected)
./drachma_cuda_miner --help

# OpenCL miner (requires explicit enable)
cmake -DDRACHMA_ENABLE_OPENCL=ON ..
make drachma_opencl_miner
```

Mobile mining is integrated into the React Native mobile app - no separate build needed.

## License

Same as parent PARTHENON CHAIN project (MIT).
