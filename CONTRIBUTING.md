# Contributing to DRACHMA

Thank you for your interest in contributing to DRACHMA (DRM). The project prioritizes correctness, reviewability, and long-term maintainability—especially for consensus-critical code in **Layer 1**. Contributions are welcome from everyone, provided they follow the guidelines below.

## Ways to Contribute

- **Bug reports:** Use the issue templates and include reproduction steps, logs, and whether consensus behavior might be affected.
- **Feature requests:** Focus on usability, observability, testing, and tooling. Consensus changes are accepted only with broad review and clear motivation.
- **Pull requests:** Small, well-scoped PRs are easier to review and merge. Draft PRs are encouraged for early feedback.
- **Documentation:** Improvements to guides, specs, and diagrams are highly valued.

## Code of Conduct

By participating, you agree to abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

## Development Workflow

1. Fork the repository and create a topic branch off `main`.
2. Keep commits **small**, **atomic**, and **well-described**. Avoid mixing refactors with behavior changes.
3. Ensure the code builds and tests pass locally. Add or update tests with every behavior change.
4. Submit a pull request using the provided template. Clearly mark whether consensus-critical code is touched.
5. Participate in review. Be prepared to revise; consensus and security reviews can be stringent.

## Review Expectations

- **Layer 1 (Consensus):** Requires multiple reviewers, explicit ACKs/NACKs, and thorough test coverage. Avoid sweeping refactors that obscure logic. Backward compatibility and determinism are mandatory. Changes that affect block/transaction validity, network handshake, or mempool policy must call out risk analysis and migration plans.
- **Layer 2 (Services):** Changes must not alter consensus behavior. Focus on robustness, P2P safety, and DoS resistance.
- **Layer 3 (Desktop):** Prioritize UX clarity and user safety. Avoid adding implicit trust assumptions.
- **Scripts/Miners/Docs:** Ensure transparency and reproducibility. Avoid opaque binaries or vendor-specific dependencies without justification.

## Security-Critical Contributions

Consensus, cryptography, and wallet key-handling changes require extra scrutiny:

- Open a draft PR early with a clear risk section and test plan; flag as **consensus/crypto** in the template.
- Require at least two reviewers familiar with consensus/crypto code, plus fuzz/regression coverage for boundary cases.
- Include test vectors, adversarial scenarios (reorgs, malformed messages), and performance impact notes.
- Expect longer review cycles; avoid combining refactors with behavior changes in these areas.

## Coding Style

- **Language Standard:** C++17 for core components. Use modern idioms conservatively and prefer clarity over cleverness.
- **File Layout:** One class per file when practical; keep headers minimal and include only what you use. Prefer `#pragma once` where supported.
- **Naming:** Use `CamelCase` for classes/types, `snake_case` for functions and variables, `ALL_CAPS` for macros/constants. Avoid Hungarian notation.
- **Formatting:** Follow existing conventions; prefer clang-format where available. Keep line lengths reasonable (≤ 100 columns when practical). Brace style matches the surrounding code (generally `K&R`).
- **Safety:** Avoid global mutable state. Handle errors explicitly; never swallow exceptions or errors silently. Do not wrap imports in try/catch blocks. Avoid implicit conversions; favor `explicit` constructors and `override` where appropriate.
- **Concurrency:** Minimize shared mutable state. Favor RAII for locks; avoid holding locks across network or disk operations when possible. Document lock ordering to prevent deadlocks.
- **Dependencies:** Minimize new dependencies. Any new third-party library requires justification, license compatibility, and deterministic builds.
- **Documentation:** Public APIs and consensus-relevant functions should include comments describing assumptions, preconditions, and failure modes.

## Testing

- Add unit or integration tests alongside code changes (see `tests/`). For consensus logic, include regression vectors and edge cases.
- Run the full test suite and relevant sanitizers when available:
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --parallel
  ctest --test-dir build --output-on-failure
  ```
- For networking or miner changes, include reproducible test vectors or pcap/log snippets when possible.
- Document any platform-specific test coverage (e.g., macOS/Windows/Linux) and simulator/emulator use.

## Commit Guidelines

- Use descriptive commit messages (e.g., `layer1: tighten block header checks`).
- Reference issues when applicable: `Fixes #123`.
- Avoid commits that break bisectability or introduce formatting-only changes mixed with behavior changes.
- **Signing:** Sign commits and tags with a trusted key when possible. Ensure the email matches your GitHub account for verification.

## Pull Requests

- Fill out the PR template completely, including risk assessment and test evidence. State explicitly whether the change is consensus-critical.
- Keep diffs focused; split apart unrelated changes. Large rewrites should be discussed in an issue first.
- CI must pass before merge. If CI is failing for unrelated reasons, note it in the PR and provide local test evidence.
- Maintainers may request additional reviewers for cryptography, consensus, networking, or wallet changes.

## Issue Reporting

- Use issue templates and label the area (Layer 1/2/3, docs, tests, tooling). Include environment details, stack traces, and minimal repro steps.
- Do not report security issues publicly; follow [SECURITY.md](SECURITY.md).

## Security & Consensus Safety

- Treat consensus code as immutable unless there is strong, peer-reviewed rationale.
- Never introduce hidden parameters, soft-forks, or policy changes without documentation and broad review.
- Report vulnerabilities privately per [SECURITY.md](SECURITY.md); do not open public issues.

## Continuous Integration

- CI must pass before merge. If CI is failing for unrelated reasons, note it in the PR and provide local test evidence.

## Documentation Updates

- Update relevant docs (e.g., `docs/`, `README.md`, `scripts/`) when behavior changes.
- Keep diagrams and examples synchronized with code.

Thank you for helping make DRACHMA reliable, auditable, and fair.
