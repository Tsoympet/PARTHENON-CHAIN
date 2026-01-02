# Multi-Asset Upgrade Plan (Talanton / Drachma / Obolos)

Status: Draft specification for implementation sequencing.

## Goals
- Introduce three native assets on one chain with shared SHA-256d PoW/validation:  
  - **Talanton (TLN):** PoW-only, Bitcoin-aligned parameters.  
  - **Drachma (DRM):** Keep current params; add PoS alongside existing PoW.  
  - **Obolos (OBL):** PoS-only with fixed 61M supply target.
- Use a single UTXO set with **asset tags**; each transaction moves **one asset type**.
- Activate via hard fork / new genesis height to avoid legacy state drift.

## Consensus Parameters (proposed)

### Talanton (TLN) — PoW
- Supply/issuance: match Bitcoin (21M cap, 10 min target, halving every 210k blocks).
- Hash: SHA-256d (existing).
- Difficulty: follow current retarget logic unless explicitly updated later.

### Drachma (DRM) — PoW + PoS
- Retain current DRM PoW parameters and rewards.
- Add PoS with 10-minute slots.
- PoS reward: flat **4% annual** on staked balance, compounded per slot.
- Minimum stake age: 1 day.
- Coinbase split: PoW rewards unchanged; PoS rewards paid separately per stake block.

### Obolos (OBL) — PoS
- Total supply target: **61,000,000 OBL**.
- Slots: 10 minutes.
- PoS reward: Eth2-style curve scaled to 61M (e.g., ~5% APR at low participation trending toward ~1.5% near full participation). Exact curve to be codified during implementation.
- Minimum stake age: 1 day.

## Transaction & UTXO Model
- Single UTXO set with an **asset identifier** on each UTXO.
- Each transaction may spend/create UTXOs of one asset type only (no mixed-asset tx).
- Standard scripts/addresses remain; asset ID is enforced at validation.

## Activation & Network
- Activation via **new genesis / hard fork** at designated height (to be set with testnet regenesis first).
- Keep existing SHA-256d, networking, and mempool policies unless a consensus rule requires tightening.
- Update seeds/configs for the forked network (testnet first).

## RPC / Wallet / Miner Surfacing
- RPC: add asset selection fields where amounts are specified; expose per-asset balances and staking info.
- Wallet/UI: allow choosing asset for send/receive, show per-asset balances, staking controls for DRM/OBL.
- Miners: PoW mining remains for TLN; staking flow for DRM/OBL blocks with asset-specific rewards.

## Next Steps (sequence)
1. Finalize numeric constants (Obolos curve parameters, activation height(s), stake age/grace).  
2. Implement consensus: asset-tagged UTXOs, single-asset tx rule, PoS logic for DRM/OBL, fork activation.  
3. Extend RPC/wallet/miner surfaces for asset selection and staking.  
4. Regenerate testnet configs/seeds and add targeted tests for multi-asset validation.  
5. Dress rehearsal on testnet before mainnet activation.
