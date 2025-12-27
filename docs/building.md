# Building DRACHMA

This guide describes how to build the DRACHMA reference stack across platforms. DRACHMA uses CMake and a C++17 toolchain; deterministic builds are encouraged for releases.

## Prerequisites

### Toolchain
- **CMake:** >= 3.22
- **C++ Compiler:** GCC >= 11 or Clang >= 13 with full C++17 support
- **Python:** >= 3.9 for scripts and tests
- **Git:** for source retrieval

### Libraries
- **OpenSSL** (crypto, RPC/TLS)
- **Boost** (filesystem, program_options, asio, serialization)
- **SQLite** (wallet/indexing) and/or **LevelDB** (chainstate) depending on build options
- **Zlib** (compression for network and storage)
- **cURL** (optional, for some service integrations)

### GPU (optional)
- **CUDA Toolkit** >= 11.7 for NVIDIA miners
- **OpenCL SDK** (vendor specific) for AMD/Intel miners
- Matching vendor drivers with development headers.

### Platform-specific packages
- **Debian/Ubuntu:** `sudo apt install build-essential cmake pkg-config libssl-dev libboost-all-dev libsqlite3-dev libleveldb-dev libz-dev python3 python3-pip ocl-icd-opencl-dev`
- **Fedora/RHEL:** `sudo dnf install gcc gcc-c++ cmake openssl-devel boost-devel sqlite-devel leveldb-devel zlib-devel python3 ocl-icd-devel`
- **macOS (Homebrew):** `brew install cmake openssl boost sqlite leveldb zlib python` (set `OPENSSL_ROOT_DIR` if needed)
- **Windows (MSVC + vcpkg):** Install Visual Studio 2022, CMake, and acquire dependencies via `vcpkg install openssl boost sqlite3 leveldb zlib`.

## Configure and Build

1. Clone repository and initialize submodules if present:
   ```bash
   git clone https://github.com/Tsoympet/BlockChainDrachma.git
   cd BlockChainDrachma
   git submodule update --init --recursive
   ```
2. Configure with CMake:
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DDRACHMA_BUILD_TESTS=ON \
     -DDRACHMA_BUILD_GUI=ON \
     -DENABLE_WALLET=ON \
     -DENABLE_GPU_MINERS=ON
   ```
   Toggle options as needed (see below).
3. Build:
   ```bash
   cmake --build build --parallel
   ```
4. Run tests (if enabled):
   ```bash
   ctest --test-dir build --output-on-failure
   ```

Artifacts follow the repository layout under `build/` (e.g., `build/layer1-core/`, `build/layer2-services/`, `build/layer3-app/`, `build/miners/`).

## Notable CMake Options

- `CMAKE_BUILD_TYPE` = `Release` | `Debug` | `RelWithDebInfo`
- `DRACHMA_BUILD_TESTS` (ON/OFF): build unit/integration tests
- `DRACHMA_BUILD_GUI` (ON/OFF): build desktop client under `layer3-app`
- `ENABLE_WALLET` (ON/OFF): wallet/key management services
- `ENABLE_GPU_MINERS` (ON/OFF): CUDA/OpenCL miners
- `DRACHMA_BUILD_FUZZ` (ON/OFF): build fuzzing harnesses (developers only)
- `CUDA_TOOLKIT_ROOT_DIR` / `OpenCL_INCLUDE_DIR` / `OpenCL_LIBRARY`: override GPU paths
- `USE_SYSTEM_LIBS` (ON/OFF): prefer system dependencies over vendored

Pass options via `-D<OPTION>=<VALUE>` during configuration.

## Cross-Platform Notes

- **Linux:** Preferred for production nodes. Ensure `ulimit -n` is sufficient for P2P peers. Use `libstdc++` matching compiler version.
- **macOS:** Specify OpenSSL path: `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl)` when CMake cannot locate it.
- **Windows:** Configure from a “x64 Native Tools” shell. Use Ninja (`-G Ninja`) for faster builds. Set `-DOPENSSL_ROOT_DIR` to your vcpkg install path. Disable GPU miners if CUDA/OpenCL SDKs are absent.
- **Containers:** The provided `Dockerfile` and `docker-compose.yml` offer reproducible environments for CI and testing.

## Mainnet Build Profile

For production nodes and release artifacts:

- Configure in release mode with deterministic flags where available:
  ```bash
  cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DDRACHMA_BUILD_TESTS=OFF \
    -DDRACHMA_BUILD_GUI=OFF \
    -DDRACHMA_BUILD_FUZZ=OFF \
    -DENABLE_WALLET=ON \
    -DENABLE_GPU_MINERS=OFF
  ```
- Prefer system packages vetted by your distro; pin versions in CI to reproduce hashes.
- Use `-DCMAKE_INSTALL_PREFIX=/opt/drachma` and `cmake --install build` for managed deployments.
- Sign resulting binaries and publish SHA-256 checksums. Verify signatures before promotion to production hosts.

## GPU Build Tips

- For CUDA, ensure `nvcc --version` matches the driver; set `-DCMAKE_CUDA_ARCHITECTURES` for target GPUs.
- For OpenCL, install vendor SDK and ICD loader; verify with `clinfo`.
- Disable GPU miners on systems without compatible hardware to shorten build times.

## Troubleshooting

- **Missing headers/libraries:** Inspect `CMakeError.log` under `build/CMakeFiles/`. Provide explicit paths via `-D<VAR>=...`.
- **Stale caches:** Remove the build directory when switching compilers or major options: `rm -rf build`.
- **Linker errors on macOS:** Add `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` or adjust to your SDK.
- **GPU issues:** Ensure driver/toolkit versions match; export `CUDA_HOME` or set `OpenCL` variables. Try `-DENABLE_GPU_MINERS=OFF` to isolate.
- **Reproducible builds:** Pin compiler versions, use container images, and avoid unpinned package upgrades.

## Build Targets

- **Layer 1 Core Daemon:** `layer1-core/drachmad`
- **Services Daemon:** `layer2-services/drachma-services`
- **Desktop App:** `layer3-app/drachma-wallet`
- **Reference Miners:** `miners/drachma-miner-cpu`, `miners/drachma-miner-gpu`

Install paths can be set via `-DCMAKE_INSTALL_PREFIX` followed by `cmake --install build`.
