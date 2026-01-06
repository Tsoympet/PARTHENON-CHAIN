# DRACHMA Mainnet Launch - Prioritized Action Items

**Date:** January 6, 2026  
**Purpose:** Executive summary of critical actions needed before mainnet launch

---

## 🔴 CRITICAL (Must Complete Before Launch)

### 1. External Security Audit
**Priority:** HIGHEST  
**Estimated Time:** 4-6 weeks  
**Owner:** Project Lead

**Actions:**
- [ ] Engage reputable blockchain security firm (e.g., Trail of Bits, Kudelski, NCC Group)
- [ ] Focus audit scope on: consensus logic, monetary policy, cryptographic implementations, RPC layer
- [ ] Provide full codebase access and documentation
- [ ] Address all high/critical severity findings
- [ ] Obtain sign-off from auditors
- [ ] Publish audit report publicly

**Rationale:** No blockchain should launch without independent security review. This protects users and project reputation.

---

### 2. Genesis Block Finalization
**Priority:** HIGHEST  
**Estimated Time:** 1-2 weeks  
**Owner:** Core Developer

**Actions:**
- [ ] Clarify subsidy halving interval (210,000 vs 2,102,400 blocks - code and docs disagree)
- [ ] Mine mainnet genesis block with correct parameters
- [ ] Update `layer1-core/consensus/params.cpp` mainParams.nGenesisNonce with mined value
- [ ] Verify genesis hash matches across all configs
- [ ] Create and test genesis verification script
- [ ] Publish genesis block parameters with timestamp

**Current Issue:** 
- params.cpp shows `nGenesisNonce = 0` with comment "mined later if not provided"
- mainnet/genesis.json shows specific nonce but this needs verification
- Halving interval discrepancy needs resolution

**Rationale:** Genesis parameters must be perfect - they cannot be changed after launch.

---

### 3. RPC Layer Hardening
**Priority:** HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** Backend Developer

**Actions:**
- [ ] Replace linear block file scanning with indexed storage (LevelDB or similar)
- [ ] Implement bounded JSON parser with configurable size limits
- [ ] Add checksums to block storage and RPC responses
- [ ] Implement comprehensive input validation for all RPC methods
- [ ] Add rate limiting to prevent DoS via RPC
- [ ] Write tests for malformed/oversized payload handling
- [ ] Update AUDIT.md to reflect completed hardening

**Current Issue (from AUDIT.md):**
> "RPC storage and parsing remain prototype-grade: ReadBlock scans length-prefixed block files linearly and the JSON-RPC parser relies on regex without size limits or checksums."

**Rationale:** RPC is the main attack surface for public nodes. Must be production-hardened.

---

### 4. Reproducible Builds
**Priority:** HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** DevOps/Release Engineer

**Actions:**
- [ ] Document deterministic build process for all platforms (Linux, macOS, Windows)
- [ ] Set up Gitian or similar reproducible build environment
- [ ] Pin all dependency versions explicitly
- [ ] Generate Software Bill of Materials (SBOM) for each release
- [ ] Create GPG signing process for release artifacts
- [ ] Test reproducibility with independent builder
- [ ] Document verification process for end users

**Rationale:** Users must be able to verify they're running the exact code that was reviewed and tested.

---

## 🟡 HIGH PRIORITY (Should Complete Before Launch)

### 5. Complete GUI Assets
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 2-3 weeks  
**Owner:** UI/UX Designer + Frontend Developer

**Actions:**
- [ ] Finalize production-ready icon set (all variants: light/dark/high-contrast)
- [ ] Complete asset icons for TLN/DRM/OBL
- [ ] Add all NFT category icons with fallbacks
- [ ] Write and review legal/EULA text
- [ ] Add translations for supported languages (if applicable)
- [ ] Create installer packages for Windows (.exe/.msi), macOS (.dmg), Linux (.AppImage/.deb)
- [ ] Test installers on clean systems
- [ ] Update GUI assets README with completion status

**Current Issue (from AUDIT.md):**
> "GUI assets still sparse: layer3-app/assets/ documents expected icons/legal bundles, but release-ready icons/translations remain minimal."

**Rationale:** Professional appearance matters for user trust and adoption.

---

### 6. Network Infrastructure Setup
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 3-4 weeks  
**Owner:** Infrastructure Team

**Actions:**
- [ ] Coordinate with at least 3 independent seed node operators
- [ ] Verify seed nodes are geographically distributed
- [ ] Set up monitoring infrastructure (Prometheus + Grafana)
- [ ] Configure alerting for network issues
- [ ] Set up at least one public block explorer
- [ ] Prepare testnet for final stress testing
- [ ] Document seed node requirements and setup
- [ ] Create runbook for incident response

**Rationale:** Network must be stable and observable from day one.

---

### 7. Extended Testnet Validation
**Priority:** MEDIUM-HIGH  
**Estimated Time:** 2-3 weeks (ongoing)  
**Owner:** QA Team + Community

**Actions:**
- [ ] Run multi-day stability test on testnet
- [ ] Stress test with realistic transaction load
- [ ] Test wallet recovery scenarios
- [ ] Verify all RPC endpoints with edge cases
- [ ] Test mining with reference implementations
- [ ] Simulate network partitions and recovery
- [ ] Test multi-asset transactions
- [ ] Verify WASM sidechain anchoring
- [ ] Document all issues found and resolutions

