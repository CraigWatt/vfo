# web

React web shell for `vfo-desktop` (Tauri frontend).

## Current v1

- Vite + React scaffold
- React Flow canvas for pipeline/status views
- preset switcher (`executive`, `operator`, `engine`)

## Run locally

```bash
cd platform/vfo-desktop/web
npm install
npm run dev
```

## Responsibilities

- onboarding UX (`quickstart` and `advanced` wizard entry points)
- readiness and run-state views (doctor/status/status-json)
- run controls (`run`, `mezzanine-clean`, `visualize`)
- profile and output observability surfaces

## Non-goals

- no direct FFmpeg command authoring logic in UI runtime
- no independent rules engine separate from `vfo`
