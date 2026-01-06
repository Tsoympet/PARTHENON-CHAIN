# DRACHMA Mining Setup Guide

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Target Audience:** Users wanting to mine TLN or DRM

This guide helps you set up and optimize mining for the DRACHMA blockchain.

---

## Prerequisites

Before you start mining:

1. **Running node:** You need a synced DRACHMA node
2. **Mining hardware:** CPU, GPU, or ASIC
3. **Mining software:** Reference miners included in this repository
4. **Basic knowledge:** Command-line usage, configuration files

**Note:** Mainnet isn't live yet. This guide covers testnet mining for now.

---

## Table of Contents

1. [Node Setup](#node-setup)
2. [CPU Mining](#cpu-mining)
3. [GPU Mining](#gpu-mining)
4. [Pool Mining](#pool-mining)
5. [Optimization](#optimization)
6. [Monitoring](#monitoring)
7. [Troubleshooting](#troubleshooting)

---

## Node Setup

### 1. Install and Sync Node

```bash
# Build from source
cd BlockChainDrachma
make
sudo make install

# Start node (testnet for now)
drachma-node --testnet

# Check sync status
drachma-cli --testnet getblockchaininfo
```

Wait for node to fully sync before mining.

### 2. Configure RPC

Edit `~/.drachma/drachma.conf`:

```conf
# Enable RPC server for mining
server=1
rpcuser=miner
rpcpassword=CHANGE_THIS_PASSWORD

# Allow local RPC connections
rpcallowip=127.0.0.1
rpcbind=127.0.0.1:9332  # Mainnet
# rpcbind=127.0.0.1:19332  # Testnet

# Optimize for mining
maxconnections=125
```

Restart node after config changes:

```bash
drachma-cli stop
drachma-node --testnet  # Or without --testnet for mainnet (when live)
```

### 3. Test RPC Connection

```bash
curl --user miner:CHANGE_THIS_PASSWORD \
  --data-binary '{"jsonrpc":"1.0","id":"test","method":"getblockcount","params":[]}' \
  -H 'content-type: text/plain;' \
  http://127.0.0.1:19332/
```

Should return current block height.

---

## CPU Mining

Reference CPU miner is included for testing and small-scale mining.

### Setup

```bash
# Build CPU miner (if not already built)
cd miners/cpu
make

# Or use pre-built binary
./drachma-cpu-miner --version
```

### Basic Usage

```bash
# Mine on local node (testnet)
./drachma-cpu-miner \
  --url http://127.0.0.1:19332 \
  --user miner \
  --pass CHANGE_THIS_PASSWORD \
  --threads 4

# For mainnet (when live)
./drachma-cpu-miner \
  --url http://127.0.0.1:9332 \
  --user miner \
  --pass CHANGE_THIS_PASSWORD \
  --threads 4
```

### Configuration

Create `cpu-miner.conf`:

```conf
# Node connection
url = http://127.0.0.1:19332
user = miner
pass = CHANGE_THIS_PASSWORD

# Mining settings
threads = 4
intensity = 20
asset = DRM  # Or TLN

# Logging
quiet = false
debug = false
```

Run with config:

```bash
./drachma-cpu-miner --config cpu-miner.conf
```

### CPU Miner Options

| Option | Description | Default |
|--------|-------------|---------|
| `--threads N` | Number of CPU threads | Auto-detect |
| `--intensity N` | Mining intensity (1-31) | 20 |
| `--asset ASSET` | Asset to mine (TLN/DRM) | DRM |
| `--timeout N` | RPC timeout (seconds) | 30 |
| `--retries N` | Connection retry attempts | 3 |
| `--quiet` | Reduce output verbosity | false |

### Performance Tips

**Thread count:**
- Use 1-2 threads less than total cores (leave some for OS)
- Example: 8-core CPU → use 6 threads

**Intensity:**
- Higher = more aggressive (more heat/power)
- Lower = more conservative
- Test different values: 15, 20, 25

**Hash rate expectations:**
- Modern CPU: 10-50 MH/s per core
- Older CPU: 1-10 MH/s per core
- Temperature should stay below 80°C

---

## GPU Mining

GPU mining offers significantly better performance than CPU.

### CUDA (NVIDIA)

```bash
# Build CUDA miner
cd miners/gpu-cuda
make

# Run
./drachma-gpu-cuda \
  --url http://127.0.0.1:19332 \
  --user miner \
  --pass CHANGE_THIS_PASSWORD \
  --device 0 \
  --intensity 20
```

### OpenCL (AMD/General)

```bash
# Build OpenCL miner
cd miners/gpu-opencl
make

# Run
./drachma-gpu-opencl \
  --url http://127.0.0.1:19332 \
  --user miner \
  --pass CHANGE_THIS_PASSWORD \
  --device 0 \
  --intensity 20
```

### GPU Options

| Option | Description | Default |
|--------|-------------|---------|
| `--device N` | GPU device ID | 0 |
| `--intensity N` | GPU intensity (8-31) | 20 |
| `--worksize N` | Work group size | Auto |
| `--threads N` | GPU threads per block | Auto |

### Multi-GPU Setup

List available GPUs:

```bash
./drachma-gpu-cuda --list-devices
# or
./drachma-gpu-opencl --list-devices
```

Mine with multiple GPUs:

```bash
# Terminal 1 (GPU 0)
./drachma-gpu-cuda --device 0 --intensity 22

# Terminal 2 (GPU 1)
./drachma-gpu-cuda --device 1 --intensity 22
```

Or use a script:

```bash
#!/bin/bash
for i in 0 1 2 3; do
  ./drachma-gpu-cuda --device $i --intensity 22 &
done
wait
```

### Performance Tips

**Intensity tuning:**
1. Start at intensity 18
2. Increase by 2 until:
   - GPU reaches 95%+ utilization
   - Or system becomes unstable
3. Back off by 1-2 for stability

**Temperature management:**
- Keep GPU under 80°C
- Improve cooling if needed
- Reduce intensity if too hot

**Power limit:**
- Reduce power limit to 70-80% for efficiency
- Example (NVIDIA): `nvidia-smi -pl 200` (200W limit)

**Hash rate expectations:**
- GTX 1660: ~500-800 MH/s
- RTX 3060: ~1000-1500 MH/s
- RX 580: ~400-700 MH/s
- High-end GPUs: 2000+ MH/s

---

## Pool Mining

Pool mining shares work and rewards among multiple miners.

### Stratum Protocol

DRACHMA supports standard Stratum protocol:

```bash
./drachma-cpu-miner \
  --url stratum+tcp://pool.example.com:3333 \
  --user your_wallet_address.worker_name \
  --pass x
```

### Pool Selection

Choose pools based on:
- **Fee:** Typical 1-3%
- **Location:** Closer = lower latency
- **Payout:** Threshold and frequency
- **Reputation:** Established pools preferred

### Monitoring Pool Stats

Most pools provide web dashboards:
- Current hashrate
- Shares submitted
- Estimated earnings
- Payout history

---

## Optimization

### Asset Selection

You can mine TLN or DRM (not both simultaneously):

```bash
# Mine TLN (21M supply, pure PoW)
./drachma-cpu-miner --asset TLN

# Mine DRM (41M supply, PoW+PoS)
./drachma-cpu-miner --asset DRM
```

**Strategy:**
- Early on: Higher difficulty asset may be less profitable
- Long term: Choose based on personal preference
- Profitability calculators may be available after mainnet launch

### Reducing Orphan Blocks

**Solo miners:**
1. Ensure good network connectivity
2. Low latency to well-connected peers
3. Monitor orphan rate: `drachma-cli getmininginfo`

**Pool miners:**
- Choose geographically close pool
- Use wired connection (not Wi-Fi)

### Energy Efficiency

**Measure power consumption:**
- Use power meter (Kill-A-Watt or similar)
- Calculate: `profit = (coins_per_day * price) - (kWh_per_day * electricity_cost)`

**Optimize:**
- Undervolt GPU for better efficiency
- Reduce intensity if power-limited
- Mine during off-peak electricity hours

### Overclocking

**CPU:**
- Not recommended unless experienced
- Minimal gains, high stability risk

**GPU:**
- Core: +50 to +150 MHz
- Memory: +500 to +1000 MHz
- Test stability extensively
- Monitor for errors/invalid shares

---

## Monitoring

### Basic Stats

```bash
# Mining info
drachma-cli getmininginfo

# Network hashrate
drachma-cli getnetworkhashps

# Your blocks mined (if solo)
drachma-cli listunspent | grep coinbase
```

### Miner Dashboard

Monitor miner output for:
- **Hashrate:** MH/s or GH/s
- **Accepted shares:** Should be > 95%
- **Temperature:** Should be safe
- **Errors:** Should be minimal

### Automated Monitoring

Create monitoring script:

```bash
#!/bin/bash
# monitor-mining.sh

while true; do
  echo "=== $(date) ==="
  
  # Hash rate
  drachma-cli getnetworkhashps
  
  # Your balance
  drachma-cli getbalance
  
  # Mining info
  drachma-cli getmininginfo
  
  sleep 300  # Every 5 minutes
done
```

### Alerting

Set up alerts for:
- Miner crashes (process monitoring)
- Temperature too high
- Hashrate drops significantly
- Node out of sync

---

## Troubleshooting

### No Blocks Found

**Solo mining:**
- Expected if network hashrate is high
- Consider pool mining
- Verify miner is actually running

**Pool mining:**
- Check pool stats for your shares
- Ensure correct worker credentials

### High Reject Rate

**Causes:**
- Stale work (slow connection)
- Overclocking too aggressive
- Miner misconfiguration

**Solutions:**
- Reduce intensity slightly
- Check network latency
- Verify RPC connection
- Reduce overclock

### Miner Crashes

**Common causes:**
- Overheating (check temps)
- Unstable overclock
- Power supply insufficient
- GPU driver issues

**Solutions:**
- Reduce intensity
- Improve cooling
- Update drivers
- Check PSU capacity

### Low Hashrate

**CPU/GPU not fully utilized:**
- Increase intensity
- Check for background processes
- Verify correct driver installation

**Thermal throttling:**
- Clean dust from heatsinks
- Improve airflow
- Replace thermal paste

---

## Advanced Topics

### Merged Mining

Currently not supported. Each block mines one asset (TLN or DRM).

### Mining Farm Management

For large-scale operations:
- Use management software (AwesomeMiner, Hive OS, etc.)
- Centralized monitoring dashboard
- Automated failover and restart
- Remote configuration management

### Custom Miners

Reference miners are intentionally simple. For production:
- Optimize assembly code for your hardware
- Implement ASIC-specific features
- Consider closed-source for competitive advantage

**Be a good citizen:**
- Don't abuse RPC endpoints
- Respect pool rules
- Report bugs in reference miners

---

## Safety and Security

### Wallet Security

**Never mine to an exchange:**
- Use your own wallet
- Control your private keys
- Regular backups

**Hot vs. cold:**
- Mine to hot wallet for convenience
- Periodically transfer to cold storage

### Pool Security

**Verify pool legitimacy:**
- Check reputation
- Start with small amounts
- Monitor payouts

**Credentials:**
- Use unique password per pool
- Never share your wallet seed

### Physical Security

**Fire safety:**
- Don't overload circuits
- Use quality power supplies
- Monitor temperatures
- Have fire extinguisher nearby

**Theft prevention:**
- Secure physical access to rigs
- Inventory valuable hardware
- Consider insurance for large operations

---

## Profitability

### Estimating Earnings

**Formula:**
```
daily_coins = (your_hashrate / network_hashrate) * blocks_per_day * block_reward
```

**Variables:**
- `your_hashrate`: Your miner's speed (MH/s or GH/s)
- `network_hashrate`: Total network (from `getnetworkhashps`)
- `blocks_per_day`: 1440 (60s blocks)
- `block_reward`: 10 DRM or 5 TLN initially (halves every ~4 years)

### Break-Even Analysis

**Costs:**
- Hardware (amortized over lifetime)
- Electricity (kWh × rate)
- Cooling
- Internet

**Revenue:**
- Coins mined per day × market price
- Consider holding vs. selling

**ROI:**
- Calculate months to break even
- Account for difficulty increases
- Consider halvings (every ~4 years)

---

## Getting Help

**Issues or questions?**

1. Check `docs/TROUBLESHOOTING.md`
2. Review miner logs
3. Search GitHub Issues
4. Ask in community channels

**Reporting bugs:**
- Include: OS, hardware, miner version, error messages
- Provide minimal reproduction steps

---

## Quick Reference

**Start mining (testnet):**
```bash
# CPU
./drachma-cpu-miner --url http://127.0.0.1:19332 --user miner --pass PASSWORD --threads 4

# GPU (CUDA)
./drachma-gpu-cuda --url http://127.0.0.1:19332 --user miner --pass PASSWORD --device 0
```

**Check stats:**
```bash
drachma-cli getmininginfo
drachma-cli getnetworkhashps
drachma-cli getbalance
```

**Asset choice:**
- `--asset TLN` for Talanton (21M cap, pure PoW)
- `--asset DRM` for Drachma (41M cap, PoW+PoS)

---

**Happy mining!** ⛏️

Remember: Mainnet isn't live yet. Practice on testnet first.

**Last Updated:** January 6, 2026
