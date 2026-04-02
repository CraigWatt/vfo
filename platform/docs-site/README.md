# docs-site

This directory contains the monorepo documentation site source.

Core pages:

- `docs/pipeline-end-to-end.md`
- `docs/flow-levels.md`
- `docs/vfo-web-app/` (standalone browser app)
- `docs/vfo-web-app-launch.md`
- `docs/profile-visual-standard.md`
- `docs/profiles/generated/*.md` (generated)
- `docs/stylesheets/vfo-theme.css` (brand palette/theme overrides)
- `docs/javascripts/reactflow-viewer.js` (interactive React Flow renderer for docs)
- `docs/javascripts/vfo-web-app.js` (browser dashboard for the demo page)

Note: interactive React Flow docs diagrams load runtime libraries from CDN (`esm.sh` + `unpkg.com`).

Local workflow:

```bash
bash infra/scripts/generate-profile-docs.sh
zensical serve -f mkdocs.yml
```

Build static site:

```bash
bash infra/scripts/generate-profile-docs.sh
zensical build -f mkdocs.yml
```
