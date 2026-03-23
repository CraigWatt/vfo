# Codex Autonomous Repo Loop (v1)

This repository ships a small, bounded automation loop powered by the official Codex GitHub Action.

## Goals Covered

- scheduled proactive sweep
- issue trigger via `agent-ready`
- bounded CI autofix for Codex-owned PRs
- explicit guardrails and easy-disable controls

## Workflows

### 1) `ci-codex-autonomous-loop.yml`

Triggers:

- schedule: weekday sweep
- manual dispatch: sweep
- issue label event: `agent-ready`

Behavior:

- **Proactive sweep**: runs Codex in read-only mode and posts/updates an issue titled `[codex] Proactive repository sweep`.
- **Issue execution**: when an issue is labeled `agent-ready`, Codex works from that issue, runs `make tests`, and only opens a PR if verification passes.

### 2) `ci-codex-pr-autofix.yml`

Trigger:

- `workflow_run` completion of `Validate (PR + main)` when conclusion is `failure` for pull requests.

Behavior:

- Only targets **Codex-owned** PRs:
  - head branch starts with `codex/`, or
  - PR has label `codex-owned`
- Runs Codex with strict intent: minimal CI fix only.
- Enforces bounds:
  - max 12 changed files
  - blocks edits to release/deploy workflows
  - requires `make ci` to pass before push
- Adds a marker comment per SHA to prevent repeated autofix attempts on the same failing commit.

## Safety Guardrails

- No auto-merge logic.
- No production deploy automation.
- No release workflow mutation from autofix lane.
- Fork PRs are excluded from autofix push behavior.
- Missing `OPENAI_API_KEY` safely skips Codex execution.

## Required Setup

1. Add repository secret:
   - `OPENAI_API_KEY`
2. Create issue label:
   - `agent-ready`
3. (Optional) Add PR label:
   - `codex-owned` (if you want autofix on non-`codex/*` branches)

## Easy Disable Switches

Set repository variables to `false` to disable lanes without deleting workflows:

- `VFO_AUTONOMOUS_LOOP_ENABLED` (master switch)
- `VFO_CODEX_SWEEP_ENABLED` (scheduled sweep lane)
- `VFO_CODEX_ISSUE_ENABLED` (issue label lane)
- `VFO_CODEX_AUTOFIX_ENABLED` (bounded PR autofix lane)

Unset variables default to enabled behavior.

## Operational Notes

- The issue-trigger lane assumes maintainers control label application.
- The autofix lane intentionally performs at most one attempt per failing head SHA.
- For broad or ambiguous issues, maintainers should keep `agent-ready` off until scope is narrowed.
