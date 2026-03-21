# vfo Service

This is the primary application service in this repository.

## Contents

- `src/` application source code and default `vfo_config.conf`
- `test/` unit and integration-style test code
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
