#!/usr/bin/env bash
# Gitian Build Environment Setup Script for PARTHENON CHAIN
# This script sets up a complete Gitian build environment

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GITIAN_DIR="${HOME}/gitian-builder"
PARTHENON_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "======================================"
echo "PARTHENON CHAIN Gitian Setup"
echo "======================================"
echo ""

# Check system
echo "==> Checking system requirements..."
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Error: Gitian builds require Linux. Detected: $(uname -s)"
    exit 1
fi

# Check for required tools
REQUIRED_TOOLS="git ruby debootstrap"
MISSING_TOOLS=""
for tool in $REQUIRED_TOOLS; do
    if ! command -v "$tool" &> /dev/null; then
        MISSING_TOOLS="$MISSING_TOOLS $tool"
    fi
done

if [[ -n "$MISSING_TOOLS" ]]; then
    echo "Error: Missing required tools:$MISSING_TOOLS"
    echo ""
    echo "Install with:"
    echo "  sudo apt-get install git ruby apt-cacher-ng qemu-utils debootstrap \\"
    echo "      lxc python3-cheetah parted kpartx bridge-utils make \\"
    echo "      ubuntu-archive-keyring curl firewalld apparmor"
    exit 1
fi

echo "✓ System requirements met"

# Clone Gitian Builder
if [[ ! -d "${GITIAN_DIR}" ]]; then
    echo ""
    echo "==> Cloning Gitian Builder..."
    git clone https://github.com/devrandom/gitian-builder.git "${GITIAN_DIR}"
    echo "✓ Gitian Builder cloned to ${GITIAN_DIR}"
else
    echo "✓ Gitian Builder already exists at ${GITIAN_DIR}"
fi

# Create base VM
BASE_VM="${GITIAN_DIR}/base-focal-amd64.tar.gz"
if [[ ! -f "${BASE_VM}" ]]; then
    echo ""
    echo "==> Creating base VM (this may take 10-20 minutes)..."
    cd "${GITIAN_DIR}"
    
    # Ensure scripts are executable
    chmod +x bin/*
    
    # Create base VM
    bin/make-base-vm --suite focal --arch amd64
    
    if [[ -f "${BASE_VM}" ]]; then
        echo "✓ Base VM created: ${BASE_VM}"
    else
        echo "Error: Failed to create base VM"
        exit 1
    fi
else
    echo "✓ Base VM already exists: ${BASE_VM}"
fi

# Create inputs directory for dependencies
INPUTS_DIR="${GITIAN_DIR}/inputs"
mkdir -p "${INPUTS_DIR}"

echo ""
echo "==> Gitian setup complete!"
echo ""
echo "Next steps:"
echo "  1. Build PARTHENON CHAIN binaries:"
echo "     cd ${GITIAN_DIR}"
echo '     ./bin/gbuild --commit parthenon-chain=${VERSION} \'
echo "       ${PARTHENON_DIR}/contrib/gitian-descriptors/gitian-linux.yml"
echo ""
echo "  2. Sign the build (optional):"
echo '     ./bin/gsign --signer "Your Name" --release ${VERSION}-linux \'
echo "       ${PARTHENON_DIR}/contrib/gitian-descriptors/gitian-linux.yml"
echo ""
echo "For detailed instructions, see:"
echo "  ${PARTHENON_DIR}/contrib/gitian-descriptors/README.md"
echo ""
echo "======================================"
