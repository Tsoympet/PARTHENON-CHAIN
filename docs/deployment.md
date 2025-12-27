# DRACHMA Deployment Guide (Mainnet)

This guide describes how to prepare, deploy, and operate DRACHMA nodes for mainnet. It assumes familiarity with Linux server administration and a trusted build pipeline.

## Host Preparation

- **Operating system:** Use a long-term-support Linux distribution with timely security updates.
- **Isolation:** Run nodes under a dedicated user with minimal privileges. Prefer systemd units or containers with seccomp/AppArmor.
- **Storage:** Allocate fast SSD storage for blocks and chainstate; keep backups of `wallet.dat`/keystore on encrypted media.
- **Time sync:** Enable NTP or chrony; poor clocks can trigger timestamp rejections.
- **Networking:** Open only required ports (P2P 9333, RPC 8332). Restrict RPC to localhost or a VPN; disable passwordless auth.

## Build and Install

1. Follow [`docs/building.md`](building.md) with a release build profile.
2. Verify maintainer signatures on tags and compare SHA-256 checksums of binaries.
3. Install artifacts to a managed prefix, e.g., `/opt/drachma`, and record the exact commit hash.

## Configuration

Create a `drachma.conf` under your datadir (default `~/.drachma`):

```
network=mainnet
listen=1
rpcuser=<user>
rpcpassword=<strong-password>
txindex=1
maxconnections=64
banthreshold=100
``` 

Additional recommendations:

- Set `addnode=` entries to vetted seed hosts; use both IPv4 and IPv6.
- For miners/pools, expose RPC over TLS-terminated proxies only.
- Pin `datadir` to a dedicated filesystem and enable log rotation (systemd or `logrotate`).

## Running as a Service (systemd)

Example unit:

```
[Unit]
Description=DRACHMA Node
After=network-online.target
Wants=network-online.target

[Service]
User=drachma
Group=drachma
ExecStart=/opt/drachma/bin/drachmad --datadir=/var/lib/drachma --configfile=/etc/drachma/drachma.conf
Restart=on-failure
LimitNOFILE=65536
PrivateTmp=yes
ProtectSystem=full
ProtectHome=yes
NoNewPrivileges=yes

[Install]
WantedBy=multi-user.target
```

Enable with `systemctl enable --now drachma.service` after placing the config and creating directories with the correct ownership.

## Monitoring and Alerts

- Export metrics via the RPC or built-in telemetry endpoints if enabled; scrape with Prometheus/Telegraf.
- Watch disk, memory, and peer counts; alert on stalled tip height or repeated reorgs.
- Keep hashes of expected binaries and scripts; detect drift with periodic integrity checks.

## Backup and Recovery

- Back up encrypted wallet/keystore files and configuration; do **not** back up the entire chainstate.
- Test restores on an offline machine; verify balances via RPC and compare against a known-good node.
- Keep multiple copies of the mnemonic/seed in separate secure locations; never store unencrypted keys on production hosts.

## Incident Response

- In case of suspected compromise, revoke credentials, rotate RPC passwords, and redeploy from a trusted image.
- If a consensus bug is suspected, halt miners, preserve logs, and coordinate with maintainers using signed channels.
- Avoid unilateral patches; run only reviewed, signed releases until a coordinated fix is available.

## Post-Launch Hygiene

- Apply security updates promptly and reboot on kernel or OpenSSL patches.
- Periodically resync a fresh node to validate chain integrity.
- Audit `drachma.conf` for drift from recommended defaults after each release.
