# vfo-desktop

Desktop shell for `vfo` using:

- Tauri for local runtime/process bridge
- React for user interaction
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
- `web/` React UI
- `contracts/` desktop-side command allowlist and IPC payload notes

## Security and safety guardrails

- Never execute arbitrary shell strings from UI input.
- Use explicit command IDs mapped to vetted argument templates.
- Require operator confirmation for destructive actions.
- Keep logs and status output transparent and exportable.
