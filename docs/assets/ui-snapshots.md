# DRACHMA Wallet Demo Screens

The Qt wallet can emit lightweight markdown descriptions of its primary views without bundling binary screenshots. Use **Help → Demo Screens** in the running application to refresh this report.

## Dashboard and Sync Status
- Displays current network (mainnet/testnet/regtest), peer count, and latest block height.
- Shows synchronization progress with a progress bar and remaining block estimate.
- Recent transactions list highlights confirmations and mempool status.

## Send Flow with Custom Fee Selection
- Recipient address entry with QR decode support.
- Amount selector supporting decimal DRM amounts; warns on dust outputs.
- Fee slider with live estimation (slow/standard/fast) and manual sat/vByte override.
- Final review screen summarizing inputs/outputs and change before signing.

## Receive and QR Generation
- Fresh address creation and labeling for the address book.
- On-demand QR generation for the selected address with copy/share helpers.

## Backup and Security Notices
- Passphrase prompt on startup for encrypted wallets.
- Backup/export guidance and warnings before restoring a wallet file.

> The report is generated at runtime to ensure it reflects the currently compiled UI without requiring binary assets in the repository.
