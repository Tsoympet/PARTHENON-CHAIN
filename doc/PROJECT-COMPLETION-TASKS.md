# DRACHMA Project Completion Tasks

**Document Date:** January 6, 2026  
**Purpose:** Comprehensive list of all tasks required to complete the DRACHMA blockchain project and prepare for mainnet launch  
**Status:** Consolidated from roadmap.md, LAUNCH-ACTION-ITEMS.md, MAINNET-READINESS.md, AUDIT.md, and other project documentation

---

## Executive Summary

This document consolidates all pending tasks from across the project documentation to provide a single source of truth for what needs to be done to complete the DRACHMA blockchain and prepare for mainnet launch.

**Current Project Status:** Testnet Ready, Approaching Mainnet Readiness  
**Estimated Time to Mainnet:** 8-12 weeks with focused effort  
**Critical Items:** 4 must-complete before launch  
**High Priority Items:** 3 should-complete before launch  
**Medium/Low Priority Items:** Can be completed post-launch

---

## Table of Contents

1. [Critical Pre-Launch Tasks](#critical-pre-launch-tasks)
2. [High Priority Tasks](#high-priority-tasks)
3. [Medium Priority Tasks](#medium-priority-tasks)
4. [Low Priority Tasks](#low-priority-tasks)
5. [Ongoing/Post-Launch Tasks](#ongoingpost-launch-tasks)
6. [Roadmap Timeline](#roadmap-timeline)
7. [Task Dependencies](#task-dependencies)
8. [Resources Required](#resources-required)

---

## Critical Pre-Launch Tasks

These items **MUST** be completed before mainnet launch. Launching without them risks catastrophic failure.

### 1. External Security Audit 🔴 CRITICAL

**Status:** Not started  
**Priority:** HIGHEST  
**Estimated Time:** 4-6 weeks  
**Cost:** $50,000 - $150,000  
**Owner:** Project Lead  
**Source:** LAUNCH-ACTION-ITEMS.md #1, MAINNET-READINESS.md

**Rationale:** No blockchain should launch without independent security review. This protects users and project reputation.

**Action Items:**
- [ ] Engage reputable blockchain security firm (e.g., Trail of Bits, Kudelski, NCC Group)
- [ ] Define audit scope: consensus logic, monetary policy, cryptographic implementations, RPC layer
- [ ] Provide full codebase access and documentation to auditors
- [ ] Address all high/critical severity findings
- [ ] Obtain sign-off from auditors
- [ ] Publish audit report publicly
- [ ] Update AUDIT.md with audit findings and resolutions

**Dependencies:** None - can start immediately  
**Risk if skipped:** Catastrophic exploits, loss of funds, project failure

---

### 2. Genesis Block Finalization 🔴 CRITICAL

**Status:** Partially complete - mainnet nonce shows 0  
**Priority:** HIGHEST  
**Estimated Time:** 1-2 weeks  
**Owner:** Core Developer  
**Source:** LAUNCH-ACTION-ITEMS.md #2, MAINNET-READINESS.md

**Rationale:** Genesis parameters must be perfect - they cannot be changed after launch.

**Current Issues:**
- params.cpp shows `nGenesisNonce = 0` with comment "mined later if not provided"
- mainnet/genesis.json shows specific nonce but needs verification
- Subsidy halving interval discrepancy resolved (now correctly uses 2,102,400 blocks)

**Action Items:**
- [ ] Clarify subsidy halving interval (code and docs now aligned at 2,102,400 blocks)
- [ ] Mine mainnet genesis block with correct parameters
- [ ] Update `layer1-core/consensus/params.cpp` mainParams.nGenesisNonce with mined value
- [ ] Verify genesis hash matches across all configs (mainnet/genesis.json, testnet/genesis.json)
- [ ] Create and test genesis verification script (already exists: scripts/verify-genesis.sh)
- [ ] Run verification script on final genesis parameters
- [ ] Publish genesis block parameters with timestamp
- [ ] Document genesis mining process for transparency

**Dependencies:** None - can start immediately  
**Risk if skipped:** Cannot be fixed after launch, network won't start correctly

---

### 3. RPC Layer Hardening 🔴 CRITICAL

**Status:** Acknowledged as "prototype-grade" in AUDIT.md  
**Priority:** HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** Backend Developer  
**Source:** LAUNCH-ACTION-ITEMS.md #3, AUDIT.md, MAINNET-READINESS.md

**Rationale:** RPC is the main attack surface for public nodes. Must be production-hardened.

**Current Issues (from AUDIT.md):**
> "RPC storage and parsing remain prototype-grade: ReadBlock scans length-prefixed block files linearly and the JSON-RPC parser relies on regex without size limits or checksums."

**Action Items:**
- [ ] Replace linear block file scanning with indexed storage (LevelDB or similar)
- [ ] Implement bounded JSON parser with configurable size limits
- [ ] Add checksums to block storage and RPC responses
- [ ] Implement comprehensive input validation for all RPC methods
- [ ] Add rate limiting to prevent DoS via RPC
- [ ] Write tests for malformed/oversized payload handling
- [ ] Add request timeout handling
- [ ] Implement request queuing with bounded queue size
- [ ] Add RPC call logging for security monitoring
- [ ] Update AUDIT.md to reflect completed hardening
- [ ] Run RPC stress tests using doc/RPC-TESTING.md procedures

**Dependencies:** None - can start immediately  
**Risk if skipped:** Nodes vulnerable to DoS attacks, data corruption, security breaches

---

### 4. Reproducible Builds 🔴 CRITICAL

**Status:** Build process documented, reproducibility not yet implemented  
**Priority:** HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** DevOps/Release Engineer  
**Source:** LAUNCH-ACTION-ITEMS.md #4, MAINNET-READINESS.md

**Rationale:** Users must be able to verify they're running the exact code that was reviewed and tested.

**Action Items:**
- [ ] Document deterministic build process for all platforms (Linux, macOS, Windows)
- [ ] Set up Gitian or similar reproducible build environment
- [ ] Pin all dependency versions explicitly (create requirements.lock or similar)
- [ ] Generate Software Bill of Materials (SBOM) for each release
- [ ] Create GPG signing process for release artifacts
- [ ] Test reproducibility with independent builder
- [ ] Document verification process for end users
- [ ] Add CI workflow for reproducible builds
- [ ] Create release checklist based on doc/assets/release-checklist.md
- [ ] Test binary verification on all platforms

**Dependencies:** BUILD-PROCESS.md already created, needs implementation  
**Risk if skipped:** Users cannot verify binaries, potential for supply chain attacks

---

## High Priority Tasks

These items **SHOULD** be completed before mainnet launch for a professional and stable release.

### 5. Complete GUI Assets 🟡 HIGH

**Status:** Assets sparse - documented but minimal  
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** UI/UX Designer + Frontend Developer  
**Source:** LAUNCH-ACTION-ITEMS.md #5, AUDIT.md

**Current Issue (from AUDIT.md):**
> "GUI assets still sparse: layer3-app/assets/ documents expected icons/legal bundles, but release-ready icons/translations remain minimal."

**Action Items:**
- [ ] Finalize production-ready icon set (all variants: light/dark/high-contrast)
- [ ] Complete asset icons for TLN/DRM/OBL
- [ ] Add all NFT category icons with fallbacks
- [ ] Write and review legal/EULA text
- [ ] Add translations for supported languages (if applicable)
- [ ] Create installer packages:
  - [ ] Windows (.exe/.msi)
  - [ ] macOS (.dmg)
  - [ ] Linux (.AppImage/.deb)
- [ ] Test installers on clean systems for all platforms
- [ ] Update GUI assets README (layer3-app/assets/README.md) with completion status
- [ ] Create UI/UX documentation for wallet features
- [ ] Generate UI snapshots using demo mode

**Dependencies:** None - can start immediately  
**Risk if skipped:** Poor user experience, reduced adoption, unprofessional appearance

---

### 6. Network Infrastructure Setup 🟡 HIGH

**Status:** Testnet infrastructure exists, mainnet needs setup  
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 3-4 weeks  
**Owner:** Infrastructure Team  
**Source:** LAUNCH-ACTION-ITEMS.md #6, MAINNET-READINESS.md

**Rationale:** Network must be stable and observable from day one.

**Action Items:**
- [ ] Coordinate with at least 3 independent seed node operators
- [ ] Verify seed nodes are geographically distributed
- [ ] Update mainnet/seeds.json with confirmed seed nodes
- [ ] Set up monitoring infrastructure (Prometheus + Grafana)
- [ ] Configure alerting for network issues
- [ ] Set up at least one public block explorer
- [ ] Deploy explorer from explorer/ directory
- [ ] Prepare testnet for final stress testing
- [ ] Document seed node requirements and setup (see doc/operators/deployment.md)
- [ ] Create runbook for incident response
- [ ] Set up log aggregation and analysis
- [ ] Configure backup and disaster recovery procedures
- [ ] Test network partition and recovery scenarios

**Dependencies:** Requires coordination with external parties  
**Risk if skipped:** Network instability, poor bootstrap, limited observability

---

### 7. Extended Testnet Validation 🟡 HIGH

**Status:** Testnet running, needs extended validation  
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 2-3 weeks (ongoing)  
**Owner:** QA Team + Community  
**Source:** LAUNCH-ACTION-ITEMS.md #7, TESTING-PROCEDURES.md

**Rationale:** Real-world testing under load reveals issues that unit tests miss.

**Action Items:**
- [ ] Run multi-day stability test on testnet (minimum 7 days)
- [ ] Stress test with realistic transaction load
- [ ] Test wallet recovery scenarios (use doc/WALLET-RECOVERY-TESTING.md)
- [ ] Verify all RPC endpoints with edge cases (use doc/RPC-TESTING.md)
- [ ] Test mining with reference implementations (CPU, CUDA, OpenCL)
- [ ] Simulate network partitions and recovery
- [ ] Test multi-asset transactions (TLN/DRM/OBL)
- [ ] Verify WASM sidechain anchoring
- [ ] Test NFT marketplace functionality
- [ ] Test smart contract deployment and execution
- [ ] Test dApp integration
- [ ] Document all issues found and resolutions
- [ ] Run fuzz testing on critical components
- [ ] Perform load testing on RPC layer
- [ ] Test with various network conditions (high latency, packet loss)

**Dependencies:** Testnet must be running  
**Risk if skipped:** Unknown bugs in production, network instability

---

## Medium Priority Tasks

These items are **RECOMMENDED** before mainnet launch but can be completed shortly after if needed.

### 8. Enhanced Documentation 🟢 MEDIUM

**Status:** Good foundation, needs user-friendly additions  
**Priority:** MEDIUM  
**Estimated Time:** 1-2 weeks  
**Owner:** Technical Writer + Core Team  
**Source:** LAUNCH-ACTION-ITEMS.md #8

**Notes:** Much of this has been completed (FAQ.md, TROUBLESHOOTING.md, MINING-GUIDE.md created)

**Action Items:**
- [x] Create user-friendly quick start guide (exists in README.md)
- [x] Add troubleshooting section to docs (TROUBLESHOOTING.md created)
- [x] Create FAQ based on testnet feedback (FAQ.md created)
- [x] Add miner setup guide with optimization tips (MINING-GUIDE.md created)
- [ ] Document common RPC usage patterns (expand doc/developer-guides/api-reference.md)
- [ ] Create video tutorials (optional but recommended)
- [ ] Review all docs for clarity and accuracy
- [ ] Add more examples to developer guides
- [ ] Create migration guides for users coming from other blockchains
- [ ] Document best practices for node operators
- [ ] Create glossary of terms

**Dependencies:** None  
**Risk if skipped:** More support requests, slower adoption

---

### 9. Community Preparation 🟢 MEDIUM

**Status:** Channels planned, needs execution  
**Priority:** MEDIUM  
**Estimated Time:** Ongoing  
**Owner:** Community Manager  
**Source:** LAUNCH-ACTION-ITEMS.md #9

**Action Items:**
- [ ] Announce launch timeline publicly
- [ ] Set up official communication channels:
  - [ ] Discord server (placeholder exists)
  - [ ] Matrix/IRC bridge (link exists)
  - [ ] Reddit community
  - [ ] GitHub Discussions (already exists)
- [ ] Coordinate with early miners and validators
- [ ] Prepare launch announcement and press materials
- [ ] Create social media presence (X/Twitter account exists)
- [ ] Engage with cryptocurrency media and journalists
- [ ] Set up support channels and documentation
- [ ] Create community guidelines and moderation policies
- [ ] Plan AMA (Ask Me Anything) sessions
- [ ] Prepare educational content about DRACHMA features

**Dependencies:** Launch timeline must be determined  
**Risk if skipped:** Low adoption, missed opportunities

---

### 10. Operational Readiness 🟢 MEDIUM

**Status:** Basic procedures documented, needs formalization  
**Priority:** MEDIUM  
**Estimated Time:** 1-2 weeks  
**Owner:** Operations Team  
**Source:** LAUNCH-ACTION-ITEMS.md #10

**Action Items:**
- [ ] Create incident response plan
- [ ] Establish on-call rotation for launch week
- [ ] Set up log aggregation and analysis
- [ ] Prepare rollback procedures (if possible)
- [ ] Document common failure modes and fixes
- [ ] Create emergency contacts list
- [ ] Plan checkpoint update schedule
- [ ] Create playbooks for common scenarios
- [ ] Set up monitoring dashboards (Prometheus/Grafana)
- [ ] Configure automated alerts
- [ ] Test incident response procedures

**Dependencies:** Network infrastructure  
**Risk if skipped:** Slow response to issues, confusion during incidents

---

## Low Priority Tasks

These items can be completed **POST-LAUNCH** without significant risk.

### 11. Hardware Wallet Support 🔵 LOW

**Status:** Experimental - not production ready  
**Priority:** LOW  
**Estimated Time:** 4-6 weeks  
**Owner:** Wallet Developer  
**Source:** LAUNCH-ACTION-ITEMS.md #11, README.md

**Action Items:**
- [ ] Complete Ledger integration
- [ ] Complete Trezor integration
- [ ] Test with physical devices
- [ ] Update documentation
- [ ] Remove "experimental" warning after thorough testing
- [ ] Add hardware wallet setup guides
- [ ] Test with various firmware versions

**Dependencies:** None - can be done post-launch  
**Risk if skipped:** Advanced users won't have hardware wallet option (acceptable for launch)

---

### 12. Additional Tooling 🔵 LOW

**Status:** Basic tools exist  
**Priority:** LOW  
**Estimated Time:** Ongoing  
**Owner:** Developer Community  
**Source:** LAUNCH-ACTION-ITEMS.md #12

**Action Items:**
- [ ] Create additional block explorers (community-driven)
- [ ] Build wallet libraries for common languages (Python, JavaScript, Go, Rust)
- [ ] Create integration examples and tutorials
- [ ] Build developer SDKs for smart contracts and dApps
- [ ] Create testing tools and frameworks
- [ ] Build monitoring and analytics tools
- [ ] Create backup and recovery tools

**Dependencies:** None - ecosystem development  
**Risk if skipped:** None - ecosystem will develop organically

---

### 13. Bug Bounty Program 🔵 LOW

**Status:** Planned (bounty.json exists)  
**Priority:** LOW (post-launch)  
**Estimated Time:** 2-3 weeks to set up  
**Owner:** Security Team  
**Source:** bounty.json, MAINNET-READINESS.md

**Action Items:**
- [ ] Define bug bounty scope and rules
- [ ] Set reward tiers based on severity
- [ ] Choose platform (Immunefi planned)
- [ ] Allocate budget for rewards
- [ ] Create submission and triage process
- [ ] Launch program with release candidate or mainnet
- [ ] Promote to security researchers

**Dependencies:** Mainnet launch or release candidate  
**Risk if skipped:** May miss some bugs, but acceptable post-launch

---

## Ongoing/Post-Launch Tasks

These tasks are continuous or should be addressed after mainnet launch.

### From Roadmap (doc/reference/roadmap.md)

#### 2025 Q3 — Networking & Wallet Polish
- [ ] Harden P2P validation, DoS protections, and peer diversity heuristics
- [ ] Ship wallet UX refinements (hardware wallet flows, watch-only support, backup UX)
- [ ] Improve mempool fee estimation and rebroadcast policies

#### 2025 Q4 — Mining, Tooling, and Ops
- [ ] Optimize CPU/GPU miners with deterministic build scripts and published benchmarks
- [ ] Publish Prometheus/Grafana dashboards and alert runbooks for node operators
- [ ] Finalize container/systemd deployment profiles for reproducible mainnet installs

#### 2026 Q1 — Testnet v2
- [x] Launch public testnet v2 (COMPLETE)
- [x] Exercise activation/reorg rehearsals
- [ ] Freeze candidate consensus parameters for audits

#### 2026 Q2 — Security Review
- [ ] Commission third-party audits (covered in Critical Task #1)
- [ ] Run fuzzing/adversarial campaigns
- [ ] Post public issue tracker for findings and patch status

#### 2026 Q3 — Release Candidates
- [ ] Produce signed RC binaries and SBOMs
- [ ] Verify deterministic builds across maintainers
- [ ] Document rollback/activation playbooks and backup/restoration drills
- [ ] Run community dry-runs of mainnet deployment scripts

#### 2026 Q4 — Mainnet Launch
- [ ] Execute fair-launch mainnet with open seed lists and monitored explorers
- [ ] Provide 24/7 observability dashboards and incident contacts during launch window
- [ ] Transition to long-term maintenance cadence and public release notes

#### 2026 Q3-Q4 — Sidechain Smart Contracts & Desktop Integration
- [x] Ship mandatory merge-mined WASM sidechain (COMPLETE)
- [x] Deliver desktop GUI features (COMPLETE)
- [ ] Harden checkpoint validation
- [ ] Publish SDKs/examples for WASM dApp builders
- [ ] Complete bridge metrics and SPV proof coverage

#### Post-Launch (Ongoing)
- [ ] Maintain regular documentation cadence
- [ ] Maintain responsible disclosure workflow
- [ ] Track operational metrics (orphan rate, mempool pressure, fee market health)
- [ ] Propose non-consensus mitigations as needed
- [ ] Encourage ecosystem clients to reuse reference implementation

---

## Roadmap Timeline

### Completed ✅
- [x] Implement Layer 1 consensus, UTXO, and PoW with difficulty retargeting
- [x] Harden fork resolution, timestamp bounds, and validation rate limits
- [x] Stand up public testnet with seeds, faucet, explorer, and Qt wallet builds
- [x] Layer 2 services: P2P, mempool, RPC surface, and wallet backend
- [x] Reference miners with Stratum support and benchmarking
- [x] Ship mandatory WASM sidechain with enforced domain law
- [x] Deliver desktop GUI features
- [x] Update max supply to 41M DRM
- [x] Fix critical bugs in emission schedule and halving interval

### In Progress 🔄
- [ ] External audits (consensus, networking, wallet, miners)
- [ ] RPC layer hardening
- [ ] Reproducible builds
- [ ] Network infrastructure coordination

### Pending ⏳
- [ ] Mainnet parameter freeze and launch ceremony
- [ ] Long-term monitoring/observability package for operators
- [ ] Sidechain checkpoint validation hardening
- [ ] WASM SDK and examples

---

## Task Dependencies

```
Genesis Finalization (Task #2)
    ↓
Extended Testnet Validation (Task #7)
    ↓
Security Audit (Task #1)
    ↓
RPC Hardening (Task #3)
Reproducible Builds (Task #4)
GUI Assets (Task #5)
Network Infrastructure (Task #6)
    ↓
Release Candidate
    ↓
Final Validation
    ↓
MAINNET LAUNCH
    ↓
Bug Bounty Program (Task #13)
Community Building (Task #9)
Additional Tooling (Task #12)
```

---

## Resources Required

### Financial Resources
- **Security Audit:** $50,000 - $150,000
- **Infrastructure (6 months):** $5,000 - $10,000 (seed nodes, monitoring, explorer)
- **Bug Bounty (first year):** $10,000 - $50,000
- **Total Estimated:** $65,000 - $210,000

### Human Resources
- **Core Development:** ~500-800 hours
- **Security/Audit:** ~200-300 hours (internal) + external firm
- **Infrastructure/DevOps:** ~100-150 hours
- **Documentation/Community:** ~50-100 hours
- **QA/Testing:** ~150-200 hours

### Timeline
- **Minimum:** 8 weeks (aggressive, high risk)
- **Recommended:** 10-12 weeks (balanced)
- **Conservative:** 14-16 weeks (safest)

---

## Success Criteria for Launch

Before launching mainnet, verify:

- [ ] ✅ External security audit complete with no unresolved critical/high findings
- [ ] ✅ All 97+ tests passing on final release candidate
- [ ] ✅ Genesis block mined and verified across all configurations
- [ ] ✅ RPC layer hardened with indexed storage and input validation
- [ ] ✅ Reproducible builds documented and tested
- [ ] ✅ At least 3 independent seed nodes confirmed and tested
- [ ] ✅ Block explorer operational
- [ ] ✅ Monitoring infrastructure in place with alerts configured
- [ ] ✅ Extended testnet validation completed (minimum 7 days stable operation)
- [ ] ✅ Incident response team and contacts confirmed
- [ ] ✅ Release binaries built, signed, and published
- [ ] ✅ Documentation reviewed and complete
- [ ] ✅ Launch announcement prepared
- [ ] ✅ Community aware and miners coordinated

---

## Related Documents

This document consolidates information from:

1. **doc/reference/roadmap.md** - Project roadmap and timeline
2. **doc/LAUNCH-ACTION-ITEMS.md** - Prioritized launch tasks
3. **doc/MAINNET-READINESS.md** - Technical readiness assessment
4. **doc/security/AUDIT.md** - Known issues and gaps
5. **doc/ANALYSIS-EXECUTIVE-SUMMARY.md** - Recent project analysis
6. **doc/IMPLEMENTATION-SUMMARY.md** - Completed work summary
7. **doc/CONTRIBUTING.md** - Development guidelines
8. **doc/security/audit-checklist.md** - Security audit checklist
9. **doc/assets/release-checklist.md** - Release procedures
10. **README.md** - Project overview

---

## Next Steps

1. **Review this document** with core team and stakeholders
2. **Assign owners** to each critical and high-priority task
3. **Create tracking board** (GitHub Projects, Jira, etc.)
4. **Begin security audit engagement** immediately (longest lead time)
5. **Start RPC hardening work** (can be done in parallel)
6. **Weekly progress reviews** until launch

---

## Updates and Maintenance

**This document should be updated:**
- When tasks are completed
- When new tasks are identified
- When priorities change
- When timelines are adjusted
- After major project milestones

**Last Updated:** January 6, 2026  
**Next Review:** Weekly until launch, then quarterly  
**Maintained By:** Project Lead / Core Team

---

**Questions or Concerns?**  
Reach out to the core team via:
- GitHub Issues: https://github.com/Tsoympet/BlockChainDrachma/issues
- GitHub Discussions: https://github.com/Tsoympet/BlockChainDrachma/discussions
- Email: security@drachma.org
