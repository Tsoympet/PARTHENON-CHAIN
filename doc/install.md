# Installing PARTHENON CHAIN

This guide covers how to install and run PARTHENON CHAIN (Drachma) core client binaries.

## Table of Contents

- [System Requirements](#system-requirements)
- [Quick Install](#quick-install)
- [Downloading Binaries](#downloading-binaries)
- [Verifying Downloads](#verifying-downloads)
- [Installation](#installation)
- [First Run](#first-run)
- [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+), Windows 10+, or macOS 11+
- **CPU**: 2 cores, x86_64 architecture
- **RAM**: 4 GB
- **Disk**: 100 GB free space (for full node)
- **Network**: Broadband internet connection

### Recommended Requirements
- **CPU**: 4+ cores
- **RAM**: 8+ GB
- **Disk**: 500 GB SSD
- **Network**: Unlimited data plan

## Quick Install

### Linux

```bash
# Download the latest release
wget https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-linux-x86_64.tar.gz

# Verify checksum (recommended)
wget https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256
sha256sum -c parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256

# Extract
tar -xzf parthenon-core-v0.1.0-linux-x86_64.tar.gz

# Install to /usr/local/bin (optional)
sudo install -m 755 drachmad drachma-cli drachma_cpu_miner /usr/local/bin/

# Run
drachmad --help
```

### macOS

```bash
# Download the appropriate version for your Mac
# For Intel Macs (x86_64):
curl -LO https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-macos-x86_64.tar.gz

# For Apple Silicon Macs (arm64):
curl -LO https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-macos-arm64.tar.gz

# Verify checksum
curl -LO https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-macos-arm64.tar.gz.sha256
shasum -a 256 -c parthenon-core-v0.1.0-macos-arm64.tar.gz.sha256

# Extract
tar -xzf parthenon-core-v0.1.0-macos-arm64.tar.gz

# Install to /usr/local/bin
sudo install -m 755 drachmad drachma-cli drachma_cpu_miner /usr/local/bin/

# You may need to approve the binaries in System Preferences > Security & Privacy
drachmad --help
```

### Windows

**Option 1: Using the Windows Installer (Recommended)**

1. Download the Windows installer from the [Releases page](https://github.com/Tsoympet/PARTHENON-CHAIN/releases)
   - Look for `parthenon-chain-vX.Y.Z-Windows-x86_64.exe` (NSIS installer)
2. Run the installer and follow the installation wizard
3. The installer will:
   - Install all core binaries to `C:\Program Files\ParthenonChain\`
   - Add shortcuts to the Start Menu
   - Optionally add binaries to PATH

**Option 2: Manual Installation from ZIP**

```powershell
# Download the ZIP archive from GitHub releases page
# https://github.com/Tsoympet/PARTHENON-CHAIN/releases
# Look for parthenon-core-vX.Y.Z-win-x86_64.zip

# Extract the ZIP file to a folder (e.g., C:\parthenon-chain)

# Run from Command Prompt or PowerShell
cd C:\parthenon-chain
.\drachmad.exe --help
```

**Note:** Windows binaries may trigger SmartScreen warnings. Click "More info" and "Run anyway" if you trust the source.

## Downloading Binaries

### From GitHub Releases

1. Visit https://github.com/Tsoympet/PARTHENON-CHAIN/releases
2. Find the latest stable release (not marked as "pre-release")
3. Download the appropriate package for your platform:
   
   **Core Binaries:**
   - Linux: `parthenon-core-vX.Y.Z-linux-x86_64.tar.gz`
   - Windows ZIP: `parthenon-core-vX.Y.Z-win-x86_64.zip`
   - Windows Installer: `parthenon-chain-vX.Y.Z-Windows-x86_64.exe`
   - macOS Intel: `parthenon-core-vX.Y.Z-macos-x86_64.tar.gz`
   - macOS Apple Silicon: `parthenon-core-vX.Y.Z-macos-arm64.tar.gz`
   
   **Qt Desktop Wallet (GUI):**
   - Windows: `parthenon-chain-vX.Y.Z-gui.exe` (NSIS installer)
   - macOS: `parthenon-chain-vX.Y.Z.dmg`
   - Linux Debian/Ubuntu: `parthenon-chain-qt_vX.Y.Z_amd64.deb`
   - Linux Fedora/RHEL: `parthenon-chain-qt-vX.Y.Z.x86_64.rpm`
   
   **Mobile Wallet:**
   - Android: `drachma-mobile-vX.Y.Z.apk`
   - iOS: Available through TestFlight (contact developers)
   
   **Blockchain Explorer:**
   - Standalone Linux: `explorer-vX.Y.Z-linux-x86_64.tar.gz`
   - Docker Image: `drachma-explorer-vX.Y.Z.tar`

4. Also download the corresponding `.sha256` checksum file for verification

### What's Included

Each release archive contains:
- **drachmad** (or drachmad.exe): The core daemon - runs a full node
- **drachma-cli** (or drachma-cli.exe): Command-line RPC client
- **drachma_cpu_miner** (or drachma_cpu_miner.exe): CPU mining software
- **README.txt**: Quick start guide
- **LICENSE**: Software license
- **VERSION**: Version information
- **SHA256SUMS**: Checksums for included binaries

## Verifying Downloads

**Always verify your downloads before running them.** This ensures you're running authentic, unmodified software.

See [Verifying Downloads](./verifying-downloads.md) for detailed instructions.

Quick verification:

```bash
# Linux/macOS
sha256sum -c parthenon-core-vX.Y.Z-linux-x86_64.tar.gz.sha256

# Windows (PowerShell)
(Get-FileHash parthenon-core-vX.Y.Z-win-x86_64.zip -Algorithm SHA256).Hash -eq (Get-Content parthenon-core-vX.Y.Z-win-x86_64.zip.sha256).Split()[0]
```

## Installation

### Option 1: Run from extracted directory

No installation required. Just extract and run:

```bash
./drachmad --datadir=/path/to/data
```

### Option 2: Install system-wide (Linux/macOS)

```bash
# Copy binaries to PATH
sudo install -m 755 drachmad /usr/local/bin/
sudo install -m 755 drachma-cli /usr/local/bin/
sudo install -m 755 drachma_cpu_miner /usr/local/bin/

# Verify installation
which drachmad
drachmad --version
```

### Option 3: Add to PATH (Windows)

1. Extract binaries to a permanent location (e.g., `C:\Program Files\PARTHENON-CHAIN`)
2. Add that directory to your system PATH:
   - Open System Properties > Environment Variables
   - Edit the "Path" variable
   - Add `C:\Program Files\PARTHENON-CHAIN`
   - Click OK and restart your terminal

## First Run

### Starting the daemon

```bash
# Start with default settings
drachmad

# Start on testnet
drachmad --network=testnet

# Specify custom data directory
drachmad --datadir=/mnt/blockchain/drachma

# Start with custom RPC credentials
drachmad --rpcuser=myuser --rpcpassword=mypassword
```

The daemon will:
1. Create a data directory at `~/.drachma` (or your specified path)
2. Initialize the blockchain database
3. Start syncing blocks from the network
4. Listen for P2P connections on port 9333
5. Start RPC server on port 8332

### Using the CLI

Once the daemon is running, use the CLI to interact with it:

```bash
# Get node info
drachma-cli getblockcount

# Get wallet balance
drachma-cli getbalance

# List RPC commands
drachma-cli help

# Use with custom RPC settings
drachma-cli -rpcuser=myuser -rpcpassword=mypass getinfo
```

### Initial Sync

The first time you run `drachmad`, it will download the entire blockchain. This may take several hours to days depending on your connection and hardware.

Monitor sync progress:

```bash
# Check current block height
drachma-cli getblockcount

# Compare with network height (view in block explorer)
# When they match, you're fully synced
```

## Configuration File

For persistent settings, create a configuration file:

**Linux/macOS:** `~/.drachma/drachma.conf`
**Windows:** `%APPDATA%\Drachma\drachma.conf`

Example configuration:

```ini
# Network
network=mainnet
port=9333

# RPC
rpcuser=yourusername
rpcpassword=yourpassword
rpcport=8332

# Data directory (optional)
datadir=/mnt/blockchain/drachma

# Connections
maxconnections=125
```

## Troubleshooting

### Port already in use

If you see "Address already in use" errors, either:
- Stop any other instance of drachmad
- Use different ports: `--port=9334 --rpcport=8333`

### Slow sync

- Ensure you have a stable internet connection
- Check disk speed (SSD recommended)
- Increase connection limit: `--maxconnections=200`

### Connection refused (CLI)

- Make sure `drachmad` is running
- Check RPC credentials match
- Verify firewall isn't blocking port 8332

### Out of disk space

The blockchain grows over time. Ensure you have sufficient space:
- Check space: `df -h`
- Use external drive: `--datadir=/mnt/external/drachma`
- Prune old blocks (if supported): `--prune=550`

### Permission denied

- On Linux/macOS, you may need to make binaries executable:
  ```bash
  chmod +x drachmad drachma-cli drachma_cpu_miner
  ```

## Upgrading

To upgrade to a new version:

1. Stop the running daemon: `drachma-cli stop`
2. Backup your wallet (if you have one): `cp ~/.drachma/wallet.dat ~/wallet-backup.dat`
3. Download and verify the new release
4. Replace the old binaries with new ones
5. Restart: `drachmad`

**Important:** Always backup your wallet before upgrading!

## Next Steps

- [Running a Node](./running-a-node.md) - Advanced configuration
- [Mining Guide](./mining-guide.md) - Solo or pool mining
- [Security Best Practices](../security/security-overview.md)
- [RPC API Reference](./rpc-reference.md)

## Support

- GitHub Issues: https://github.com/Tsoympet/PARTHENON-CHAIN/issues
- Discussions: https://github.com/Tsoympet/PARTHENON-CHAIN/discussions
- Documentation: https://github.com/Tsoympet/PARTHENON-CHAIN/tree/main/doc

## License

PARTHENON CHAIN is released under the MIT License. See LICENSE file for details.
