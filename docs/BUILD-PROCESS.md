# DRACHMA Build Process Documentation

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Purpose:** Document the build process for reproducibility and transparency

This document provides comprehensive information about building DRACHMA from source, including detailed steps for deterministic builds.

---

## Table of Contents

1. [Build Requirements](#build-requirements)
2. [Quick Build](#quick-build)
3. [Platform-Specific Builds](#platform-specific-builds)
4. [Build Configuration](#build-configuration)
5. [Dependency Management](#dependency-management)
6. [Reproducible Builds](#reproducible-builds)
7. [Verification](#verification)

---

## Build Requirements

### Minimum Requirements

- **CMake:** 3.15 or higher
- **Compiler:** 
  - GCC 9.0+ (Linux)
  - Clang 10.0+ (macOS/Linux)
  - MSVC 2019+ (Windows)
- **C++ Standard:** C++17 or higher
- **Memory:** 4GB RAM minimum (8GB recommended)
- **Disk Space:** 2GB for build artifacts

### Dependencies

**Required:**
- Boost 1.65+
- OpenSSL 1.1.1+
- LevelDB (or compatible)

**Optional:**
- Qt 5.12+ (for GUI)
- ZMQ (for notification support)
- miniupnpc (for UPnP support)

See `INSTALL.md` for platform-specific dependency installation.

---

## Quick Build

### Linux/macOS

```bash
# Clone repository
git clone https://github.com/Tsoympet/BlockChainDrachma.git
cd BlockChainDrachma

# Build with Makefile wrapper
make

# Run tests
make test

# Install (optional)
sudo make install
```

### Windows

```powershell
# Clone repository
git clone https://github.com/Tsoympet/BlockChainDrachma.git
cd BlockChainDrachma

# Build
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# Run tests
ctest -C Release
```

---

## Platform-Specific Builds

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  libboost-all-dev \
  libssl-dev \
  libleveldb-dev \
  libzmq3-dev \
  libminiupnpc-dev

# Qt for GUI (optional)
sudo apt-get install -y \
  qtbase5-dev \
  qttools5-dev \
  qttools5-dev-tools

# Build
cd BlockChainDrachma
make
make test
```

### Fedora/RHEL/CentOS

```bash
# Install dependencies
sudo dnf install -y \
  gcc-c++ \
  cmake \
  boost-devel \
  openssl-devel \
  leveldb-devel \
  zeromq-devel \
  miniupnpc-devel

# Qt for GUI (optional)
sudo dnf install -y \
  qt5-qtbase-devel \
  qt5-qttools-devel

# Build
cd BlockChainDrachma
make
make test
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S --needed \
  base-devel \
  cmake \
  boost \
  openssl \
  leveldb \
  zeromq \
  miniupnpc

# Qt for GUI (optional)
sudo pacman -S qt5-base qt5-tools

# Build
cd BlockChainDrachma
make
make test
```

### macOS

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake boost openssl@1.1 leveldb zeromq miniupnpc

# Qt for GUI (optional)
brew install qt@5

# Set Qt path for CMake
export Qt5_DIR=/usr/local/opt/qt@5/lib/cmake/Qt5

# Build
cd BlockChainDrachma
make
make test
```

### Windows (MSVC)

```powershell
# Install vcpkg (package manager)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install boost:x64-windows openssl:x64-windows leveldb:x64-windows

# Qt for GUI (optional)
.\vcpkg install qt5:x64-windows

# Build DRACHMA
cd ..\BlockChainDrachma
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=..\..\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
ctest -C Release
```

---

## Build Configuration

### CMake Options

Common CMake configuration options:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_GUI=ON \
  -DBUILD_TESTS=ON \
  -DENABLE_WALLET=ON \
  -DENABLE_ZMQ=ON \
  -DENABLE_UPNP=ON
```

### Available Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Debug/Release/RelWithDebInfo) | Release |
| `CMAKE_INSTALL_PREFIX` | Installation directory | /usr/local |
| `BUILD_GUI` | Build Qt GUI wallet | ON |
| `BUILD_TESTS` | Build test suite | ON |
| `ENABLE_WALLET` | Enable wallet functionality | ON |
| `ENABLE_ZMQ` | Enable ZMQ notifications | ON |
| `ENABLE_UPNP` | Enable UPnP support | ON |
| `ENABLE_HARDENING` | Enable security hardening flags | ON |

### Build Types

**Release (default):**
- Optimizations enabled
- No debug symbols
- Fastest runtime performance
- Recommended for production

**Debug:**
- No optimizations
- Full debug symbols
- Slower but easier to debug
- For development only

**RelWithDebInfo:**
- Optimizations enabled
- Debug symbols included
- Good for profiling

Example:

```bash
# Debug build
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release with debug info
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-rel
```

---

## Dependency Management

### Pinned Versions

For reproducible builds, we pin specific dependency versions:

```cmake
# CMakeLists.txt (excerpt)
find_package(Boost 1.71.0 REQUIRED COMPONENTS system filesystem thread)
find_package(OpenSSL 1.1.1 REQUIRED)
```

Current pinned versions:
- Boost: 1.71.0+
- OpenSSL: 1.1.1+
- Qt: 5.12.0+ (if GUI enabled)

### Bundled Dependencies

Some dependencies are bundled in `contrib/`:
- secp256k1 (Schnorr signature library)
- LevelDB (may be system or bundled)

### Static vs. Dynamic Linking

**Default:** Dynamic linking (shared libraries)

**Static builds:**

```bash
cmake -S . -B build-static \
  -DBUILD_STATIC=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-static
```

Benefits of static builds:
- Single binary, no external dependencies
- Easier distribution
- Reproducible across systems

Drawbacks:
- Larger binary size
- Cannot benefit from system library updates

---

## Reproducible Builds

### Why Reproducible Builds?

Users must verify that distributed binaries match the audited source code. Reproducible builds ensure:
- No backdoors in binaries
- Build matches reviewed code
- Multiple builders get identical results

### Current Status

**Status:** In development (LAUNCH-ACTION-ITEMS.md #4)

Reproducible build infrastructure using Gitian or similar is planned before mainnet launch.

### Build Environment

For deterministic builds:

1. **Fixed compiler version:**
   ```bash
   gcc --version  # Must match release environment
   ```

2. **Fixed dependency versions:**
   - Use pinned versions (see above)
   - Avoid auto-updating package managers

3. **Consistent build flags:**
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_C_FLAGS="-O2 -g" \
     -DCMAKE_CXX_FLAGS="-O2 -g"
   ```

4. **Timestamps:**
   - Set `SOURCE_DATE_EPOCH` for reproducibility
   ```bash
   export SOURCE_DATE_EPOCH=1640995200
   ```

### Verifying Your Build

```bash
# Build from clean state
git clean -dfx
make distclean
make

# Generate checksums
sha256sum build/drachma-node > my-build.sha256
sha256sum build/drachma-cli >> my-build.sha256

# Compare with official release
wget https://github.com/Tsoympet/BlockChainDrachma/releases/download/v0.1.0/SHA256SUMS
diff my-build.sha256 SHA256SUMS
```

### Docker-based Reproducible Build

```dockerfile
# Dockerfile.reproducible (example)
FROM ubuntu:20.04

# Fixed package versions
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential=12.8ubuntu1.1 \
    cmake=3.16.3-1ubuntu1 \
    libboost-all-dev=1.71.0.0ubuntu2 \
    libssl-dev=1.1.1f-1ubuntu2

# Build
COPY . /src
WORKDIR /src
RUN make

# Extract binaries
RUN mkdir /output && \
    cp build/drachma-node /output/ && \
    cp build/drachma-cli /output/
```

Build:

```bash
docker build -f Dockerfile.reproducible -t drachma-reproducible .
docker run --rm -v $(pwd)/output:/output drachma-reproducible
```

---

## Verification

### Build Verification Script

Use the provided verification script:

```bash
./scripts/verify-build-test.sh
```

This checks:
- Dependencies installed
- Build completes successfully
- All tests pass
- Supply constants correct

### Manual Verification Steps

1. **Verify dependencies:**
   ```bash
   cmake --version
   g++ --version
   pkg-config --modversion openssl
   ```

2. **Clean build:**
   ```bash
   make distclean
   make
   ```

3. **Run tests:**
   ```bash
   make test
   # Or verbose:
   ctest --test-dir build --output-on-failure --verbose
   ```

4. **Check binaries:**
   ```bash
   ls -lh build/drachma-node build/drachma-cli
   ldd build/drachma-node  # Check linked libraries (Linux)
   otool -L build/drachma-node  # macOS
   ```

5. **Verify supply constants:**
   ```bash
   ./scripts/verify-genesis.sh
   ```

6. **Test basic functionality:**
   ```bash
   build/drachma-node --version
   build/drachma-node --testnet &
   sleep 5
   build/drachma-cli --testnet getblockchaininfo
   build/drachma-cli --testnet stop
   ```

---

## Build Artifacts

### Output Locations

After successful build:

```
build/
├── drachma-node          # Node daemon
├── drachma-cli           # Command-line interface
├── drachma-tx            # Transaction utility
├── drachma-wallet        # Wallet utility
├── drachma-qt            # GUI wallet (if BUILD_GUI=ON)
├── test/
│   └── *_test           # Test executables
└── lib/
    └── *.a/*.so         # Libraries
```

### Installing

```bash
# System-wide installation
sudo make install

# Default locations:
# - /usr/local/bin/drachma-*
# - /usr/local/lib/libdrachma*
# - /usr/local/share/drachma/

# Custom prefix
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ...
make install
```

### Uninstalling

```bash
# If install manifest exists
sudo make uninstall

# Or manual removal
sudo rm /usr/local/bin/drachma-*
sudo rm -rf /usr/local/share/drachma/
```

---

## Troubleshooting Build Issues

See `docs/TROUBLESHOOTING.md` for detailed build troubleshooting.

**Common issues:**
- Missing dependencies → install required packages
- CMake version too old → upgrade CMake
- Compiler errors → check GCC/Clang version
- Test failures → check for known issues

---

## CI/CD Integration

### GitHub Actions

DRACHMA uses GitHub Actions for continuous integration:

`.github/workflows/ci.yml` - Builds and tests on:
- Ubuntu 20.04, 22.04
- macOS latest
- Windows latest

See workflow file for exact build commands used in CI.

### Local CI Simulation

Run the same steps as CI locally:

```bash
# Ubuntu
docker run -it ubuntu:20.04
apt-get update && apt-get install -y git
# ... (follow CI steps)

# Or use act (GitHub Actions locally)
act -j build
```

---

## Build Best Practices

1. **Always start clean:**
   ```bash
   make distclean
   ```

2. **Use parallel builds:**
   ```bash
   cmake --build build --parallel $(nproc)
   ```

3. **Test before install:**
   ```bash
   make test
   ```

4. **Document your environment:**
   ```bash
   cmake --version > build-env.txt
   g++ --version >> build-env.txt
   lsb_release -a >> build-env.txt  # Linux
   ```

5. **Keep build separate from source:**
   - Never build in source directory
   - Use `build/` or similar

---

## Getting Help

**Build issues?**

1. Check `INSTALL.md`
2. Review `docs/TROUBLESHOOTING.md`
3. Search GitHub Issues
4. Ask in community channels with:
   - Your OS and version
   - Compiler version
   - Error messages
   - Steps to reproduce

---

## Quick Reference

**Standard build:**
```bash
make
make test
sudo make install
```

**Clean rebuild:**
```bash
make distclean
make
```

**Custom build:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF
cmake --build build --parallel
ctest --test-dir build
```

---

**Last Updated:** January 6, 2026

For the latest build instructions, always refer to the repository's main branch.
