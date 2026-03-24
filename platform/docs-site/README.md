# docs-site

This directory contains the monorepo documentation site source.

Core pages:

- `docs/pipeline-end-to-end.md`
- `docs/flow-levels.md`
- `docs/profile-visual-standard.md`
- `docs/profiles/generated/*.md` (generated)
- `docs/stylesheets/vfo-theme.css` (brand palette/theme overrides)

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
