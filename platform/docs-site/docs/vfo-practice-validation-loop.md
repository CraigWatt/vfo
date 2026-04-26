# VFO Practice Validation Loop

The VFO practice validation loop is a bounded way to test real mezzanine assets without turning the repository into an unreviewed auto-fix machine.

It uses `/Volumes/Mitchum/vfo_practice` as the only user-managed media staging area and reports only technical identifiers and failures. It must not summarize media content.

## What The Loop Does

One cycle runs:

1. `vfo doctor`
2. `vfo status`
3. a configurable profile pipeline command, defaulting to `vfo run`

It captures:

- shell and script errors
- `ffmpeg`, `mkvtoolnix`, and `dovi_tool` failure markers
- missing files or bad paths
- obvious regression markers in VFO output
- selected weak signals from the relevant VFO code paths

The selected or newest asset is recorded as reproduction context. VFO still processes whatever candidates are visible to the active config, so isolate the practice bed when you want a single-asset run.

Reports are written under:

```text
tests/e2e/.reports/vfo-practice/latest
```

## Run Once

From the repository root:

```bash
tests/e2e/run_vfo_practice_validation_loop.sh --once
```

Defaults:

- practice root: `/Volumes/Mitchum/vfo_practice`
- VFO binary: `vfo` from `PATH`
- profile pack context: `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`
- profile command: `vfo run`
- smoke trim: `SOURCE_TEST_ACTIVE=true`
- smoke duration: `SOURCE_TEST_TRIM_DURATION=00:02:00`

## Run The Interactive Profile Path

Use this when you specifically want to exercise `vfo profiles`:

```bash
VFO_PRACTICE_PROFILE_COMMAND="yes y | vfo profiles" \
tests/e2e/run_vfo_practice_validation_loop.sh --once
```

To force a specific installed or local binary:

```bash
VFO_PRACTICE_VFO_BIN="/usr/local/bin/vfo" \
tests/e2e/run_vfo_practice_validation_loop.sh --once
```

## Watch For New Practice Media

The watch mode polls the newest media fingerprint under `mezzanine`:

```bash
VFO_PRACTICE_POLL_SECONDS=21600 \
tests/e2e/run_vfo_practice_validation_loop.sh --watch
```

This is suitable for a local terminal, `launchd`, or another human-controlled scheduler on the machine that can access `/Volumes/Mitchum/vfo_practice`.

## Create Deduped GitHub Issues

Issue creation is opt-in:

```bash
VFO_PRACTICE_CREATE_ISSUES=1 \
tests/e2e/run_vfo_practice_validation_loop.sh --once
```

Issue intake uses the `codex-candidate` label.

The runner deduplicates by a normalized technical signature, not by asset. It includes only:

- asset path
- pack/profile context
- command run
- failing stage
- exact error text
- minimal technical stream metadata needed to reproduce

It intentionally does not include title tags, descriptive summaries, screenshots, or media content notes.

## Codex Handoff

Use two labels:

- `codex-candidate`: validation found a scoped technical candidate
- `agent-ready`: one issue is approved for Codex to attempt a branch and PR

Before adding `agent-ready`, check that there is no active Codex PR:

```bash
gh pr list --state open --json number,headRefName,title \
  --jq '.[] | select(.headRefName | startswith("codex/"))'
```

Then promote exactly one issue:

```bash
gh issue edit ISSUE_NUMBER --add-label agent-ready
```

The existing Codex issue workflow will create an isolated `codex/` branch, run the scoped fix, verify with tests, and open a PR only if verification passes.

## Safety Rules

- Never push directly to `main`.
- Keep at most one active Codex PR per repo.
- Keep fixes small and reversible.
- Do not change infra or secrets without human approval.
- Use `/Volumes/Mitchum/vfo_practice` as the only media staging area.
- Report technical identifiers and failures only, never media content.
