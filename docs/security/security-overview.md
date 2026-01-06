# Security Policy

We welcome responsible disclosure and professional security reviews. Please report suspected vulnerabilities **privately** and **do not** open public issues until a coordinated release is agreed.

## Reporting a Vulnerability
- **Primary contact:** security@drachma.org (PGP available on request).
- **Alternative:** Use GitHub's "Report a vulnerability" to open a private Security Advisory.
- **Encrypted channel:** Share your key/fingerprint to arrange encrypted communications if needed.

Include impact, reproduction steps or PoC (logs/seeds), affected commits/tags, environment details, and your suggested disclosure timeline. If you believe consensus or cryptography is affected, mention it explicitly so reviews can be prioritized.

### Response Targets
- Acknowledgment within **48 hours**.
- Triage update within **7 days**.
- Coordinated remediation plan before any public disclosure; fixes for critical issues are expedited and may include backported releases.

### Disclosure Policy
- No public disclosure until a fix, mitigation, or coordinated release date is agreed.
- Maintainers will propose an embargo window that balances user safety and transparency.
- Reporters can opt in to public credit in release notes once the issue is addressed.

## Scope
Consensus code, networking/P2P surfaces, wallet key handling, miners, build/release pipelines, and default configurations are all in scope. Third-party forks or external pools are out of scope.

## Security Resources
- [Security Audit Guide](audit-guide.md)
- [Threat Model](threat-model.md)
- [Fuzzing Playbook](audit-guide.md#fuzzing-and-dynamic-analysis)

### Fuzzing Results
- Run libFuzzer targets with AddressSanitizer/UndefinedBehaviorSanitizer enabled; capture crash logs, minimized reproducers, and coverage summaries.
- Store the latest coverage artifact (e.g., `llvm-cov show` HTML) with the release candidate build; include commands used to generate it.
- File reproducible crashes as private security issues until triaged; link sanitizer output, offending corpus entries, and symbolized stack traces.
- Track outstanding fuzzing issues in release checklists and block releases for unfixed consensus or parser-impacting defects.

Thank you for helping keep DRACHMA users safe.
