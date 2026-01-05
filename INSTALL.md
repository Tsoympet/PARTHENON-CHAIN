# DRACHMA Installation Guide

This document provides instructions for building and installing DRACHMA from source.

## Quick Start

### Linux / macOS

```bash
# Clone the repository
git clone https://github.com/Tsoympet/BlockChainDrachma.git
cd BlockChainDrachma

# Install using the convenience script
./scripts/install-linux.sh    # For Linux
./scripts/install-macos.sh     # For macOS

# Or use make directly
make
sudo make install
```

### Windows

```powershell
# Clone the repository
git clone https://github.com/Tsoympet/BlockChainDrachma.git
cd BlockChainDrachma

# Run the installation script
.\scripts\install-windows.ps1
```

## Detailed Instructions

### Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config git \
  libssl-dev libboost-all-dev libsqlite3-dev libleveldb-dev libz-dev \
  python3 python3-pip
```

#### macOS
```bash
brew install cmake openssl boost sqlite leveldb zlib python git
```

#### Windows
- Install Visual Studio 2022 with C++ workload
- Install CMake
- Install vcpkg for dependencies

### Building

The build system uses CMake with a Makefile wrapper for convenience.

#### Using Make (Linux/macOS)

```bash
# Build everything with default settings
make

# Customize the build
make PREFIX=/opt/drachma CMAKE_BUILD_TYPE=Release

# Install to the system
sudo make install

# Or install to a custom location (no sudo needed)
make PREFIX=$HOME/.local install
```

#### Using CMake directly

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
cmake --build build --parallel

# Install
sudo cmake --build build --target install
```

### Build Options

- `PREFIX` - Installation prefix (default: `/usr/local`)
- `CMAKE_BUILD_TYPE` - Build type: Release, Debug, RelWithDebInfo (default: Release)
- `BUILD_DIR` - Build directory (default: build)

CMake options:
- `DRACHMA_BUILD_TESTS` - Build test executables (default: ON)
- `DRACHMA_BUILD_GUI` - Build Qt desktop wallet (default: OFF)
- `DRACHMA_ENABLE_OPENCL` - Enable OpenCL GPU miner (default: OFF)
- `DRACHMA_COVERAGE` - Enable coverage instrumentation (default: OFF)

Example:
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DDRACHMA_BUILD_GUI=ON \
  -DDRACHMA_BUILD_TESTS=OFF
```

### What Gets Installed

The installation process will install:

#### Binaries (to `$PREFIX/bin`)
- `drachmad` - DRACHMA daemon (consensus node)
- `drachma-cli` - RPC command-line client
- `drachma-cpuminer` - CPU miner
- `drachma-qt` - Desktop wallet (if built with GUI enabled)
- GPU miners (if CUDA/OpenCL available)

#### Libraries (to `$PREFIX/lib`)
- `libdrachma_layer1.a` - Layer 1 consensus library
- `libdrachma_layer2.a` - Layer 2 services library

#### Headers (to `$PREFIX/include/drachma`)
- Development headers for building applications

#### Documentation (to `$PREFIX/share`)
- Man pages (`share/man/man1/`)
- Documentation files (`share/doc/drachma/`)
- Desktop integration file (`share/applications/` on Linux)

### Uninstalling

```bash
# Using make
sudo make uninstall

# Or using CMake directly
sudo xargs rm -f < build/install_manifest.txt
```

### Running Tests

```bash
# Using make
make test

# Or using CMake/CTest directly
ctest --test-dir build --output-on-failure
```

### Platform-Specific Notes

#### Linux
- Default installation requires root/sudo for system-wide install
- Use `PREFIX=$HOME/.local` for user-local installation
- Desktop file installed to `/usr/local/share/applications/`

#### macOS
- May need to set `OPENSSL_ROOT_DIR=$(brew --prefix openssl)`
- Qt apps can be bundled with macdeployqt (see docs/building.md)

#### Windows
- Use Developer Command Prompt or PowerShell
- Binaries installed to `C:\Program Files\Drachma\bin`
- Add to PATH manually or use the installer

## Getting Help

- For build issues, see `docs/building.md`
- For running the node, see `README.md`
- Report bugs: https://github.com/Tsoympet/BlockChainDrachma/issues

## See Also

- `README.md` - Project overview and quick start
- `docs/building.md` - Detailed build instructions
- `CONTRIBUTING.md` - Contribution guidelines
