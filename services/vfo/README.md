# vfo Service

This is the primary application service in this repository.

## Contents

- `src/` application source code and default `vfo_config.conf`
- `test/` unit and integration-style test code
- `actions/` reusable profile action scripts
- `docs/` service-specific design and configuration docs
- `examples/` sample config snippets and usage templates
- `presets/` stock profile packs (for example `balanced_open_audio`)
- `bin/` compiled binaries
- `lib/` build artifacts (`.o` files)
- `log/` runtime and tooling logs

## Build

From the repository root:

```bash
make all
make tests
```

Or directly from this service:

```bash
make -C services/vfo all
make -C services/vfo tests
```

## Runtime helpers

- `vfo doctor` validates toolchain and configured paths.
- `vfo run` executes the default pipeline (mezzanine -> source if enabled -> profiles).
