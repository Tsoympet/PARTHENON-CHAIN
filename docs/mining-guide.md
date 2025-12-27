# DRACHMA Mining Guide

This guide explains how to run the reference CPU and GPU miners, tune performance, and choose hardware for DRACHMA's PoW.

## Miner Binaries

- **CPU Miner:** `miners/drachma-miner-cpu`
- **GPU Miner:** `miners/drachma-miner-gpu` (CUDA or OpenCL depending on build)

Both miners communicate with Layer 2 services via RPC to fetch block templates and submit solved headers.

## Prerequisites

- A synced DRACHMA node and services stack with RPC enabled.
- RPC credentials or access tokens configured (see Layer 2 service docs).
- For GPU mining: compatible drivers and CUDA/OpenCL runtime installed.

## Starting the CPU Miner

The CPU miner now supports solo RPC templates *and* Stratum pool mode. Anti-botnet defaults restrict remote pool connections; add `--allow-remote` only when you fully trust the endpoint.

```bash
./miners/drachma-miner-cpu \
  --stratum-url stratum+tcp://127.0.0.1:3333 \
  --stratum-user workername --stratum-pass password \
  --stratum-v2 --rpc-auth-token <jwt_or_token> \
  --threads <num> --min-target-bits 1d00ffff
```

Key flags:
- `--threads`: Number of CPU worker threads (multi-threaded hashing is enabled).
- `--min-target-bits`: Enforce a minimum target (compact bits) so pool jobs cannot downshift difficulty unexpectedly.
- `--benchmark` / `--benchmark-seconds`: Run an offline hash-rate benchmark with your chosen thread count.
- `--allow-remote`: Opt-in to non-local Stratum endpoints; keep unset for anti-botnet safety.
- `--stratum-v2`: Prefer Stratum V2 framing when the pool supports it; falls back to V1 otherwise.
- `--rpc-auth-token`: Optional bearer token for pools requiring stronger authentication (JWT or HMAC tokens).

## Starting the GPU Miner

CUDA example:
```bash
./miners/drachma-miner-gpu \
  --rpc-url http://127.0.0.1:9332 \
  --rpc-user <user> --rpc-pass <pass> \
  --backend cuda --devices 0,1 \
  --intensity 22
```

OpenCL example:
```bash
./miners/drachma-miner-gpu \
  --rpc-url http://127.0.0.1:9332 \
  --rpc-user <user> --rpc-pass <pass> \
  --backend opencl --platform 0 --devices 0 \
  --work-size 256
```

ROCm/AMD example:
```bash
./miners/drachma-miner-gpu \
  --rpc-url http://127.0.0.1:9332 \
  --rpc-user <user> --rpc-pass <pass> \
  --backend opencl --platform 1 --devices 0,1 \
  --work-size 256 --intensity 21
```

Key flags:
- `--backend`: `cuda` or `opencl`.
- `--devices`: Comma-separated device indices.
- `--platform`: OpenCL platform index (for AMD/Intel stacks).
- `--intensity` / `--work-size`: Tune kernel occupancy and batch size.
- `--pool`: Pool/Stratum URL; GPU miners forward solutions through the CPU miner's submitter when present.

## Performance Tuning

- **CPU:**
  - Use hugepages and NUMA pinning on multi-socket systems.
  - Set `--threads` to match physical cores; hyper-threading yields diminishing returns.
  - Pin miner threads with `taskset` or `numactl` to reduce cross-node latency.
  - Run `--benchmark --benchmark-seconds 20` after changes to compare hash rate.
- **GPU:**
  - Match `--intensity`/`--work-size` to available VRAM and bus bandwidth.
  - Keep GPUs cool; hash rate drops when thermal throttling triggers.
  - Use recent drivers; outdated OpenCL ICDs often reduce stability.
  - For CUDA, launch bounds are tuned for 256-thread blocks; prefer intensities that map to full SM occupancy.
  - AMD ROCm: stick to 256 work items per group for RDNA2/3; start with lower intensities on Polaris.
- **Node/Network:**
  - Run miners close to the node (same LAN) to minimize RPC latency.
  - Ensure the services daemon has sufficient peers for timely template updates.

## Hardware Recommendations

- **Entry / Development:** 4–8 core CPU, integrated or low-end GPU; suitable for testing.
- **Prosumer:** 12–24 core CPU, modern NVIDIA RTX (30/40 series) or AMD RDNA2/3 GPUs with >= 8 GB VRAM.
- **Enterprise:** Multi-GPU rigs with high-efficiency PSUs, adequate cooling, and stable power delivery. Consider ECC memory on hosts for reliability.
- **Compatibility notes:**
  - CUDA: Optimized for compute capability 7.5+ with 256-thread blocks; older GPUs run but may hash slower.
  - OpenCL: Targeted at AMD RDNA2/3 and Intel Arc drivers; set `--work-size 128` on legacy Polaris cards.
  - CPUs: Prefer AVX2-capable cores; SMT provides small gains only when memory bandwidth is ample.

## Pool Mining and Stratum Security

- Stratum is enabled for CPU miners via `--stratum-url/--stratum-user/--stratum-pass`.
- Pools supporting Stratum V2 can be reached with `--stratum-v2`; the miner will negotiate down to V1 if needed.
- Remote endpoints are blocked by default; add `--allow-remote` after verifying pool ownership to avoid botnet abuse.
- Use `--min-target-bits` to reject jobs below your chosen difficulty floor.
- Keep per-worker credentials unique; rotate passwords regularly and avoid running miners as root.

## Monitoring and Stability

- Enable verbose logs during tuning: `--log-level debug`.
- Watch acceptance rate; frequent rejects indicate stale work or unstable overclocks.
- Restart miners after driver updates to reload kernels.

## Mainnet Readiness Checklist

- Connect miners to trusted endpoints on secured networks (TLS or VPN) and avoid exposing Stratum over the public internet.
- Set conservative intensity/work-size values first; ramp up only after observing low reject rates.
- Pin `--min-target-bits` to prevent difficulty downgrades from malicious pools.
- Enable `--rpc-auth-token` for pools requiring stronger authentication and monitor the miner logs for anti-malware alerts.
- Keep host firmware, drivers, and the node software patched; rescan configs after each release.

## Troubleshooting

- **RPC auth failures:** Verify credentials and ensure RPC is bound to the expected interface.
- **Low hash rate:** Reduce intensity, update drivers, or test another backend (CUDA vs OpenCL).
- **Stales or rejects:** Improve connectivity to peers, lower overclocks, and ensure system clocks are synchronized (NTP).
- **Build/runtime errors:** Rebuild miners with matching driver/toolkit versions; see `docs/building.md` for GPU build notes.
