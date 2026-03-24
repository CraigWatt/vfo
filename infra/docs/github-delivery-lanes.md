# GitHub Delivery Lanes

This repository now uses all three GitHub delivery primitives where they fit:

- Deployments
- Packages
- Releases

## 1) Deployments

### Docs deployment

- Workflow: `.github/workflows/ci-docs-pages.yml`
- Environment: `github-pages`
- Output: published docs at GitHub Pages

### Release deployment environments

- Stable release workflow: `.github/workflows/on-tags-push.yml`
  - Environment: `release-stable`
- Beta release workflow: `.github/workflows/on-beta-tags-push.yml`
  - Environment: `release-beta`

These environment-scoped release jobs provide explicit deployment history for release publication events.

## 2) Packages

- Workflow: `.github/workflows/ci-package-ghcr.yml`
- Registry: GitHub Container Registry (GHCR)
- Image: `ghcr.io/<owner>/vfo`
- Build source: `infra/packaging/container/Dockerfile`

Package lane intent:

- provide a reproducible containerized vfo runtime
- enable infra/testing integrations that prefer container pull over release-asset download

## 3) Releases

- Stable tags (`vX.Y.Z`): `.github/workflows/on-tags-push.yml`
- Beta tags (`vX.Y.Z-beta.N`): `.github/workflows/on-beta-tags-push.yml`
- Release notes source: `RELEASE.txt`
- Assets: macOS pkg + static build zip

## Why all three lanes are useful together

- Deployments track where/when published artifacts became active (docs and release lanes).
- Packages provide pull-based consumption for automation and infra.
- Releases provide user-facing versioned downloads and changelog context.
