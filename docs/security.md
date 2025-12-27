# DRACHMA Security Notes

## Cryptography
- SHA-256d for proof-of-work and Merkle hashing.
- Tagged SHA-256 for transaction and message domain separation.
- Schnorr signatures on secp256k1 only; no alternative curves or ECDSA fallback.
- Verification routines aim for constant-time behavior where applicable to limit timing side channels.

## Validation Pipeline
- Headers: structural checks, proof-of-work, and linkage to known ancestry.
- Transactions: deterministic serialization, value range enforcement, Schnorr verification, and script interpreter limits (no loops/recursion).
- Blocks: recompute subsidy based on height, enforce fees, coinbase maturity, and Merkle root correctness.
- Chainstate updates are atomic per block with rollback support for reorgs; corruption detection guards disk persistence.

## Networking and RPC
- P2P queues are bounded; invalid messages are discarded without resource amplification.
- RPC server requires authentication and rejects unauthenticated requests.
- No telemetry or remote attestation is collected; all networking is peer-to-peer.
- Magic bytes isolate networks; headers with invalid difficulty or timestamps trigger bans.

## Wallet Storage
- Wallet seeds are encrypted with AES-256 and stored locally only.
- Mnemonic-derived seeds never leave the host; exports should be offline backups.
- Wallet operations rely on Schnorr signing and deterministic change selection respecting relay policy.

## Build and Deployment
- C++17 and Qt 6 for the desktop application; consensus code is isolated in layer1-core and never modified by UI.
- No hidden parameters or privileged flags exist to bypass validation.
- Configuration should be performed through documented settings and authenticated RPC calls.
- Prefer deterministic builds, signature verification of releases, and systemd hardening (see `deployment.md`).

## Monitoring
Operators should monitor:
- Block height and peer count for potential partitions.
- Mempool behavior for unexpected fee spikes or unusual transaction patterns.
- Disk integrity of block and UTXO databases; leverage backups and fsync policies where appropriate.
- Resource metrics (CPU/memory) and alert thresholds tied to `sync-check` scripts and Prometheus exports.

## Operational Hygiene for Mainnet
- Enforce firewall rules that expose only P2P and admin SSH/VPN; never expose RPC publicly without TLS and auth.
- Keep OS packages and dependencies patched; upgrade nodes in waves with quick rollback paths.
- Store wallet backups offline; validate restores before relying on them.
- Document incident-response runbooks for reorgs, unexpected forks, and key compromise scenarios.
