# DRACHMA (DRM) Whitepaper

## Introduction
Drachma is a minimalist proof-of-work cryptocurrency engineered for predictable issuance, transparent accounting, and conservative security assumptions. The protocol follows a Bitcoin-class model with SHA-256d proof-of-work, Schnorr signatures, a UTXO ledger, and straightforward networking. The design prioritizes determinism, reproducibility, and reviewability over novelty while maintaining production-grade operational expectations.

## Monetary Policy
- **Max supply:** 42,000,000 DRM
- **Genesis premine:** None; the genesis coinbase is provably unspendable.
- **Block subsidy:** 10 DRM initially, halving every 2,102,400 blocks (~4 years at 60-second targets) to converge on the cap without tail emission.
- **Block interval:** 60 seconds target.
- **Reward maturity:** Coinbase outputs require standard maturity before spending.
- **Fee model:** Transaction fees are collected by miners and are the only reward after subsidy exhaustion.

The subsidy schedule follows integer division at each halving height, and cumulative supply accounting enforces the hard cap through consensus validation. Supply range checks and overflow prevention are part of transaction and block validation.

## Consensus Overview
1. **Proof-of-Work:** SHA-256d over the block header. Difficulty retargets every 60 blocks using a 3,600-second target window with ±25% clamp per period. Mainnet disallows minimum-difficulty blocks; testnet permits them when timestamps drift beyond the retarget window.
2. **Blocks:** Contain version, previous hash, Merkle root, timestamp, nBits, nonce, and transactions. Merkle roots duplicate the final leaf for odd layers to preserve deterministic tree construction.
3. **Transactions:** Deterministic serialization with tagged SHA-256 (BIP-340 style) for transaction IDs. Only Schnorr signatures over secp256k1 are valid; no ECDSA fallback exists. Scripts are minimal, non-Turing-complete, and forbid loops and recursion.
4. **Validation:** Nodes verify proof-of-work, header linkage, Merkle consistency, input availability, signature validity, coinbase maturity, fee correctness, and money-range constraints. Blocks failing any rule are rejected deterministically.

### Consensus Pseudocode (Simplified)
```pseudocode
function validate_block(block, chainstate):
    assert block.header.previous_hash == chainstate.tip.hash
    assert block.header.nBits == retarget(chainstate)
    assert double_sha256(block.header) <= target_from_bits(block.header.nBits)
    assert merkle_root(block.transactions) == block.header.merkle_root

    subsidy = subsidy_for_height(chainstate.height + 1)
    fees = 0
    for tx in block.transactions:
        assert check_serialization(tx)
        assert check_scripts(tx)
        assert check_schnorr_signatures(tx)
        fees += tx.input_value - tx.output_value
    assert block.coinbase_output_value == subsidy + fees

    chainstate.apply(block)
```

```pseudocode
loop consensus():
    block = p2p.wait_for_block()
    if validate_block(block, chainstate):
        chainstate = chainstate.advance(block)
        p2p.broadcast(block.header)
    else:
        p2p.penalize(block.sender)
```

The reference implementation mirrors this flow with replay-safe state transitions and rollback metadata for reorganizations.

## Genesis Block
The genesis block includes an unspendable coinbase with a commitment string documenting the chain launch. The Merkle root is derived from this single transaction. The genesis header is mined so its double-SHA-256 hash meets the encoded difficulty target. No special launch or checkpoint logic exists; the chain starts normally from height 0.

## Network
Drachma uses TCP-based peer connections with custom magic bytes per network (mainnet/testnet). Peers relay headers, transactions, and blocks respecting fee and size policies. DNS seeds and static seed nodes assist initial bootstrapping; bootstrap.dat import accelerates initial sync without bypassing validation.

## Wallet Model
Wallets are local-only HD wallets producing Schnorr keypairs. Seeds are 24-word mnemonics; private material is encrypted on disk using AES-256. The wallet tracks UTXOs, constructs transactions using fee estimation from mempool policy, and signs inputs with Schnorr.

## Security Considerations
- Deterministic serialization and tagged hashing reduce malleability risk.
- Schnorr signatures and constant-time verification mitigate timing leaks.
- Difficulty clamping prevents large oscillations while retaining responsiveness.
- UTXO set updates are atomic per block and reorg-safe through rollback metadata.
- No governance, staking, or admin keys exist; all participants follow the same rules.

