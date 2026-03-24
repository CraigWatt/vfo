# Deployment and Runtime Strategy (v1)

This document defines a practical deployment strategy for `vfo` + `vfo-desktop` that avoids toolchain fragmentation while preserving local-media performance and operator control.

## Decision Summary

Use a hybrid model:

- `vfo` CLI remains host-native by default.
- `vfo-desktop` adds an optional managed dependency lane.
- GHCR container remains a reproducible CI/headless lane.

This balances:

- local hardware acceleration and filesystem access
- predictable versions for supportability
- low operational friction for end users

## Goals

- Keep the primary local-library workflow fast and hardware-aware.
- Make dependency readiness explicit (`doctor`, docs, e2e reports).
- Provide a consistent "managed" option without forcing it on every install.
- Keep deployment lanes clear: Releases, Packages, Deployments.

## Non-Goals (v1)

- Full self-contained bundle of all codecs/tools in every release artifact.
- Automatic remote media mounting or network filesystem orchestration.
- Re-implementing media logic in desktop UI/runtime.

## Runtime Modes

### 1) Local Native (default)

Who this is for:

- CLI-first operators with known-good local toolchains.
- Users who want max compatibility with hardware-specific encoders.

Behavior:

- Resolve tools from system paths (`PATH`) and run directly.
- Read/write local media paths directly (for example `/Volumes/...`).
- `doctor` reports presence/absence and versions.

### 2) Desktop Managed Toolchain (optional)

Who this is for:

- Desktop users who want a reproducible dependency set.
- Users who want fewer manual install steps.

Behavior:

- `vfo-desktop` installs and pins vetted tool binaries in an app-managed directory.
- Desktop prepends managed tool dir when launching `vfo` commands.
- Fallback policy is explicit and visible in UI and logs.

Proposed managed root:

- macOS: `~/Library/Application Support/vfo/toolchain/`
- Linux: `~/.local/share/vfo/toolchain/`

### 3) Container (GHCR)

Who this is for:

- CI/CD and automation jobs.
- Headless runtime use where container pull is preferred.

Behavior:

- Use `ghcr.io/<owner>/vfo:<tag>`.
- Best for reproducibility and isolated execution.
- Not the default recommendation for high-touch local-media library operations.

## Toolchain Support Tiers

Define explicit tiers in `doctor`, docs, and release notes.

### Tier `base`

- Required for core pipeline:
  - `ffmpeg`
  - `ffprobe`
  - `mkvmerge`

### Tier `dv`

- Dolby Vision conversion/injection support:
  - `dovi_tool`
  - optionally `mkvextract` where workflow requires it

### Tier `quality`

- Post-profile quality scoring:
  - `ffmpeg` filters for PSNR/SSIM
  - optional `libvmaf` support in ffmpeg build

`doctor` should report tier readiness clearly (pass/warn/fail by feature enabled state).

## Resolution Policy (proposed)

Introduce a consistent policy for command resolution:

- `TOOLCHAIN_MODE=system|managed|auto`

Recommended semantics:

- `system`: only host tools.
- `managed`: only managed tools (hard fail if missing).
- `auto`: prefer managed, then fallback to system.

Resolution should be surfaced in:

- `vfo doctor`
- `vfo show` (effective runtime)
- `vfo status-json` (machine-readable component details)

## Security and Integrity (managed lane)

For desktop-managed installs:

- Pin versions in a manifest committed in repo.
- Download over TLS from vetted sources only.
- Verify checksum before activation.
- Keep staged + active directories to support rollback.
- Log installed versions in a local manifest.

## Filesystem and Access Considerations

### Local native / desktop-managed

- Continue to rely on local OS permissions and explicit folder paths.
- Multi-volume support remains path-based (`MEZZANINE_LOCATIONS`, `SOURCE_LOCATIONS`, profile `*_LOCATIONS`).

### Container

- Require explicit volume mounts for media paths.
- Recommend read/write mount strategy per stage.
- Treat container lane as explicit operator choice, not implicit default.

## CI/CD Mapping

Current lanes:

- Releases: stable/beta tag flows.
- Package: GHCR image build/push.
- Deployments: Pages docs + environment-scoped release publication history.

Recommended additions:

- Keep e2e toolchain reports and publish latest into docs.
- Add "lane suitability" reference to docs so users choose the right mode quickly.

## Implementation Plan

### Phase 1 (now)

- Publish strategy + mode matrix docs.
- Keep CLI default host-native.
- Keep GHCR lane for reproducible headless runtime.
- Keep dependency/version observability in e2e reports.

### Phase 2

- Add desktop-managed toolchain installer and manifest.
- Add runtime mode selector in desktop settings.
- Add surfaced "effective tool source" (managed/system) per dependency.

### Phase 3

- Add policy controls for offline environments and strict pinning.
- Add richer health diagnostics and remediation actions from desktop UI.

## Acceptance Criteria (Phase 1)

- Docs describe all three runtime modes with clear recommendation.
- Docs define support tiers (`base`, `dv`, `quality`) and dependency mapping.
- README links to runtime strategy and deployment-lane docs.
- Desktop README reflects optional managed lane direction without changing engine ownership boundaries.

## Open Questions

- Whether managed lane should support side-by-side ffmpeg variants.
- How strict default fallback policy should be in `auto` mode.
- Whether Windows support should be represented now or deferred until runtime parity work is planned.
