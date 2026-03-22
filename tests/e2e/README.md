# E2E Profile Action Tests

This directory contains end-to-end tests for profile action scripts under:

- `services/vfo/actions/`

## What Is Covered

- 1080 profile action output (`transcode_hevc_1080_profile.sh`)
- 4K profile action output (`transcode_hevc_4k_profile.sh`)
- Assertions for:
  - output is readable by `ffprobe`
  - video codec is HEVC
  - resolution stays within profile ceiling
  - audio stream count is preserved

## Running Locally

From repository root:

```bash
make e2e
```

This uses asset mode `auto` by default:

- if `tests/e2e/assets/open-source/` contains media files, those are used as seed inputs
- otherwise synthetic fixtures are generated (CI-safe fallback)

## Force Real Open-Source Local Assets

If you already keep your open-source sample pack locally:

```bash
VFO_E2E_ASSET_MODE=local \
VFO_E2E_ASSETS_DIR="/absolute/path/to/open-source-media" \
VFO_E2E_MAX_SEEDS=4 \
make e2e
```

## Useful Environment Variables

- `VFO_E2E_ASSET_MODE`: `auto` (default), `local`, `synthetic`
- `VFO_E2E_ASSETS_DIR`: media directory used by `auto/local` modes
- `VFO_E2E_CLIP_DURATION`: fixture clip duration in seconds (default: `2`)
- `VFO_E2E_MAX_SEEDS`: number of local seed assets to process per run (default: `1`)
- `VFO_E2E_KEEP_TMP=1`: keep `tests/e2e/.tmp/` for debugging

## GitHub Actions (Self-Hosted Media Runner)

Use workflow:

- `.github/workflows/on-self-hosted-e2e.yml`

It targets runner labels:

- `self-hosted`
- `macOS`
- `vfo-media`

This workflow now runs in two ways:

- automatically on same-repo PRs to `main` and on `main` pushes (uses workflow defaults)
- manually via `workflow_dispatch` (override inputs as needed)

Trigger manually in Actions and provide:

- `assets_dir`: absolute path on the runner machine where your open-source media is mounted
- `clip_duration`: optional fixture duration override
- `max_seeds`: optional count of media assets to exercise in one run (default: `4`)
