# Escalation

This file defines the routing policy for everyday Codex work versus premium high-reasoning Codex work.

## Default Rule

Every task should be scoped first and attempted with `GPT-5.4 mini` or lower reasoning before premium escalation, unless it is obviously high risk.

## GPT-5.4 Mini First

Use `GPT-5.4 mini` first, or `GPT-5.4` with `minimal` / `low` reasoning, for:

- issue triage
- repo navigation and file discovery
- CI log summarization
- implementation planning
- docs and markdown work
- PR descriptions and status summaries
- bounded, moderate-risk fixes where the blast radius is small

## GPT-5.4 High Now

Escalate directly to `GPT-5.4` with `high` reasoning for:

- high-risk logic in core pipeline behavior
- subtle bug fixes where preserving current behavior matters
- correctness-sensitive changes in `Profile`, `Source`, `InputHandler`, `quality`, workflow files, or action scripts
- tasks that span multiple important subsystems
- work where low-confidence mistakes would be expensive

## Escalate After One Lightweight Attempt

Do not loop endlessly on the cheap path.

Escalate to `GPT-5.4 high` after one good lightweight attempt when:

- the model reports low confidence
- the first pass fails verification
- the change reaches into more subsystems than originally expected
- the fix needs careful behavior preservation or architectural judgment
- the maintainer explicitly promotes the task
- the lightweight pass stalls, thrashes, or burns too much context without reducing uncertainty

## Anti-Pattern

The repository should not keep retrying `GPT-5.4 mini` or low reasoning just because it is cheaper. The cost rule is:

- one good lightweight attempt
- then escalate if needed

## Done Means

Routing is complete only when the output includes:

- a recommended route (`GPT-5.4 mini first` or `GPT-5.4 high now`)
- the reason for that route
- the likely subsystem or file area
- the expected verification bar
- any escalation trigger that would move the task to `GPT-5.4 high` later
