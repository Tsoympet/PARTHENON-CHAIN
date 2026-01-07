# PARTHENON CHAIN Frequently Asked Questions (FAQ)

**Version:** 2.0  
**Last Updated:** January 6, 2026

**Master Brand**: PARTHENON CHAIN  
**Theme**: Classical Greece · Value · Order · Timelessness  
**Visual DNA**: Marble, Bronze, Silver, Obsidian

This document answers common questions about the PARTHENON CHAIN blockchain.

---

## General Questions

### What is PARTHENON CHAIN?

PARTHENON CHAIN (formerly DRACHMA) is a multi-asset blockchain built on Classical principles featuring:
- **Proof-of-Work** (SHA-256d) consensus for security
- **Three native assets:** TLN (Talanton/Bronze), DRM (Drachma/Silver), OBL (Obolos/Aegean Blue)
- **Pure PoW** - all assets are mined via proof-of-work
- **WASM-based smart contracts** and NFTs
- **No premine** - fair launch with transparent emission schedule
- **Classical design** - built to endure like the Parthenon

### Is mainnet live?

**No, not yet.** Mainnet launch is planned for 10-12 weeks from now (as of January 2026) after completing:
- External security audit
- RPC layer hardening
- Reproducible builds setup
- Extended testnet validation

See `ANALYSIS-EXECUTIVE-SUMMARY.md` for the complete launch roadmap.

### How is PARTHENON CHAIN different from Bitcoin?

**Similarities:**
- SHA-256d proof-of-work
- UTXO model
- No premine, no central authority
- Open source

**Key differences:**
- **Classical Greek theme:** Marble, Bronze, Silver, Obsidian design
- **Multi-asset:** Three assets with different monetary policies
- **Schnorr signatures:** More efficient than ECDSA
- **WASM smart contracts:** Mandatory execution layer
- **Pure PoW:** All assets are mined via proof-of-work
- **Faster blocks:** 60-second target (vs. Bitcoin's 10 minutes)
- **Different supply:** 41M DRM (vs. Bitcoin's 21M BTC)
- **Timeless design:** Built on Classical principles of order and permanence

### What are TLN, DRM, and OBL?

**TLN (Talanton) - Bronze:**
- Pure PoW asset
- Max supply: 21,000,000
- Initial block reward: 5 TLN
- Halvings every 2,102,400 blocks (~4 years)

**DRM (Drachma):**
- Pure PoW asset
- Max supply: 41,000,000
- Initial block reward: 10 DRM
- Halvings every 2,102,400 blocks (~4 years)
- Primary settlement asset

**OBL (Obolos):**
- Pure PoW asset
- Max supply: 61,000,000
- Initial block reward: 8 OBL
- Halvings every 2,102,400 blocks (~4 years)
- Designed for dApps and settlements

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
- Miners choose which asset to mine (TLN, DRM, or OBL)
- All three assets are mined via proof-of-work
- Different assets can be mined by the same hardware

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
- Mined via PoW (TLN, DRM, OBL)
- No ICO, no founders' reward, no treasury

See `doc/reference/fair-launch.md` for details.

### What's the emission schedule?

See `doc/reference/economics.md` for detailed emission tables.

**DRM summary:**
- ~21M in first 4 years
- ~31.5M by year 8
- Converges to 41M over decades
- Halvings every ~4 years

### Will there be inflation forever?

**Eventually, no.** The geometric series converges:
- TLN → exactly 21,000,000 (then zero)
- DRM → converges to 41,000,000 (hard cap enforced)
- OBL → converges to 61,000,000 (hard cap enforced)

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
- `doc/LAUNCH-ACTION-ITEMS.md` - Pre-launch tasks
- `doc/MAINNET-READINESS.md` - Technical assessment

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

See `doc/getting-started/quickstart.md` for setup.

### Do I need coverage.py for code coverage?

**No.** PARTHENON CHAIN is a **C++ project** that uses **gcovr** for code coverage, not Python's coverage.py.

**For coverage analysis:**
- Install `gcovr` (not coverage.py): `sudo apt-get install gcovr`
- Build with coverage enabled: `cmake -S . -B build-cov -DDRACHMA_COVERAGE=ON`
- Run tests: `ctest --test-dir build-cov`
- Generate report: `gcovr --root . --object-directory build-cov --print-summary`

The CI workflow automatically handles coverage using gcovr and uploads results to Codecov.

---

## Security Questions

### Is DRACHMA secure?

**Cryptographically:** Uses battle-tested primitives (SHA-256d, secp256k1, Schnorr).

**Consensus:** Based on proven PoW model (pure proof-of-work).

**Audit status:** Pre-audit. External security review pending before mainnet.

**Risk level:** Mainnet not yet recommended for production use. See `ANALYSIS-EXECUTIVE-SUMMARY.md` for assessment.

### Are there known vulnerabilities?

See `.github/SECURITY.md` and `doc/security/AUDIT.md` for:
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

1. **Documentation:** `doc/` folder
2. **Troubleshooting:** `doc/TROUBLESHOOTING.md`
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
- Getting Started: `doc/getting-started/`
- Technical Specs: `doc/technical-specs/`
- Operations: `doc/operators/`
- Security: `doc/security/`

---

**Have more questions?**

Check the full documentation or ask in GitHub Issues!

**Last Updated:** January 6, 2026