**Rationale:** Real-world testing under load reveals issues that unit tests miss.

---

## 🟢 MEDIUM PRIORITY (Recommended Before Launch)

### 8. Enhanced Documentation
**Priority:** MEDIUM  
**Estimated Time:** 1-2 weeks  
**Owner:** Technical Writer + Core Team

**Actions:**
- [ ] Create user-friendly quick start guide
- [ ] Add troubleshooting section to docs
- [ ] Create FAQ based on testnet feedback
- [ ] Add miner setup guide with optimization tips
- [ ] Document common RPC usage patterns
- [ ] Create video tutorials (optional but recommended)
- [ ] Review all docs for clarity and accuracy

**Rationale:** Better documentation = fewer support requests and faster adoption.

---

### 9. Community Preparation
**Priority:** MEDIUM  
**Estimated Time:** Ongoing  
**Owner:** Community Manager

**Actions:**
- [ ] Announce launch timeline publicly
- [ ] Set up official communication channels (Discord/Matrix/Forum)
- [ ] Coordinate with early miners and validators
- [ ] Prepare launch announcement and press materials
- [ ] Create social media presence
- [ ] Engage with cryptocurrency media
- [ ] Set up support channels

**Rationale:** Successful launch needs community awareness and participation.

---

### 10. Operational Readiness
**Priority:** MEDIUM  
**Estimated Time:** 1-2 weeks  
**Owner:** Operations Team

**Actions:**
- [ ] Create incident response plan
- [ ] Establish on-call rotation for launch week
- [ ] Set up log aggregation and analysis
- [ ] Prepare rollback procedures (if possible)
- [ ] Document common failure modes and fixes
- [ ] Create emergency contacts list
- [ ] Plan checkpoint update schedule

**Rationale:** Be prepared for issues - they will happen.

---

## 🔵 NICE TO HAVE (Post-Launch Acceptable)

### 11. Hardware Wallet Support
**Priority:** LOW (Currently marked experimental)  
**Estimated Time:** 4-6 weeks  
**Owner:** Wallet Developer

**Actions:**
- [ ] Complete Ledger integration
- [ ] Complete Trezor integration
- [ ] Test with physical devices
- [ ] Update documentation
- [ ] Remove "experimental" warning after thorough testing

---

### 12. Additional Tooling
**Priority:** LOW  
**Estimated Time:** Ongoing  
**Owner:** Developer Community

**Actions:**
- [ ] Create additional block explorers
- [ ] Build wallet libraries for common languages
- [ ] Create integration examples
- [ ] Build developer SDKs

---

## Summary Timeline

### Week 1-2: Immediate Actions
- Start security audit engagement
- Begin RPC hardening work
- Clarify and finalize genesis parameters
- Set up infrastructure coordination

### Week 3-4: Core Development
- Continue security audit
- Complete RPC hardening
- Start reproducible builds setup
- Begin extended testnet validation

### Week 5-6: Integration & Testing
- Address audit findings
- Complete GUI assets
- Finalize network infrastructure
- Continue testnet stress testing

### Week 7-8: Pre-Launch
- Security audit sign-off
- Final genesis block verification
- Release candidate testing
- Launch coordination

### Week 9-10: Launch Preparation
- Code freeze (critical fixes only)
- Final testing and verification
- Launch announcement
- Incident response team ready

### Week 11-12: Buffer & Launch
- Final checks
- **MAINNET LAUNCH**
- Intensive monitoring
- Rapid response to issues

---

## Risk Assessment

### High Risk if Skipped:
1. **Security Audit** - Could result in catastrophic exploits
2. **Genesis Finalization** - Cannot be fixed after launch
3. **RPC Hardening** - Nodes vulnerable to DoS attacks

### Medium Risk if Skipped:
4. **Reproducible Builds** - Users cannot verify binaries
5. **GUI Completion** - Poor user experience, adoption issues
6. **Network Infrastructure** - Network instability, poor bootstrap

### Low Risk if Skipped:
7. **Enhanced Docs** - Frustration but not blocking
8. **Hardware Wallets** - Advanced feature, can come later
9. **Additional Tooling** - Ecosystem development can follow launch

---

## Budget Estimates (Rough)

- **Security Audit:** $50,000 - $150,000 (depending on firm and scope)
- **Infrastructure (6 months):** $5,000 - $10,000 (seed nodes, monitoring, explorer)
- **Development Time:** ~500-800 hours of focused engineering work
- **Total Timeline:** 10-12 weeks with dedicated resources

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

## Conclusion

DRACHMA is **well-architected and approaching readiness**, but rushing to mainnet would be a mistake. The items above represent **industry-standard due diligence** for blockchain launches.

**Recommendation:** Follow the 10-12 week timeline above, focusing first on the CRITICAL items (security audit, genesis finalization, RPC hardening, reproducible builds).

With proper execution of this plan, DRACHMA can launch with confidence as a secure, reliable, production-ready blockchain.

---

**Next Steps:**
1. Review this document with core team
2. Assign owners to each action item
3. Create project tracking board (GitHub Projects, Jira, etc.)
4. Begin immediately with security audit engagement
5. Weekly progress reviews until launch

**Questions or Concerns?**  
Reach out to the core team via the project's communication channels.
