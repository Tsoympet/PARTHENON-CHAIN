# DRACHMA Audit Checklist

## Consensus
- [ ] Verify block header serialization and SHA-256d hashing match specification.
- [ ] Confirm difficulty retarget window (60 blocks) and ±25% clamp are enforced.
- [ ] Inspect subsidy schedule (50 DRM, halving every 210,000 blocks) and money-range checks for overflow.
- [ ] Validate Merkle tree duplication for odd leaves and correct root placement.
- [ ] Ensure Schnorr-only signature acceptance and script limits (no loops/recursion).
- [ ] Confirm coinbase maturity and fee calculations are enforced in block rewards.

## Chainstate and Storage
- [ ] Review UTXO database atomicity and rollback handling for reorgs.
- [ ] Check disk persistence integrity checks for blocks and UTXO state.
- [ ] Validate supply accounting and prevention of negative balances.

## Networking and Services (Layer 2)
- [ ] Confirm P2P message parsing uses network magic and bounded buffers.
- [ ] Review mempool admission rules, fee policy, and eviction strategy.
- [ ] Verify RPC authentication and absence of unauthenticated endpoints.
- [ ] Inspect wallet keystore encryption (AES-256) and mnemonic handling.
- [ ] Check cross-chain adapter verification logic for deterministic proof validation.

## Desktop Application
- [ ] Ensure the Qt UI does not bypass consensus rules or modify validation paths.
- [ ] Verify asset loading (branding, EULA, whitepaper) is local-only and immutable at runtime.
- [ ] Confirm miner controls interface with service layer without altering consensus parameters.

## Documentation
- [ ] Cross-check documentation against implementation for accuracy.
- [ ] Verify fair-launch declaration matches genesis configuration and absence of premine.
