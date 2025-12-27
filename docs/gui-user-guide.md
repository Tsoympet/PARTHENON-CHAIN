# DRACHMA Core Desktop User Guide (Layer3 GUI)

## Quick start
1. Launch the app with `-DDRACHMA_BUILD_GUI=ON` during CMake configuration and run the `drachma_qt` binary.
2. On first run, accept the EULA. Configure your network (mainnet/testnet), RPC credentials, and data directory in **Settings**.
3. If prompted, unlock or restore your encrypted wallet. Use **Backup wallet** to keep an offline copy.
4. The **Overview** tab shows node sync, peer count, and RPC connectivity. Mining controls and hashrate live in **Mining**.

## Wallet safety
- Encrypt the wallet from **Settings → Wallet encryption**; a passphrase of at least 8 characters is required.
- Use **Backup wallet** to export an encrypted `wallet.dat` copy, and **Restore wallet** to import it on another machine.
- Unlock only when sending; the app blocks spending from a locked wallet.

## Sending and receiving
- **Receive** generates fresh addresses and displays a QR placeholder.
- **Send** supports address-book lookups and fee control. Choose Economy/Normal/Priority or a custom sat/vB rate; the estimated fee is shown before broadcast.
- Transactions appear in **Transactions** with timestamp, direction, confirmations, and status.

## Address book
Maintain labels and destinations in **Address book**. Copy entries into the send form or clipboard with a single click.

## Cross-platform and theming
The Qt-based UI ships Fusion-themed Light/Dark options for Windows, macOS, and Linux. Theme and settings persist between sessions.
