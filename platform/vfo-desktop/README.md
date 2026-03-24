# vfo-desktop

Desktop shell for `vfo` using:

- Tauri for local runtime/process bridge
- React for user interaction
- React Flow for interactive pipeline/status canvases
- `vfo` CLI as the only media-processing engine

## Architecture contract

- `services/vfo/` owns all media logic and decisioning.
- `platform/vfo-desktop/` must not re-implement profile/scenario rules.
- UI calls a constrained command bridge, then renders results.
- Machine-readable CLI outputs are the integration contract.

## Command lanes (first version)

- `vfo --version`
- `vfo doctor`
- `vfo show`
- `vfo status`
- `vfo status-json`
- `vfo wizard`
- `vfo mezzanine-clean`
- `vfo run`
- `vfo visualize`

## Directory scaffold

- `src-tauri/` runtime host, process bridge, permission model
- `web/` React + React Flow UI
- `contracts/` desktop-side command allowlist and IPC payload notes

## Web shell quick run

```bash
cd platform/vfo-desktop/web
npm install
npm run dev
```

## Security and safety guardrails

- Never execute arbitrary shell strings from UI input.
- Use explicit command IDs mapped to vetted argument templates.
- Require operator confirmation for destructive actions.
- Keep logs and status output transparent and exportable.

## Managed toolchain direction (planned)

`vfo-desktop` is the right place to add an optional managed dependency lane:

- install/pin vetted tool binaries in an app-managed directory
- prefer managed tools in desktop-launched runs (policy-driven fallback)
- surface effective tool source and versions in UI + doctor/status views

The CLI engine remains authoritative for all media logic and decisions; desktop only manages runtime ergonomics.
