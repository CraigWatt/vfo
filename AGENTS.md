# AGENTS.md

Compatibility alias for tools that expect `AGENTS.md`.

Canonical policy has moved to:

- `AGENT.md`

If any statement here and `AGENT.md` diverge, treat `AGENT.md` as authoritative.

## Repository Boundaries (Summary)

- `infra/`: IaC, environments, networking, CI/CD, packaging
- `platform/`: shared runtime components and common libraries
- `services/`: product/domain services (current service: `services/vfo/`)

## Test Placement (Summary)

- service tests: `services/<service>/test/`
- platform tests: `platform/<component>/test/`
- infra tests: `infra/test/`
- cross-service e2e: `tests/e2e/`

## Naming Policy

All naming standards and migration rules are defined in `AGENT.md`, including:

- file and directory naming
- C symbol naming
- CLI/config naming and legacy compatibility
- deprecation and migration guardrails
