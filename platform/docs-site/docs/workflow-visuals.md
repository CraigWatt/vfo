# Workflow Visuals

vfo includes local visualization artifacts from the CLI:

```bash
vfo visualize
vfo visualize --open
```

Generated assets include:

- runtime status JSON snapshot
- mermaid graph output
- HTML report for quick browsing

## How This Relates To Docs Flow Charts

- `vfo visualize` reflects your **current runtime state**.
- docs-site flow pages reflect **reference operating models** for onboarding and profile understanding.
- use both together: docs for expected behavior, `vfo visualize` for current run behavior.

Suggested sequence:

1. read [Pipeline](pipeline-end-to-end.md) and [Flow Levels](flow-levels.md)
2. check profile-specific docs in [Profile Info Sheets](profiles/index.md)
3. run `vfo status-json` and `vfo visualize` on your machine to compare expectation vs current state

You can also reference formal workflow models in-repo:

- `services/vfo/docs/workflow-engine.bpmn`
- `services/vfo/docs/workflow-decisions.dmn`

These complement (not replace) the generated profile info sheets in this docs site.
