# docs-site

This directory contains the monorepo documentation site source.

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
