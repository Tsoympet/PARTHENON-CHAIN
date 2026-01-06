# DRACHMA Quickstart Guide

This guide provides a quick introduction to using DRACHMA blockchain software.

## For New Users

### Installation

Choose one of the following methods to install DRACHMA:

**Option 1: Use Pre-built Binaries (Recommended)**
- Download from [GitHub Releases](https://github.com/Tsoympet/BlockChainDrachma/releases)
- Verify SHA-256 checksums and GPG signatures
- Extract and run

**Option 2: Build from Source**
- See [Building from Source](building.md) for detailed instructions
- Use the convenient Makefile: `make && sudo make install`

### First Run

1. **Start a testnet node:**
   ```bash
   ./drachmad --datadir ~/.drachma --network testnet --listen --rpcuser=user --rpcpassword=pass
   ```

2. **Launch the desktop wallet:**
   ```bash
   ./drachma-wallet --connect 127.0.0.1:9333
   ```

3. **Query your balance:**
   ```bash
   ./drachma-cli -rpcuser=user -rpcpassword=pass getbalance
   ```

## Desktop Wallet Guide

### Security First

- **Wallet Encryption**: Your wallet file is AES-256 encrypted. Always set a strong passphrase and keep offline backups.
- **Recovery Seed**: Open **Settings → Show recovery seed** to generate a BIP39 mnemonic. Write the 12 words on paper and store them offline. Use **Restore from seed** to rebuild the hot wallet on a new machine.
- **Hardware Wallets**: Click **Detect hardware wallet** to verify Ledger/Trezor visibility (requires `libhidapi`). Keep devices unlocked and connected via USB.

### Basic Operations

- **Receiving Funds**: On the **Receive** tab, generate a new address and share the QR code
- **Sending Funds**: On the **Send** tab, enter recipient address and amount. Use **Scan QR** to pull a recipient from an image or webcam
- **Transaction History**: View all transactions in the **Transactions** tab with filtering options
- **Fees**: Adjust the fee slider or use custom sat/vB to target confirmation speed

### Network and Sync

- **Network Health**: Watch the sync bar and RPC indicator on Overview. Any RPC degradation triggers a blocking alert so you can retry or reconfigure connectivity.
- **Sync Status**: The wallet shows current block height and synchronization progress

### Internationalization

The app attempts to load `assets/i18n/drachma_<locale>.qm`; place translations there and restart.

## Next Steps

- **Mining**: See [Mining Guide](../user-guides/mining-guide.md) to start mining
- **Deployment**: For production deployments, see [Deployment Guide](../operators/deployment.md)
- **Development**: For API documentation, see [Developer Guides](../developer-guides/)

## Getting Help

- **GitHub Discussions**: [https://github.com/Tsoympet/BlockChainDrachma/discussions](https://github.com/Tsoympet/BlockChainDrachma/discussions)
- **Matrix**: [https://matrix.to/#/#drachma:matrix.org](https://matrix.to/#/#drachma:matrix.org)
- **Discord**: [https://discord.gg/drachma](https://discord.gg/drachma)
