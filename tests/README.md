# Cross-Cutting Tests

This directory contains tests that span more than one service or layer.

## Put here

- End-to-end scenarios across services
- System-level integration checks
- Regression suites that validate complete user journeys

## Suggested layout

- `tests/e2e/` for end-to-end test suites

## Current E2E entrypoint

- `tests/e2e/run_profile_actions_e2e.sh`

From repository root:

```bash
make e2e
```

This supports both:

- local real-media mode (`VFO_E2E_ASSET_MODE=local`, `VFO_E2E_ASSETS_DIR=/path/to/media`)
- hosted CI smoke mode (`auto` fallback to generated fixtures)

CI lane mapping:

- `Validate (PR + main)`: hosted synthetic e2e smoke + unit tests (`make ci`)
- `Full E2E (self-hosted media)`: full media-backed e2e on self-hosted runner (auto on same-repo PRs to `main` + `main` pushes, manual dispatch supported)
