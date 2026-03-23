# vfo Workflow Visualization

`vfo visualize` generates local workflow artifacts that reflect current runtime readiness and stage state.

## Command

```bash
vfo visualize
vfo visualize --open
```

## Output

Each run writes a timestamped folder under:

- `<config_dir>/visualizations/<run_id>/`
- fallback: `/tmp/vfo-visualizations/<run_id>/` when config directory is not writable

Generated artifacts:

- `status.json`: machine-readable status snapshot
- `workflow.mmd`: Mermaid flowchart with stage state styling
- `index.html`: self-contained local report (pipeline cards + component table)

## Browser behavior

- default: `vfo visualize` **does not** launch a browser
- optional: pass `--open` to attempt opening `index.html`
  - macOS: uses `open`
  - Linux: uses `xdg-open`

No standalone frontend application is required.

## BPMN / DMN models

Formal models shipped with vfo:

- `services/vfo/docs/workflow-engine.bpmn`
- `services/vfo/docs/workflow-decisions.dmn`

These model files are intended for import into BPMN/DMN tooling and documentation workflows.
