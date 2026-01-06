# DRACHMA Cross-Chain Framework (Layer 2)

## Scope
Cross-chain features are strictly non-consensus. The core chain state, supply, and validation rules remain unchanged regardless of external chain interactions. Relayers and adapters run in Layer 2 and provide proofs to wallets and monitoring tools.

## Components
 - **Adapters:** Pluggable validators for external chains (e.g., Bitcoin-like or WASM-light header formats). They verify headers and Merkle proofs to attest to external events.
- **Relayer:** Untrusted service that fetches external data, verifies proofs via adapters, and queues messages for local consumers.
- **Bridge Manager:** Routes verified messages to interested subsystems (wallet UI, auditing tools) without mutating Layer 1 state.
- **Cross-chain Messages:** Structured payloads containing the external header chain context, Merkle branches, and event metadata.

## Trust Model
- Relayers are untrusted; all data must include sufficient proof material for independent verification.
- Wallets and services MUST validate proofs locally using deterministic adapters.
- No minted assets, wrapped tokens, or consensus-affecting pegs exist. Cross-chain information is advisory only.

## Verification Flow
1. Relayer retrieves external block headers and the associated Merkle proof for an event (transaction or log).
2. Adapter validates the external header chain (proof-of-work/slot rules) and checks the Merkle branch against the referenced root.
3. Upon success, the message is queued for consumers. Failure results in rejection with detailed error codes for auditability.
4. Consumers decide how to act (display status, trigger monitoring alerts, or prepare off-chain workflows). Consensus state is never mutated.

## Security Considerations
- Keep adapter logic deterministic and side-effect-free; proofs must be reproducible.
- Deny messages lacking sufficient confirmations as configured by local policy.
- Bound message queues to resist flooding.
- Persist relay logs for post-mortem analysis; do not treat cross-chain assertions as authoritative without proof.
