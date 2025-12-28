# Testing the release workflow

The release workflow can now be exercised without creating a real release by using **Run workflow** in GitHub Actions.

1. Navigate to **Actions > Release**.
2. Click **Run workflow** and pick a branch.
3. Set **version_tag** to any test label (for example `v0.0.0-devtest`).
4. Leave **publish_release** as `false` to perform a dry-run build that uploads artifacts but does not publish a GitHub release.
5. Inspect the build matrix results and downloaded artifacts to validate the build.

When you're ready to publish a real release from the UI, rerun the workflow with `publish_release` set to `true` and a final version tag. For production releases, the existing tag push trigger (`v*`) still works as before.
