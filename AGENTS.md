# AGENTS.md

This file defines repository-specific operating rules for AI coding agents and automation.

## Purpose

Use this repository layout and workflow consistently while we transition to:

- `infra/`
- `platform/`
- `services/`

## Repository Ownership Boundaries

- `infra/`: IaC, environments, networking, CI/CD plumbing, packaging.
- `platform/`: shared runtime components and common libraries used by multiple services.
- `services/`: product/domain services, one folder per service.

Current service:

- `services/vfo/`

## Test Placement Rules

- Service tests: `services/<service-name>/test/`
- Platform component tests: `platform/<component>/test/`
- Infra validation tests: `infra/test/`
- Cross-service/system tests: `tests/e2e/`

When adding code, place or update tests in the nearest applicable location above.

## Migration Guardrails (Important)

- Treat root `Makefile` as a compatibility shim that delegates to `services/vfo/Makefile`.
- Prefer additive migration steps over disruptive rewrites.
- Do not reintroduce new runtime code under root-level legacy `src/` or `test/` paths.
- Keep packaging assets under `infra/packaging/`.

## Change Scope and Safety

- Keep changes focused to the task requested by the user.
- Avoid broad refactors unless explicitly asked.
- Do not modify release/version behavior unless required by the task.
- Preserve existing user changes in a dirty worktree; do not revert unrelated edits.

## Validation Expectations

Before handing off substantial code changes, run what is feasible locally:

1. `make all`
2. `make tests` (if dependencies are available)

If tests cannot run (for example missing system dependency), report that clearly.

## Documentation Expectations

Update docs when behavior or paths change, especially:

- `README.md`
- service-level READMEs (for example `services/vfo/README.md`)
- workflow or packaging path references

## PR Notes

When restructuring files, include a short migration summary in the PR description:

- what moved
- why it moved
- compatibility notes
- follow-up tasks (if any)
