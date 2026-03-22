# vfo Status Observability

`vfo` now exposes a component-level status layer so users and CI can inspect readiness without running full transcode execution.

## Commands

- `vfo status`: human-readable status table.
- `vfo status-json`: machine-readable JSON payload.
- `vfo status_json`: legacy alias for compatibility.

Diagnostic commands (`status`, `status-json`, `doctor`, `show`) use lenient location validation, so missing paths are reported in status output instead of failing early during config load.

## State Model

Each component reports one state:

- `pending`
- `in_progress`
- `complete`
- `error`
- `skipped`

Status health is derived from this model:

- A report is `healthy=true` when no component is in `error`.
- A report is `healthy=false` when at least one component is in `error`.

## Current Component Keys

Snapshot command (`vfo status` / `vfo status-json`) includes:

- `engine.snapshot`
- `config.directory`
- `dependency.ffmpeg`
- `dependency.ffprobe`
- `dependency.mkvmerge`
- `dependency.dovi_tool` (optional dependency, usually `skipped` when missing)
- `storage.original[<n>]`
- `storage.source[<n>]`
- `profiles.detected`
- `profile.<profile_name>.locations[<n>]`
- `profile.<profile_name>.scenarios`
- `stage.mezzanine`
- `stage.source`
- `stage.profiles`
- `stage.execute`

Run command (`vfo run`) emits stage progress transitions for:

- `engine.run`
- `stage.mezzanine`
- `stage.source`
- `stage.profiles`

## JSON Contract

`vfo status-json` output shape:

```json
{
  "schema_version": 1,
  "healthy": true,
  "components": [
    {
      "name": "dependency.ffmpeg",
      "state": "complete",
      "detail": "ffmpeg available"
    }
  ],
  "summary": {
    "pending": 1,
    "in_progress": 0,
    "complete": 7,
    "error": 0,
    "skipped": 1
  }
}
```

## CI/Test Usage

Example gating:

```bash
vfo status-json | jq -e '.healthy == true and .summary.error == 0'
```

This keeps readiness checks decoupled from full encode execution and gives tests a stable status contract to assert against.
