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

- `vfo wizard` walks users through creating/updating `vfo_config.conf`.
- `vfo show` prints the active config in a readable summary view.
- `vfo doctor` validates toolchain and configured paths.
- `vfo run` executes the default pipeline (mezzanine -> source if enabled -> profiles) and pre-checks required dependencies.
