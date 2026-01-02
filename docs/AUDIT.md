# Repository Audit (Build, Logic, and Missing Components)

This refresh aligns the audit notes with the current codebase and calls out the remaining simplified or risky areas that still need hardening.

## Verified Improvements
- **Build and CI are wired:** The root CMake configuration defines Layer 1/2 libraries, miners, optional GUI, and test executables, while `scripts/test.sh` and the CI workflow configure, build, and run `ctest` on every change. 【F:CMakeLists.txt†L1-L120】【F:scripts/test.sh†L1-L13】【F:.github/workflows/ci.yml†L1-L25】
- **Consensus validation and scripting:** Block/transaction validation enforces coinbase structure, subsidy/fee bounds, UTXO-backed spends, and 32-byte x-only Schnorr pubkeys. Script verification now hashes sanitized inputs per-index and checks 64-byte Schnorr signatures. 【F:layer1-core/validation/validation.cpp†L110-L208】【F:layer1-core/script/interpreter.cpp†L11-L32】
- **Wallet correctness:** Change outputs now return to spendable x-only keys, and signing uses deterministic BIP-340 Schnorr digests rather than HMAC stubs that burned change. Multisig spends reuse the same deterministic signing path. 【F:layer2-services/wallet/wallet.cpp†L165-L210】【F:layer2-services/wallet/wallet.cpp†L328-L359】

## Remaining Gaps / Risks
- **RPC storage and parsing remain prototype-grade:** `ReadBlock` scans length-prefixed block files linearly and the JSON-RPC parser relies on regex without size limits or checksums. Malformed or oversized payloads can exhaust IO/CPU and bypass integrity checks; replace with indexed storage plus a bounded JSON parser. 【F:layer2-services/rpc/rpcserver.cpp†L323-L377】
- **GUI assets still sparse:** `layer3-app/assets/` documents expected icons/legal bundles, but release-ready icons/translations remain minimal. Populate these before shipping installers. 【F:layer3-app/assets/README.md†L1-L13】
