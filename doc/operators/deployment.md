# DRACHMA Deployment Guide

This guide covers running DRACHMA nodes and the desktop wallet on testnet/mainnet, plus hardened procedures for operators and packagers.

## Quick start options
- **Pre-built binaries (recommended when available):** Download the latest tagged release from the [GitHub Releases page](https://github.com/Tsoympet/PARTHENON-CHAIN/releases), verify SHA-256 and GPG signatures, then follow the run commands below.
- **Docker:** Use the included `Dockerfile` for a single node or `docker-compose.yml` for a multi-node testnet stack with monitoring.
- **Build from source:** Follow `../getting-started/building.md` for platform-specific instructions.

## Running a full node
- Minimal testnet start:
  ```bash
  ./drachmad --network testnet --datadir ~/.drachma-testnet --listen \
    --rpcuser=user --rpcpassword=pass --prune=550
  ```
- Mainnet with explicit config:
  ```bash
  ./drachmad --network mainnet --conf=/etc/drachma.conf --datadir=/var/lib/drachma --daemon=0
  ```
- Verify sync and connectivity:
  ```bash
  ./drachmad -rpcuser=user -rpcpassword=pass --network testnet getblockchaininfo
  scripts/sync-check.sh --network testnet
  ```

## Running the desktop wallet
- Connect to a local node:
  ```bash
  ./drachma-wallet --connect 127.0.0.1:9333
  ```
- Connect to a remote node (trusted RPC):
  ```bash
  ./drachma-wallet --connect rpc.example.com:9333 --rpcuser=user --rpcpassword=pass
  ```
- On macOS and Windows, launch from the bundled `.app` or `.exe` produced by `macdeployqt`/`windeployqt`.

## Docker alternative
- **Single node:**
  ```bash
  docker build -t drachma/base -f Dockerfile .
  docker run --rm -p 19333:19333 -p 18332:18332 drachma/base ./drachmad --network testnet --rpcuser=user --rpcpassword=pass
  ```
- **Multi-node with monitoring:**
  ```bash
  docker-compose up -d
  docker-compose logs -f drachma-seed-a
  ```
  The compose file wires seeds, peers, faucet, Prometheus, and Grafana; edit `docker-compose.yml` to change published ports.

## Pre-flight checklist (security & reliability)
- Run under a dedicated non-root user; avoid exposing RPC on public interfaces.
- Open firewall only for the chosen P2P port (testnet 19333 / mainnet 9333) and SSH/VPN.
- Use strong RPC credentials and, where possible, TLS termination in front of public endpoints.
- Keep `checkpoints.json` current and monitor tip age with `scripts/sync-check.sh`.
- Back up configs and wallets (if applicable) before upgrading. Use `scripts/upgrade-node.sh` for atomic replacements.

## Hardened systemd service
Example unit for a dedicated `drachma` user:
```ini
[Unit]
Description=Drachma node
After=network-online.target
Wants=network-online.target

[Service]
User=drachma
Group=drachma
ExecStart=/opt/drachma/bin/drachmad --datadir=/var/lib/drachma --network=mainnet --conf=/etc/drachma.conf --daemon=0
Restart=on-failure
RestartSec=5
LimitNOFILE=65536
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ProtectHome=yes
ProtectKernelTunables=yes
ProtectControlGroups=yes
ReadWritePaths=/var/lib/drachma

[Install]
WantedBy=multi-user.target
```
- Place configs in `/etc/drachma.conf` with `chmod 640` and `chown drachma`.
- Consider `SystemCallFilter=@system-service` and `MemoryDenyWriteExecute=yes` where compatible.
- Use `tmpfs` for `/var/lib/drachma/tmp` if database durability settings permit.

## Monitoring and health
- Prometheus and Grafana dashboards live in `testnet/monitoring/`; update scrape targets as needed.
- Example Prometheus static config:
  ```yaml
  scrape_configs:
    - job_name: drachma-node
      static_configs:
        - targets: ['127.0.0.1:9311']
  ```
- `scripts/sync-check.sh` alarms when the tip age exceeds a threshold—use in cron or CI.
- Export systemd logs to a centralized collector (Loki, Elastic) for retention.

## Upgrade and recovery
- Use `scripts/upgrade-node.sh -b /path/to/new/drachma_node --sha256 <digest> --restart -s drachma.service` to atomically install new binaries.
- Back up the data directory and `drachma.conf` regularly; for wallet nodes, export encrypted backups from the wallet UI or copy `wallet.dat` while the wallet is locked.
- Validate backups by restoring on an isolated host and checking balances/transactions via RPC.
