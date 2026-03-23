# vfo Service

This is the primary application service in this repository.

## Contents

- `src/` application source code and default `vfo_config.conf`
- `test/` unit and integration-style test code
- `actions/` reusable profile action scripts
- `docs/` service-specific design and configuration docs (including device conformance notes)
- `examples/` sample config snippets and usage templates
- `presets/` stock profile packs (for example `balanced_open_audio`, `device_targets_open_audio`)
- `bin/` compiled binaries
- `lib/` build artifacts (`.o` files)
- `log/` runtime and tooling logs

## Build

From the repository root:

```bash
make all
make tests
make e2e
make ci
```

Or directly from this service:

```bash
make -C services/vfo all
make -C services/vfo tests
```

Cross-cutting E2E tests live under `tests/e2e/` and are run from repository root.

## Runtime helpers

- `vfo wizard` walks users through creating/updating `vfo_config.conf` and supports stock preset pack multi-select.
- `vfo show` prints the active config in a readable summary view.
- `vfo doctor` validates toolchain and configured paths.
- `vfo status` prints component/stage readiness snapshot for the vfo engine.
- `vfo status-json` prints machine-readable status.
- `vfo visualize` generates local workflow artifacts (`status.json`, `workflow.mmd`, `index.html`) and supports optional `--open`.
- `vfo mezzanine-clean` audits or applies mezzanine folder/filename hygiene (no transcode/remux).
- `vfo run` executes the default pipeline (mezzanine -> source if enabled -> profiles -> optional quality scoring) and pre-checks required dependencies.
- `MEZZANINE_LOCATIONS`, `SOURCE_LOCATIONS`, and `<PROFILE>_LOCATIONS` support semicolon-separated multi-drive targets with per-location caps via `*_LOCATION_MAX_USAGE_PCT`.
- `QUALITY_CHECK_*` config keys control post-profile PSNR/SSIM scoring, optional VMAF, thresholds, and strict gating.

Status schema/details are documented in `docs/status-observability.md`.
Mezzanine hygiene behavior is documented in `docs/mezzanine-clean.md`.
Quality scoring behavior is documented in `docs/quality-scoring.md`.
Workflow progression map is documented in `docs/workflow-engine-progression.md`.
Workflow visualization command is documented in `docs/workflow-visualization.md`.
