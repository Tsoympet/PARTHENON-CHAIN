#!/usr/bin/env bash
# DRACHMA macOS Installation Script
# Mirrors Bitcoin Core's installation flow
set -euo pipefail

PREFIX=${PREFIX:-/usr/local}
BUILD_TYPE=${BUILD_TYPE:-Release}

echo "=== DRACHMA Blockchain Installation (macOS) ==="
echo "Installation prefix: ${PREFIX}"
echo "Build type: ${BUILD_TYPE}"
echo ""

# Check for required dependencies
echo "Checking dependencies..."
missing_deps=()
for cmd in cmake; do
    if ! command -v $cmd &> /dev/null; then
        missing_deps+=($cmd)
    fi
done

if [ ${#missing_deps[@]} -ne 0 ]; then
    echo "ERROR: Missing required dependencies: ${missing_deps[*]}"
    echo "Please install them using Homebrew:"
    echo "  brew install cmake"
    exit 1
fi

# Build using Makefile wrapper
echo "Building DRACHMA binaries..."
make PREFIX="${PREFIX}" CMAKE_BUILD_TYPE="${BUILD_TYPE}" all

# Install using CMake install target
echo "Installing DRACHMA to ${PREFIX}..."
make PREFIX="${PREFIX}" install

echo ""
echo "=== Installation Complete ==="
echo ""
echo "Installed binaries:"
echo "  - drachmad      : DRACHMA daemon"
echo "  - drachma-cli   : RPC client"
echo "  - drachma-cpuminer : CPU miner"
if [ -f "${PREFIX}/bin/drachma-qt" ]; then
    echo "  - drachma-qt    : Desktop wallet"
fi
echo ""
echo "Man pages installed to ${PREFIX}/share/man/man1/"
echo "Documentation installed to ${PREFIX}/share/doc/drachma/"
echo ""
echo "To start the daemon:"
echo "  ${PREFIX}/bin/drachmad --network=testnet"
echo ""
echo "For help:"
echo "  ${PREFIX}/bin/drachmad --help"
echo "  man drachmad"