## Security Model
DRACHMA assumes an **open, adversarial environment** with the following invariants:

- Honest miners eventually control >50% of cumulative work over meaningful windows.
- Nodes validate every rule locally; no trust is placed in checkpoints beyond optional operator-configured anchors.
- Network connectivity is imperfect: eclipse resistance relies on diverse peer selection (DNS seeds, manual `addnode`, inbound/outbound balance) and message-level magic bytes.
- Private keys remain off-chain; compromise of individual wallets does not affect ledger correctness.
- Layer 2/3 components may fail or be replaced without jeopardizing consensus so long as Layer 1 rules are enforced by every node.

The model explicitly **excludes** reliance on social recovery, governance voting, or trusted hardware. Safety derives from validation, cumulative work, and transparent parameters rather than special authorities.

## Threat Model (Structured)
The following threat model enumerates assets, adversaries, and mitigations. Severity assumes mainnet exposure.

| Asset | Adversary | Vector | Impact | Mitigations |
| --- | --- | --- | --- | --- |
| Chain correctness | Hashpower majority | Deep reorgs, double-spend | High | Chainwork-first fork choice, 60-block retarget with clamps, monitoring for abnormal reorg depth, operator-configured checkpoints for detection only |
| Network availability | Eclipse attacker | Peer isolation, poisoned headers | High | Diverse DNS/manual peers, inbound/outbound balance, misbehavior scoring, magic-byte isolation, eviction of anomalous headers |
| Wallet funds | Malware/operator error | Key theft, seed leakage | High | Local-only encrypted seeds, airgapped signing support, mnemonic backups, no custodial recovery backdoors |
| Node resources | DoS actor | Oversized messages, mempool flooding | Medium | Bounded queues, inventory limits, script/weight caps, fee and ancestor/descendant policy, per-peer bans |
| Privacy | Traffic analyst | Address/peer correlation | Medium | Optional Tor/VPN, manual peer rotation, no silent telemetry, discouraged RPC exposure |
| Supply cap | Protocol bug | Overflow, incorrect subsidy | Critical | Height-indexed subsidy calculation, consensus range checks, deterministic serialization tests, fuzzing of monetary logic |

Residual risks common to open networks—such as widespread miner collusion or nationwide censorship—require social coordination and monitoring rather than protocol-level overrides. Operational guidance appears in `deployment.md` and `security.md`.

## Comparison to Bitcoin
- **What stays the same:** SHA-256d proof-of-work, UTXO accounting, Schnorr-over-secp256k1 signatures, and conservative script rules mirror Bitcoin’s well-tested foundations. Fork choice relies on cumulative work and full validation—never checkpoints or trust anchors.
- **What differs:**
  - **Block cadence:** 60-second targets with 60-block retargets tighten confirmation latency while retaining bounded difficulty swings.
  - **Monetary cap:** 42 million DRM with longer halving intervals to preserve multi-decade emission, versus 21 million BTC.
  - **Launch posture:** No premine, no developer fees, no version-bits governance. All activation parameters are transparent and reproducible from genesis.
  - **Layered design:** Services (P2P/RPC/wallet/indexes) and UI live outside consensus to simplify audits and minimize attack surface.
  - **Mainnet readiness:** Deterministic build instructions, monitoring defaults (Prometheus/Grafana), and reproducible genesis verification scripts accompany the launch materials.

## Cross-Chain Support (Layer 2)
Cross-chain components operate off-consensus. Proof-based adapters validate external chain headers and Merkle proofs to inform relayers and wallets, but Layer 1 state is never altered by cross-chain messages. Relayers are untrusted; users must verify proofs locally.

## Conclusion
Drachma delivers a conservative, auditable proof-of-work system with clear monetary bounds and a minimal feature set. The layered architecture isolates consensus from services and UI, enabling independent review and safe extensibility without compromising core security guarantees.

## Economic Rationale for a 42M Cap
The 42,000,000 DRM maximum supply balances **scarcity** and **transactional utility**:

- A higher unit count than Bitcoin reduces UX friction for retail payments while maintaining scarcity via predictable halvings.
- Halving every ~4 years maintains miner revenue visibility and reduces abrupt security budget drops.
- The cap, coupled with bounded block sizes and conservative fee policies, keeps verification costs low enough for community-operated full nodes, preserving decentralization.

Issuance transparency is enforced in consensus by hard range checks and overflow protections; any block exceeding the cap is invalid.
