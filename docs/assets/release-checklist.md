# Release Checklist

Use this checklist when cutting a new tag (e.g., `v0.1.0-rc`). Copy into the GitHub Release body or PR description as needed.

## Preparation
- [ ] Decide scope and confirm CHANGELOG.md is updated.
- [ ] Bump versions in code/configs if required; tag commit with `vX.Y.Z[-rc|-testnet]`.
- [ ] Ensure build environment is reproducible (pinned toolchains, clean build directory).

## Build and signing
- [ ] Build Release binaries for Linux, Windows, and macOS (see `../getting-started/building.md`).
- [ ] Package installers/bundles (tar.gz, AppImage, zip/exe, dmg) and generate `checksums.txt`.
- [ ] Generate SBOM (e.g., `syft packages dir:. -o json > sbom.json`).
- [ ] Sign artifacts and tag with your maintainer GPG key. Required secrets for CI signing: `GPG_KEY` (ASCII-armored) and `GPG_PASSPHRASE` (optional).

## Verification
- [ ] Verify hashes on a second machine/maintainer build; compare deterministic outputs when possible.
- [ ] Smoke-test node, wallet, and miners on testnet; confirm wallet connects to the freshly built node.
- [ ] Re-run unit/integration tests if enabled.

## Publish
- [ ] Create a GitHub Release for the tag with:
  - [ ] Summary of changes (use `CHANGELOG.md`).
  - [ ] Upload binaries/installers, `checksums.txt`, SBOM, and signatures.
  - [ ] Mark as pre-release for `-rc`/`-testnet` tags.
- [ ] Update docs/website links if download URLs changed.
- [ ] Publish container images to GitHub Packages (GHCR):
  - [ ] Choose **Container registry (ghcr.io)** when prompted to publish the first package.
  - [ ] Tag images as `ghcr.io/<org>/drachmad:<tag>` and `ghcr.io/<org>/drachma-wallet:<tag>` (matching the Git tag).
  - [ ] Push with `docker login ghcr.io` + `docker push`; confirm the packages appear under the **Packages** tab.

## Announce
- [ ] Post release notes and download links to Matrix/Discord/Reddit/X.
- [ ] Share signatures and checksums; remind users to verify before running.
- [ ] Track feedback and issues for quick patch releases.
