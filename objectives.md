# Objectives

This repository is building a repo-aware Codex workflow that defaults to lighter model and reasoning settings for everyday work, then escalates only when the task genuinely needs it.

## Primary Goals

- Reduce premium Codex token consumption without losing strong coding assistance when correctness matters.
- Keep GitHub issues as the durable trigger surface for triage, planning, and implementation routing.
- Use markdown files in the repository as durable memory instead of relying on one long chat thread.
- Keep escalation explicit so maintainers can see why a task moved from `GPT-5.4 mini` or lower reasoning to `GPT-5.4 high`.

## Non-Goals

- Do not make `GPT-5.4 high` the default first pass for every issue.
- Do not let lightweight passes retry forever just because they are cheaper.
- Do not bypass repository tests, workflow guardrails, or human review.
- Do not introduce automation that mutates release or deployment lanes without separate approval.

## Definition Of Done

A task is considered routed correctly when all of the following are true:

- the issue or request is scoped to a subsystem
- the routing decision is recorded with a short reason
- the first-pass model choice is clear (`GPT-5.4 mini first` or `GPT-5.4 high now`)
- required verification expectations are listed before implementation starts
- escalation, when needed, is explicit and justified

## Current Implementation Intent

The first implementation pass in this repository should provide:

- repo-local memory files for objectives, architecture, subsystems, testing, and escalation policy
- a GitHub issue-routing workflow that reads those files and comments with a recommended route
- task-packet docs and issue templates that keep Codex requests small and explicit
- premium `GPT-5.4 high` lanes that are opt-in and treated as escalation paths rather than default automation
