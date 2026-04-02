# web

React web shell for `vfo-desktop` (Tauri frontend).

The wireframe layout is intentionally shared in spirit with the docs-site demo page under `platform/docs-site/docs/vfo-web-app.md`.

## Current v1

- Vite + React scaffold
- three-column dashboard wireframe
- assets rail, workflow lane, and inspector panel
- React Flow still available for more detailed pipeline canvases

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
- mirrored browser demo surface for the docs-site web app page

## Non-goals

- no direct FFmpeg command authoring logic in UI runtime
- no independent rules engine separate from `vfo`
