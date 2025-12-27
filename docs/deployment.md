# DRACHMA Deployment Guide

This guide covers hardened procedures for running DRACHMA nodes on the public
testnet and preparing for mainnet launch.

## Prerequisites
- 64-bit Linux host with recent kernel, `curl`, `jq`, and `docker`/`docker-compose` (for containerized setups).
- Open firewall for the chosen P2P port (default 19335 testnet / 9333 mainnet).
- Dedicated user account to run the daemon; avoid running as root.

## Building from source
1. Clone the repository and run `scripts/build.sh`.
2. Verify the resulting binary hash matches the published release notes.
3. Record the commit hash used for reproducibility and attestation.

## Running a node
- Quick start: `scripts/start-node.sh -n testnet` will create a minimal config in
  `~/.drachma-testnet` and launch the daemon using the built binary.
- For production, copy `mainnet/config.sample.conf` into your datadir, adjust
  RPC credentials, and run `scripts/start-node.sh -n mainnet -c /path/to/drachma.conf`.
- Confirm connectivity with `scripts/sync-check.sh --network testnet` (or
  `mainnet`) which validates checkpoint height and block freshness.

## Firewall and Network Hardening
- Restrict inbound traffic to the P2P port and SSH/VPN; drop all else:
  ```bash
  sudo ufw default deny incoming
  sudo ufw allow 9333/tcp comment "drachma mainnet"
  sudo ufw allow 22/tcp    comment "ssh"
  sudo ufw enable
  ```
- If using `nftables`, prefer stateful rules and rate limits on new connections.
- Disable inbound RPC on public interfaces; bind RPC to localhost or VPN-only IPs.
- Enable `Fail2ban` or equivalent to block repeated auth failures on SSH and reverse proxies.

## systemd Service Hardening
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

## Docker-based multi-node testnet
- `docker-compose up -d` launches two seed nodes, three regular peers, a faucet,
  Prometheus, and Grafana. The compose file exposes RPC on 18332/18333/18334 and
  metrics on 9311 for the first seed.
- Tail logs with `docker-compose logs -f drachma-seed-a` and inspect metrics via
  Grafana at http://localhost:3000 (default password `drachma`).

## Security hardening
- Restrict RPC access to localhost or VPN ranges; always set strong RPC
  credentials and consider TLS termination via a reverse proxy for public
  faucets.
- Keep `checkpoints.json` up-to-date to resist long-range forks and configure
  `maxuploadtarget` and `banscore` as in `mainnet/config.sample.conf`.
- Enable systemd service isolation with `ProtectSystem=strict`, `NoNewPrivileges`
  and `PrivateTmp=yes`.
- Run nodes under dedicated Unix users; disable password SSH logins and enforce
  key-based auth.

## Monitoring and health
- Prometheus and Grafana dashboards live in `testnet/monitoring/`; update scrape
  targets if your hostnames differ from the default compose stack.
- Example Prometheus static config:
  ```yaml
  scrape_configs:
    - job_name: drachma-node
      static_configs:
        - targets: ['127.0.0.1:9311']
  ```
- `scripts/sync-check.sh` alarms when the tip age exceeds a threshold or the node
  lags published checkpoints—use in cron or CI.
- Export systemd logs to a centralized collector (Loki, Elastic) for retention.

## Upgrades
- Use `scripts/upgrade-node.sh -b /path/to/new/drachma_node --sha256 <digest>
  --restart -s drachma.service` to atomically install new binaries and restart a
  managed service.
- Always back up configs and the `wallet.dat` (if applicable) before upgrading;
  the script automatically preserves the previous binary with a timestamp.
- Stage upgrades on canary nodes before rolling to production peers.

## Backups and recovery
- Periodically back up the data directory and `drachma.conf`. For wallet nodes,
  export encrypted backups via the layer3 app’s backup menu or copy `wallet.dat`
  while the wallet is locked.
- Validate backups by restoring onto an isolated host and verifying `getbalance`
  and a few historical transactions via RPC.
- Keep offsite copies of configuration, TLS certificates (if used), and systemd
  unit files to rebuild quickly after incidents.
