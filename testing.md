# Testing

This repository uses a layered verification bar. The expected checks depend on the subsystem being changed.

## Core Commands

From repository root:

```bash
make all
make tests
make e2e
make ci
make docs-build
```

## What Each Command Covers

- `make all` builds the main `vfo` binary.
- `make tests` builds and runs the service test runner under `services/vfo/test/`.
- `make e2e` runs the cross-cutting profile action and device-conformance flows under `tests/e2e/`.
- `make ci` runs build, tests, and e2e together.
- `make docs-build` regenerates profile docs and builds the MkDocs site.

## Verification Expectations By Change Type

- docs-only changes: keep the scope docs-only and run `make docs-build` when doc generation or navigation is affected
- workflow or automation-only changes: validate conditions, environment assumptions, and comment/update behavior; run targeted local scripts where possible
- C source changes in pipeline logic: run `make tests` at minimum
- profile routing, source, quality, or action-script changes: prefer `make ci`; when that is too heavy locally, call out the gap explicitly
- issue triage, planning, and summarization work: no repository test run is required, but the issue comment must state the expected verification bar for the eventual implementation

## Evidence Rules For Automation

- Triage and summaries must stay grounded in repository files, issue text, workflow data, and test evidence.
- Do not claim a fix is safe unless the required verification bar has been met or the remaining gap is stated plainly.
- If the environment prevents running the intended checks, the output must say which checks were skipped and why.

## Routing Notes

- Lightweight Codex passes should still state the expected verification bar even when they are only scoping or planning the task.
- Escalation to `GPT-5.4 high` is justified when the verification burden is hard to reason about without deeper analysis.
