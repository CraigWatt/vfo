# Architecture

This repository is adopting a layered Codex workflow so that lightweight repo work defaults to smaller model settings and lower reasoning, while expensive high-reasoning turns are reserved for harder tasks.

## Layers

### Interface Layer

- `Codex` is the primary repo worker.
- `GPT-5.4 mini` or `GPT-5.4` with lower reasoning is the default lane for triage, planning, docs, file discovery, and bounded implementation work.
- `GPT-5.4 high` is the premium escalation lane for difficult, correctness-sensitive, or stalled work.

### Runtime And Model Layer

- Everyday work should prefer:
  - `GPT-5.4 mini`, or
  - `GPT-5.4` with `minimal` or `low` reasoning
- Escalation work should prefer:
  - `GPT-5.4` with `high` reasoning
- The model choice is part of task routing, not an implementation detail.

### Trigger Layer

- GitHub issues are the main durable trigger surface.
- GitHub Actions performs repository-side classification and reporting.

### Durable Memory Layer

Repository-local markdown files define the operating policy:

- `AGENTS.md`
- `objectives.md`
- `architecture.md`
- `subsystems.md`
- `testing.md`
- `escalation.md`

These files are the persistent brain for routing decisions, not disposable chat context.

## Intended Control Flow

1. A GitHub issue or maintainer request enters the system.
2. The request is classified against repository docs and likely subsystems.
3. The default route is `GPT-5.4 mini first`.
4. If the task is high risk, correctness-sensitive, or a failed/stalled lightweight first pass, the task is promoted to `GPT-5.4 high now`.
5. The chosen route and required verification bar are written back to the issue or control surface.

## Current Repository State

The repository currently implements the first routing scaffold:

- a lightweight GitHub issue-routing workflow for classification and recommendation
- repo-local markdown instruction files for durable context
- task-packet docs and issue templates that keep prompts small
- premium Codex workflows kept as explicit escalation lanes

Repository-integrated routing is intended to reduce context waste before work reaches Codex, not to create a second autonomous worker stack.
