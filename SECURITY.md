# Security Policy

## Reporting a Vulnerability

We take security and consensus safety seriously. Please report suspected vulnerabilities **privately** and **do not** open public issues.

- **Preferred contact:** security@drachma.org (optionally encrypted; request the GPG fingerprint in your initial email)
- **Alternate:** GitHub "Report a vulnerability" (Security tab) to open a private advisory with maintainers
- **Information to include:**
  - A description of the issue and potential impact (especially consensus divergence or DoS)
  - Minimal reproduction steps, logs, test vectors, or proof-of-concept if available
  - Affected versions/commit hashes and environment details
  - Whether the issue has been disclosed elsewhere

## Responsible Disclosure Process

1. Report the issue privately via email or GitHub advisory.
2. We will acknowledge receipt within **3 business days** and assign a tracking ID.
3. Maintainers will triage severity and impacted components (consensus, networking, wallet, miners, build, docs).
4. You will receive periodic updates (at least weekly) until resolution. For consensus-affecting bugs, expect additional peer review.
5. Coordinated disclosure timelines will be agreed upon before public release. We aim to publish fixes within **30 days** for critical issues, but may defer public details to protect users.
6. Once remediated, we will publish release notes and advisories summarizing impact, fixes, and mitigations.

## Bounties and Acknowledgment

- We do not operate a formal monetary bounty program at this time.
- We are happy to acknowledge reporters in release notes and advisories (with consent). Anonymous reporting is respected.

## Scope

- Consensus logic, networking, wallet services, miners, build scripts, release artifacts, and configuration defaults.
- Documentation that could mislead users in a way that impacts security.

## Out of Scope

- Third-party pools or forks not maintained in this repository.
- Hypothetical attacks without actionable details.
- Vulnerabilities requiring root/administrator access on a fully compromised host without network implications.

Thank you for helping keep DRACHMA users safe.
