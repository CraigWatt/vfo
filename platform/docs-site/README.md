# docs-site

This directory contains the monorepo documentation site source.

Core pages:

- `docs/pipeline-end-to-end.md`
- `docs/flow-levels.md`
- `docs/profile-visual-standard.md`
- `docs/profiles/generated/*.md` (generated)

Local workflow:

```bash
bash infra/scripts/generate-profile-docs.sh
mkdocs serve
```

Build static site:

```bash
bash infra/scripts/generate-profile-docs.sh
mkdocs build --strict
```
