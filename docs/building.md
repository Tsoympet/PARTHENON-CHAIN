# Building DRACHMA

This guide walks through building the DRACHMA reference implementation (core node `drachmad`, desktop wallet `drachma-wallet`, and reference miners) on Linux, Windows, and macOS. Commands favor a Release build similar to the artifacts published on the GitHub Releases page.

## Prerequisites

### Core toolchain
- **CMake:** 3.22 or newer
- **C++17 compiler:** GCC ≥ 11, Clang ≥ 13, or MSVC 2022
- **Python:** 3.9+ for scripts/tests
- **Git:** to clone and fetch tags

### Required libraries
- **OpenSSL** (TLS/RPC)
- **Boost** (filesystem, program_options, asio)
- **SQLite / LevelDB** (depending on wallet/indexing options)
- **zlib** (compression)
- **cURL** (optional service integrations)

### GPU (optional)
- **CUDA Toolkit** ≥ 11.7 for NVIDIA miners
- **OpenCL SDK** for AMD/Intel miners
- Matching vendor drivers and ICD loaders

### Platform packages
- **Ubuntu/Debian:**
  ```bash
  sudo apt update
  sudo apt install build-essential cmake pkg-config git \
    libssl-dev libboost-all-dev libsqlite3-dev libleveldb-dev libz-dev \
    python3 python3-pip ocl-icd-opencl-dev
  ```
- **Fedora/RHEL:**
  ```bash
  sudo dnf install gcc gcc-c++ cmake openssl-devel boost-devel \
    sqlite-devel leveldb-devel zlib-devel python3 ocl-icd-devel git
  ```
- **macOS (Homebrew):**
  ```bash
  brew install cmake openssl boost sqlite leveldb zlib python git
  ```
  If OpenSSL is not found by CMake, add `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl)`.
- **Windows (MSVC + vcpkg):**
  - Install **Visual Studio 2022** with C++ workload and **CMake**.
  - Install dependencies with vcpkg (from a Developer PowerShell):
    ```powershell
    vcpkg install openssl boost sqlite3 leveldb zlib
    ```
  - Pass `-DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>/scripts/buildsystems/vcpkg.cmake"` to CMake.

## Directory layout
- `CMakeLists.txt` (root): builds **drachmad** and **miners**
- `layer3-app/CMakeLists.txt`: builds **drachma-wallet** (Qt desktop wallet)

## Build the core node and miners (all platforms)
```bash
git clone https://github.com/Tsoympet/BlockChainDrachma.git
cd BlockChainDrachma

# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build daemons, CLI, and miners
cmake --build build --parallel --target drachmad drachma_cli drachma_cpu_miner
```
Binaries appear under `build/layer1-core/` (node + cli) and `build/miners/`.

## Build the Qt desktop wallet
```bash
cd BlockChainDrachma
cmake -S layer3-app -B build-wallet -DCMAKE_BUILD_TYPE=Release
cmake --build build-wallet --parallel
```
The wallet binary lives under `build-wallet/` (for example `build-wallet/drachma-wallet`).

## Platform-specific notes and deployment helpers

### Linux
- Ensure `ulimit -n` is large enough for many peers.
- For the wallet, generate a relocatable bundle or AppImage using **linuxdeploy**:
  ```bash
  # Install linuxdeploy (example; choose the latest release)
  wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
  chmod +x linuxdeploy-x86_64.AppImage

  # Bundle the Qt wallet into an AppImage
  ./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    -e build-wallet/drachma-wallet \
    -d layer3-app/resources/drachma-wallet.desktop \
    -i layer3-app/resources/icons/drachma.png \
    --output appimage
  ```
- Tarball example for node + miners:
  ```bash
  tar -C build -czf drachma-vX.Y.Z-linux-x64.tar.gz layer1-core layer2-services miners
  ```

### Windows
- Configure from a **x64 Native Tools** shell. Prefer Ninja for speed: `-G Ninja`.
- After building the wallet, stage Qt dependencies with **windeployqt** (Qt 6 path may vary):
  ```powershell
  $qtDir = "C:\Qt\6.6.3\msvc2019_64\bin"
  & "$qtDir\windeployqt.exe" --release --qmldir layer3-app \
      build-wallet\drachma-wallet.exe
  ```
- Package the node and wallet separately (zip/installer). A simple zip example:
  ```powershell
  Compress-Archive -Path build\layer1-core\drachmad.exe,build\layer1-core\drachma-cli.exe,build\miners\* -DestinationPath drachma-vX.Y.Z-windows-x64.zip
  Compress-Archive -Path build-wallet\* -DestinationPath drachma-wallet-vX.Y.Z-windows-x64.zip
  ```

### macOS
- Use the latest Xcode command-line tools. Set `CMAKE_OSX_DEPLOYMENT_TARGET=12.0` if needed.
- Deploy Qt frameworks with **macdeployqt**:
  ```bash
  /Applications/Qt/6.6.3/macos/bin/macdeployqt \
    build-wallet/drachma-wallet.app -dmg
  ```
  This produces `drachma-wallet.dmg` alongside the `.app` bundle.
- Package the node binary separately:
  ```bash
  tar -C build/layer1-core -czf drachma-vX.Y.Z-macos-x64.tar.gz drachmad
  ```

## Running what you built
- **Run a testnet node:**
  ```bash
  ./build/layer1-core/drachmad --network testnet --datadir ~/.drachma-testnet --listen --rpcuser=user --rpcpassword=pass
  ```
- **Launch the wallet (connect to local node):**
  ```bash
  ./build-wallet/drachma-wallet --connect 127.0.0.1:9333
  ```
- **CPU miner example:**
  ```bash
  ./build/miners/cpu-miner/drachma-cpuminer --rpc http://user:pass@127.0.0.1:8332 --threads 4
  ```
- **GPU miner example (CUDA):**
  ```bash
  ./build/miners/gpu-miner/drachma-cuda --url 127.0.0.1:9333 --user user --pass pass --intensity 22
  ```

## Reproducibility tips
- Build from a tagged release and record the tag/commit, compiler, and dependency versions.
- Prefer pinned package versions or containerized builds (see `Dockerfile`, `docker-compose.yml`).
- Publish SHA-256 checksums for any distributed archives and verify them before running.
- Cross-check release builds between maintainers whenever possible.

## Common CMake options
- `CMAKE_BUILD_TYPE` = `Release` | `Debug` | `RelWithDebInfo`
- `DRACHMA_BUILD_TESTS` (ON/OFF): build tests
- `DRACHMA_BUILD_GUI` (ON/OFF): build the Qt desktop wallet
- `ENABLE_WALLET` (ON/OFF): wallet/key services
- `ENABLE_GPU_MINERS` (ON/OFF): CUDA/OpenCL miners
- `CMAKE_TOOLCHAIN_FILE`: point to vcpkg or custom toolchains
- `CUDA_TOOLKIT_ROOT_DIR`, `OpenCL_INCLUDE_DIR`, `OpenCL_LIBRARY`: override GPU SDK paths

Pass options via `-D<OPTION>=<VALUE>` during configuration. Remove the `build` directory when switching toolchains or major options to avoid stale caches.
