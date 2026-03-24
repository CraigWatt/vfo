# Deployment Modes

`vfo` now has three practical runtime modes. The recommendation is a hybrid strategy so users can choose consistency or performance based on context.

## Recommended Default

- Primary: local native CLI (`vfo` on host machine tools)
- Optional: desktop-managed dependency mode (`vfo-desktop`)
- CI/headless: container package lane (GHCR)

## Mode Matrix

| Mode | Best for | Strengths | Tradeoffs |
| --- | --- | --- | --- |
| Local native | day-to-day media library operations | direct local path access, best hardware acceleration options, no container overhead | user must manage dependencies |
| Desktop managed | users who want pinned/reproducible dependencies | easier onboarding, consistent tool versions, less manual drift | extra desktop implementation + binary management lifecycle |
| Container (GHCR) | CI, automation, headless jobs | reproducible runtime and pull-based consumption | not ideal as the default local-media workflow |

## Dependency Tiers

`vfo doctor` and docs use feature-oriented tiers:

- `base`: `ffmpeg`, `ffprobe`, `mkvmerge`
- `dv`: `dovi_tool` (and related DV utilities where needed)
- `quality`: PSNR/SSIM via ffmpeg filters, optional VMAF via `libvmaf`

## Why Not Bundle Everything by Default?

Fully baking all dependencies into all distributions increases:

- artifact size and update complexity
- platform/architecture-specific packaging overhead
- support burden for multiple bundled codec stacks

The hybrid strategy keeps the core local workflow fast while still offering a managed consistency lane.

## Release, Package, Deployment Lanes

- Releases: user-facing tagged artifacts (stable + beta)
- Package: GHCR container image for automation/headless use
- Deployments: docs + environment-scoped release publication history

See:

- [GitHub Delivery Lanes](https://github.com/CraigWatt/vfo/blob/main/infra/docs/github-delivery-lanes.md)
- [Deployment and Runtime Strategy (v1)](https://github.com/CraigWatt/vfo/blob/main/infra/docs/deployment-runtime-strategy.md)
