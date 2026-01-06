# DRACHMA Frequently Asked Questions (FAQ)

**Version:** 1.0  
**Last Updated:** January 6, 2026

This document answers common questions about the DRACHMA blockchain.

---

## General Questions

### What is DRACHMA?

DRACHMA is a multi-asset blockchain featuring:
- **Proof-of-Work** (SHA-256d) consensus for security
- **Three native assets:** TLN (Talanton), DRM (Drachma), OBL (Obolos)
- **Hybrid PoW/PoS** support with different rules per asset
- **WASM-based smart contracts** and NFTs
- **No premine** - fair launch with transparent emission schedule

### Is mainnet live?

**No, not yet.** Mainnet launch is planned for 10-12 weeks from now (as of January 2026) after completing:
- External security audit
- RPC layer hardening
- Reproducible builds setup
- Extended testnet validation

See `ANALYSIS-EXECUTIVE-SUMMARY.md` for the complete launch roadmap.

### How is DRACHMA different from Bitcoin?

**Similarities:**
- SHA-256d proof-of-work
- UTXO model
- No premine, no central authority
- Open source

**Key differences:**
- **Multi-asset:** Three assets with different monetary policies
- **Schnorr signatures:** More efficient than ECDSA
- **WASM smart contracts:** Mandatory execution layer
- **Hybrid PoW/PoS:** DRM and OBL support staking
- **Faster blocks:** 60-second target (vs. Bitcoin's 10 minutes)
- **Different supply:** 41M DRM (vs. Bitcoin's 21M BTC)

### What are TLN, DRM, and OBL?

**TLN (Talanton):**
- Pure PoW asset
- Max supply: 21,000,000
- Initial block reward: 5 TLN
- Halvings every 2,102,400 blocks (~4 years)

**DRM (Drachma):**
- Hybrid PoW + PoS asset
- Max supply: 41,000,000
- Initial block reward: 10 DRM
- PoS staking with 4% APR
- Primary settlement asset

**OBL (Obolos):**
- Pure PoS asset (no mining)
- Max supply: 61,000,000
- Earned through staking only
- Eth2-style variable APR (0.5%-5%)
- Designed for dApps and governance

---

## Technical Questions

### What is the block time?

**60 seconds** target block time, compared to Bitcoin's 10 minutes.

### What is the difficulty adjustment algorithm?

- Retarget window: **60 blocks** (~1 hour)
- Adjustment bounds: **±25% per retarget**
- More responsive than Bitcoin's 2-week adjustment

### How do halvings work?

Block rewards halve every **2,102,400 blocks** (~4 years at 60s blocks):

**DRM Emission Schedule:**
- Era 0 (blocks 0-2,102,399): 10 DRM/block → 21,024,000 DRM
- Era 1 (blocks 2,102,400-4,204,799): 5 DRM/block → 10,512,000 DRM more
- Era 2 (blocks 4,204,800-6,307,199): 2.5 DRM/block → 5,256,000 DRM more
- And so on, converging to 41,000,000 DRM cap

### Are there transaction fees?

Yes. Miners/validators earn:
1. Block subsidy (new coins)
2. Transaction fees from included transactions

Fees are market-driven and depend on network congestion.

### What signature algorithm is used?

**Schnorr signatures** over secp256k1 (same curve as Bitcoin).

Benefits:
- More efficient than ECDSA
- Smaller signatures
- Enables advanced features (multisig aggregation, adaptor signatures)

### Is SegWit or Taproot implemented?

Not directly, but DRACHMA uses **Schnorr natively** rather than as an upgrade layer. The UTXO model and script system are simplified compared to Bitcoin.

---

## Mining Questions

### Can I mine DRACHMA?

Yes! Mining options:
- **CPU mining:** Reference implementation provided
- **GPU mining:** CUDA and OpenCL miners available
- **ASIC mining:** SHA-256d is ASIC-friendly (like Bitcoin)

**Note:** Mainnet isn't live yet. You can mine on testnet for testing.

### What hardware do I need?

**Minimum (testnet/small-scale):**
- Modern multi-core CPU
- Or entry-level GPU

**Competitive mining (mainnet, when live):**
- SHA-256d ASICs (like Bitcoin miners)
- Or GPU farms

**For node operation only:**
- 2+ CPU cores
- 4GB+ RAM
- 50GB+ disk space

### Are mining pools available?

Not yet for mainnet (which isn't live).

Pool protocol support (Stratum) is implemented in the reference miner. Pools can be set up by third parties after mainnet launch.

### What's the expected difficulty?

Unknown until mainnet launch. Will depend on:
- Initial hashrate from miners
- Market interest
- ASIC participation

Testnet difficulty is very low for easy testing.

### Can I mine multiple assets simultaneously?

**Only one asset per block:**
- Miners choose which asset to mine (TLN or DRM)
- OBL cannot be mined (PoS only)
- Different assets compete for the same SHA-256d hash power

---

## Staking Questions

### How does staking work?

**Proof-of-Stake** is available for DRM and OBL:

1. Hold coins in a wallet
2. Keep wallet online and unlocked for staking
3. Wait for minimum stake depth (confirmations)
4. Earn staking rewards based on:
   - Amount staked
   - Time held
   - Network participation

### What's the minimum stake?

**Minimum stake depth:** 500 blocks for coins to mature

**No minimum amount** - any amount can stake, but tiny amounts may rarely produce blocks.

### What are the staking rewards?

**DRM:** ~4% annual return (APR)

**OBL:** Variable APR (Eth2-style curve)
- ~5% at low participation
- ~1.5% at high participation
- Encourages balanced network participation

### Can I stake and mine simultaneously?

Yes! You can:
- Mine TLN or DRM with PoW
- Stake DRM or OBL with PoS
- All in the same wallet/node

### Is there slashing?

**No slashing** in the current design. Unlike Ethereum 2.0, there are no penalties for:
- Downtime
- Double-signing
- Validator misbehavior

This makes staking more forgiving for casual users.

---

## Wallet Questions

### What wallet software is available?

**Included in this repository:**
- Command-line wallet (`drachma-cli`)
- GUI wallet (Qt-based, in `layer3-app/`)

**Hardware wallet support:**
- Marked as experimental
- Ledger/Trezor integration in development

### How do I back up my wallet?

**Two methods:**

1. **Seed phrase (recommended):**
   - Write down your 24-word mnemonic
   - Store securely offline
   - Can restore wallet on any device

2. **Wallet file:**
   - Backup `~/.drachma/wallet.dat`
   - Encrypt with strong passphrase
   - Store in multiple secure locations

### Can I use the same wallet for all assets?

**Yes!** One wallet handles TLN, DRM, and OBL simultaneously. Each has separate balances and addresses.

### Are addresses reusable?

Yes, but **not recommended** for privacy. Generate a new address for each transaction when possible.

### What address format is used?

Native format based on BIP-340 x-only public keys (Schnorr). Different from Bitcoin's legacy, SegWit, or Taproot addresses.

---

## Smart Contract Questions

### Does DRACHMA support smart contracts?

**Yes!** Via a **mandatory WASM execution layer** (sidechain).

### What language are contracts written in?

Any language that compiles to **WebAssembly (WASM)**:
- Rust (recommended)
- C/C++
- AssemblyScript
- Others

### How are contracts deployed?

1. Compile contract to WASM bytecode
2. Deploy to sidechain via transaction
3. Pay deployment fee in DRM or OBL
4. Contract is now callable by anyone

### Are contracts Turing-complete?

Yes, but with **gas limits** to prevent infinite loops and DoS attacks (like Ethereum).

### What about NFTs?

NFTs are **asset-agnostic Layer-2 records** anchored to Layer 1:
- Minted on WASM sidechain
- Can represent any digital asset
- Royalty enforcement supported
- Marketplace contracts available

---

## Economic Questions

### What's the total supply?

**Multi-asset totals:**
- TLN: 21,000,000 max
- DRM: 41,000,000 max
- OBL: 61,000,000 max

**Note:** These are independent assets, not denominations of the same coin.

### Was there a premine?

**No premine.** All coins are:
- Mined via PoW (TLN, DRM)
- Earned via PoS (DRM, OBL)
- No ICO, no founders' reward, no treasury

See `docs/reference/fair-launch.md` for details.

### What's the emission schedule?

See `docs/reference/economics.md` for detailed emission tables.

**DRM summary:**
- ~21M in first 4 years
- ~31.5M by year 8
- Converges to 41M over decades
- Halvings every ~4 years

### Will there be inflation forever?

**Eventually, no.** The geometric series converges:
- TLN → exactly 21,000,000 (then zero)
- DRM → converges to 41,000,000 (hard cap enforced)
- OBL → staking inflation, but capped at 61,000,000

After many halvings, block subsidies become negligible and fees dominate.

---

## Development Questions

### Is DRACHMA open source?

**Yes!** Licensed under MIT license. See `LICENSE` file.

### How can I contribute?

See `CONTRIBUTING.md` for:
- Code contribution guidelines
- Pull request process
- Code style requirements
- Testing requirements

### Where's the roadmap?

See:
- `ANALYSIS-EXECUTIVE-SUMMARY.md` - Launch readiness
- `docs/LAUNCH-ACTION-ITEMS.md` - Pre-launch tasks
- `docs/MAINNET-READINESS.md` - Technical assessment

### Has the code been audited?

**Not yet.** External security audit is planned before mainnet launch (8-12 weeks from January 2026).

See `.github/SECURITY.md` for security policy and responsible disclosure.

### What programming language is it written in?

- **Core blockchain:** C++ (like Bitcoin)
- **Smart contracts:** WASM (any language that compiles to it)
- **Scripts/tools:** Python, Bash, etc.

### Can I run a testnet node?

**Yes!** Testnet is live:

```bash
drachma-node --testnet
```

See `docs/getting-started/quickstart.md` for setup.

---

## Security Questions

### Is DRACHMA secure?

**Cryptographically:** Uses battle-tested primitives (SHA-256d, secp256k1, Schnorr).

**Consensus:** Based on proven PoW model with added PoS support.

**Audit status:** Pre-audit. External security review pending before mainnet.

**Risk level:** Mainnet not yet recommended for production use. See `ANALYSIS-EXECUTIVE-SUMMARY.md` for assessment.

### Are there known vulnerabilities?

See `.github/SECURITY.md` and `docs/security/AUDIT.md` for:
- Known limitations
- Areas requiring hardening (RPC layer)
- Responsible disclosure process

### How do I report a security issue?

**Do NOT open a public GitHub issue.**

Instead:
1. Review `.github/SECURITY.md`
2. Email security contacts privately
3. Wait for response before disclosure

### Is quantum computing a threat?

**Long-term, yes** (like all cryptocurrencies using ECDSA/Schnorr).

**Current risk:** Low. Quantum computers aren't yet capable of breaking secp256k1.

**Mitigation:** Post-quantum signature schemes may be added in future upgrade.

---

## Community Questions

### Where can I get support?

1. **Documentation:** `docs/` folder
2. **Troubleshooting:** `docs/TROUBLESHOOTING.md`
3. **GitHub Issues:** Bug reports and questions
4. **Community channels:** TBD (Discord/Matrix/Forum)

### How can I stay updated?

- Watch the GitHub repository
- Follow release announcements
- Join community channels (when available)

### Is there a foundation or company behind DRACHMA?

**No.** DRACHMA is a community project with:
- No central authority
- No governance tokens
- No admin keys
- Open source development

---

## Miscellaneous

### What does "DRACHMA" mean?

Historical currency used in ancient Greece, representing a return to sound money principles.

### Why "Talanton," "Drachma," and "Obolos"?

All are ancient Greek currency denominations:
- **Talanton:** Large unit (talent)
- **Drachma:** Common unit
- **Obolos:** Small unit (obol)

Chosen to reflect the multi-asset nature with historical significance.

### When moon?

**When ready.** 🚀

DRACHMA prioritizes security and proper engineering over rushing to market. See launch timeline in `ANALYSIS-EXECUTIVE-SUMMARY.md`.

---

## Quick Reference

**Key Parameters:**
- Block time: 60 seconds
- Halving: 2,102,400 blocks (~4 years)
- Difficulty adjustment: 60 blocks
- Signature: Schnorr/secp256k1
- Hash: SHA-256d
- Mainnet port: 9333
- Testnet port: 19333

**Supply Caps:**
- TLN: 21,000,000
- DRM: 41,000,000
- OBL: 61,000,000

**Documentation:**
- Getting Started: `docs/getting-started/`
- Technical Specs: `docs/technical-specs/`
- Operations: `docs/operators/`
- Security: `docs/security/`

---

**Have more questions?**

Check the full documentation or ask in GitHub Issues!

**Last Updated:** January 6, 2026
