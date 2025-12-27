# Cross-Chain Layer (Non-Consensus)

This directory provides a conservative, proof-driven cross-chain framework. It intentionally avoids any consensus coupling or token minting logic while enabling external state to be referenced by higher layers.

* `messages/` defines deterministic message formats used by adapters and bridges.
* `validation/` contains a basic SPV-style proof validator that walks chained block headers using double-SHA256 hashing.
* `bridge/` keeps per-chain tips and queues verified messages for consumption by the application layer.
* `relayer/` is an untrusted component that forwards proofs and messages into the bridge after local verification.

All components are synchronous, auditable, and avoid mutable global state to keep cross-chain handling reviewable and deterministic.
