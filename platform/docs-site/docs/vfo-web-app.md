# VFO Web App Demo

This page is the browser-friendly companion to `vfo-desktop`.

- it mirrors the desktop wireframe for convenient sharing
- it can render the latest available e2e-backed dashboard artifact when present
- it falls back to a built-in demo payload so the page is always viewable on GitHub Pages

Use it as a lightweight prospect/demo view and as a convenient check that the current pipeline shape still reads well in a browser.

## Demo Dashboard

<div
  class="vfo-web-app"
  data-vfo-web-app
  data-vfo-web-app-src="../e2e-toolchain-artifacts/vfo-web-app.json"
></div>

## How It Works

- the left rail shows singular assets from the mezzanine folder
- the center lane shows the left-to-right workflow
- the right rail shows the selected node output in a code-style inspector
- the lower band shows summary totals and stage counts

If the latest e2e artifact is available, the dashboard uses it as its source metadata and header context.
Otherwise it uses demo content that matches the desktop wireframe layout.
