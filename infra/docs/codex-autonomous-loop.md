# Codex Autonomous Repo Loop (v1)

This repository ships a small, bounded premium automation loop powered by the official Codex GitHub Action.

Codex is not the default first pass. The intended order is:

1. issue routing and scoping
2. Qwen Cloud first when the task is bounded
3. Codex only when maintainers intentionally escalate

## Goals Covered

- premium proactive sweep when explicitly enabled
- premium issue trigger via `agent-ready`
- bounded premium CI autofix for Codex-owned PRs
- explicit guardrails and easy-disable controls

## Related Routing Workflow

Initial issue classification now lives in:

- `.github/workflows/ci-issue-routing.yml`

That workflow reads the repo policy files and comments with a `Qwen Cloud first` or `Codex now` recommendation before maintainers promote work into these premium lanes.

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
- Premium lanes stay off unless repository variables explicitly enable them.

## Required Setup

1. Add repository secret:
   - `OPENAI_API_KEY`
2. Create issue label:
   - `agent-ready`
3. (Optional) Add PR label:
   - `codex-owned` (if you want autofix on non-`codex/*` branches)
4. Explicitly enable premium lanes with repository variables:
   - `VFO_AUTONOMOUS_LOOP_ENABLED=true`
   - `VFO_CODEX_SWEEP_ENABLED=true`
   - `VFO_CODEX_ISSUE_ENABLED=true`
   - `VFO_CODEX_AUTOFIX_ENABLED=true`

## Enable Switches

Set repository variables to `true` to enable lanes. Leaving them unset keeps premium Codex automation off by default.

- `VFO_AUTONOMOUS_LOOP_ENABLED` (master switch)
- `VFO_CODEX_SWEEP_ENABLED` (scheduled sweep lane)
- `VFO_CODEX_ISSUE_ENABLED` (issue label lane)
- `VFO_CODEX_AUTOFIX_ENABLED` (bounded PR autofix lane)

## Operational Notes

- The issue-trigger lane assumes maintainers control label application.
- The autofix lane intentionally performs at most one attempt per failing head SHA.
- For broad or ambiguous issues, maintainers should keep `agent-ready` off until scope is narrowed.
