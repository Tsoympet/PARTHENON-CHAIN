# Gitian Building Guide

Gitian is a deterministic build process used to ensure that binaries built by different people produce bit-for-bit identical results. This allows independent verification that release binaries match the published source code.

## Overview

Gitian builds run in isolated, deterministic environments (VMs or containers) with fixed:
- Operating system version
- Compiler versions
- Library versions
- Build timestamps
- File ordering

This ensures anyone can verify that official releases were built from the published source code without modifications.

## Why Gitian?

**Security:** Prevents backdoored compilers or build environments from compromising binaries.

**Trust:** Multiple independent builders can verify releases match the source.

**Transparency:** Build process is fully documented and reproducible.

## Prerequisites

### System Requirements

- Linux system (Ubuntu 20.04+ or Debian 11+ recommended)
- 20 GB free disk space
- 4 GB RAM
- Virtualization support (KVM/VirtualBox)

### Software

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y git ruby apt-cacher-ng qemu-utils debootstrap \
    lxc python3-cheetah parted kpartx bridge-utils make ubuntu-archive-keyring \
    curl firewalld apparmor
```

## Setup

### 1. Clone Gitian Builder

```bash
git clone https://github.com/devrandom/gitian-builder.git
cd gitian-builder
```

### 2. Setup Base VM

```bash
# Ubuntu (recommended)
bin/make-base-vm --suite focal --arch amd64

# Or Debian
bin/make-base-vm --suite bullseye --arch amd64
```

This creates `base-focal-amd64.tar.gz` which is reused for all builds.

### 3. Clone PARTHENON CHAIN

```bash
cd ..
git clone https://github.com/Tsoympet/PARTHENON-CHAIN.git
```

## Building

### Linux Build

```bash
cd gitian-builder

# Set version to build
export VERSION=v0.1.0

# Build
./bin/gbuild --commit parthenon-chain=${VERSION} \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml

# Sign result (optional, requires GPG key)
./bin/gsign --signer "Your Name" --release ${VERSION}-linux \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml

# Move result
mv build/out/parthenon-*.tar.gz ../
```

### Windows Build

**Note:** Windows cross-compilation requires downloading and preparing dependencies (OpenSSL, Boost, LevelDB) for MinGW. This is complex and time-consuming.

```bash
# Download Windows dependencies first
# (See gitian-win.yml for required files)

./bin/gbuild --commit parthenon-chain=${VERSION} \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-win.yml
```

### macOS Build

**Note:** macOS builds require the macOS SDK, which can only be obtained from Xcode (requires Apple Developer account). For this reason, macOS builds are typically done on actual macOS hardware via GitHub Actions.

```bash
# Requires MacOSX11.0.sdk.tar.gz
./bin/gbuild --commit parthenon-chain=${VERSION} \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-osx.yml
```

## Verifying Results

### Check Output

```bash
# Linux
ls -lh gitian-builder/build/out/parthenon-core-*.tar.gz

# View build log
cat gitian-builder/var/build.log
```

### Compare Signatures

Multiple builders should produce identical hashes:

```bash
# Your build's hash
sha256sum parthenon-core-v0.1.0-linux-x86_64.tar.gz

# Compare with official release
curl -L https://github.com/Tsoympet/PARTHENON-CHAIN/releases/download/v0.1.0/parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256

# Compare with other builders' signatures
cat gitian.sigs/v0.1.0-linux/*/parthenon-core-*.assert
```

If all hashes match, the build is verified as reproducible!

## Signing Builds

If you're an established contributor, you can sign builds to help verify releases:

### 1. Generate GPG Key (if needed)

```bash
gpg --full-generate-key
# Select RSA, 4096 bits
# Use your real name and email
```

### 2. Sign the Build

```bash
cd gitian-builder
./bin/gsign --signer "Your Name <your@email.com>" \
    --release ${VERSION}-linux \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml
```

### 3. Submit Signatures

```bash
# Fork the gitian.sigs repository
git clone https://github.com/parthenon-chain/gitian.sigs.git
cd gitian.sigs

# Copy your signature
mkdir -p ${VERSION}-linux/yourname
cp ../gitian-builder/sigs/${VERSION}-linux/yourname/* ${VERSION}-linux/yourname/

# Commit and submit PR
git add .
git commit -m "Add ${VERSION}-linux signatures for yourname"
git push

# Create pull request on GitHub
```

## Troubleshooting

### "command not found: gbuild"

Ensure you're in the `gitian-builder` directory and the script is executable:
```bash
chmod +x bin/gbuild bin/gsign
```

### Build fails with dependency errors

Check that all required packages are installed:
```bash
sudo apt-get install -y cmake ninja-build libssl-dev libboost-all-dev libleveldb-dev
```

### "No base image found"

Create the base VM:
```bash
bin/make-base-vm --suite focal --arch amd64
```

### Out of disk space

Gitian builds can use significant space. Ensure at least 20 GB free:
```bash
df -h
```

## Advanced Options

### Using LXC Instead of KVM

```bash
# Install LXC
sudo apt-get install lxc

# Build with LXC
./bin/gbuild --lxc --commit parthenon-chain=${VERSION} \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml
```

### Parallel Builds

```bash
# Use multiple cores
./bin/gbuild -j4 --commit parthenon-chain=${VERSION} \
    ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml
```

### Caching Dependencies

Enable caching to speed up subsequent builds:
```bash
export USE_VENV=1
export GITIAN_CACHE_DIR=~/gitian-cache
mkdir -p $GITIAN_CACHE_DIR
```

## Best Practices

1. **Verify the descriptor:**
   - Review `contrib/gitian-descriptors/*.yml` before building
   - Ensure it matches what others are using

2. **Build in clean environment:**
   - Fresh VM or container
   - No running processes that might affect build

3. **Document your environment:**
   - OS version
   - Gitian version
   - Any modifications made

4. **Compare with multiple builders:**
   - Don't trust a single build
   - Verify against community consensus

## Contributing Signatures

To become a trusted signer:

1. Build multiple releases successfully
2. Compare with official and other builders
3. Submit signatures via gitian.sigs repository
4. Participate in release verification discussions

## Resources

- [Gitian Builder](https://github.com/devrandom/gitian-builder)
- [Bitcoin's Gitian Documentation](https://github.com/bitcoin-core/docs/blob/master/gitian-building.md)
- [Reproducible Builds Project](https://reproducible-builds.org/)

## Support

- GitHub Issues: https://github.com/Tsoympet/PARTHENON-CHAIN/issues
- Discussions: https://github.com/Tsoympet/PARTHENON-CHAIN/discussions

## License

PARTHENON CHAIN is released under the MIT License. See LICENSE file for details.
