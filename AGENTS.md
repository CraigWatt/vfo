# AGENTS.md

Canonical repository conventions for human contributors and coding agents.

`AGENTS.md` is the single source of truth for repository naming and terminology policy.

## Scope

This policy defines naming conventions for:

- repository paths and filenames
- C source modules and symbols
- CLI and config naming (user-facing and internal)
- docs, workflows, scripts, and tests

## Naming Conventions

### 1) Repository Paths and Filenames

- New directories under `infra/`, `platform/`, `services/`, and `tests/` use `lowercase-kebab-case`.
- New C source/header filenames use `lower_snake_case` (for example `profile_router.c`, `profile_router.h`).
- New shell scripts use `lower_snake_case.sh` (verb-first where possible).
- New docs use `lowercase-kebab-case.md`.
- Allowed uppercase filename exceptions:
  - `README.md`
  - `LICENSE`
  - `RELEASE.txt`
  - `AGENTS.md`
  - `AGENT.md` (compatibility shim only; points to `AGENTS.md`)
  - `Makefile`

### 2) C Modules and Symbols

- Functions use `lower_snake_case`.
- File-local helpers should be `static` and retain module prefix.
- Public function names should be prefixed by module, then action:
  - good: `ih_doctor_check_path`
  - avoid for new code: unprefixed generic names
- `typedef` aliases for structs should use `_t` suffix.
- Macros and compile-time constants use `UPPER_SNAKE_CASE`.
- For new modules, prefer readable prefixes over cryptic abbreviations.
  - example: `profile_` is preferred to introducing a new short ambiguous prefix.

### 3) CLI and UX Terminology

Canonical user-facing pipeline terms are:

- `mezzanine`
- `source`
- `profile`

Deprecated vocabulary may still exist in internal compatibility paths.

Rules:

- New user-facing help/docs must prefer canonical terms.
- Do not surface deprecated vocabulary in help output, status output, docs, or examples.
- Do not introduce additional new user-facing terms that conflict with this vocabulary.

### 4) CLI Command Naming

- New commands should be canonical and human-readable.
- Preferred pattern for new commands: `kebab-case`.
- Backward-compat command shims may exist, but canonical commands are the only forms shown in help/docs/examples.

### 5) Config Key Naming

- Config keys remain `UPPER_SNAKE_CASE`.
- Existing keys are stable and must remain supported unless a formal deprecation is approved.
- For new keys, prefer canonical terminology in names:
  - prefer `MEZZANINE_*`
  - prefer `PROFILE=*`
- Profile-specific keys follow:
  - `<PROFILE_NAME>_<SETTING>`
  - where `<PROFILE_NAME>` is uppercase-snake derived from a lowercase-snake profile id.

### 6) Workflow, Script, and Test Naming

- Workflow filenames should clearly indicate lane intent:
  - `ci-*.yml`
  - `release-*.yml`
  - `e2e-*.yml`
- E2E runner scripts:
  - `run_*_e2e.sh` for execution flows
  - `validate_*` for assertions/check-only scripts

### 7) Observability Key Naming

- Status component keys should use dot-scoped `lower_snake_case` segments.
  - good: `dependency.ffmpeg`
  - good: `stage.mezzanine`
  - good: `profile.netflixy_open_audio_1080p.scenarios`
- Keep top-level scopes stable so CI/test automation can safely parse:
  - `engine.*`
  - `config.*`
  - `dependency.*`
  - `storage.*`
  - `profiles.*`
  - `profile.*`
  - `stage.*`

### 8) Docs Navigation and Information Architecture

- Any new user-facing docs page must be added to MkDocs navigation in `mkdocs.yml` within the same PR.
- New generated profile pages under `platform/docs-site/docs/profiles/generated/` must be linked from the relevant profile pack section in nav.
- Do not merge docs additions that are only reachable by direct URL.

## Compatibility and Deprecation Rules

- Never break existing CLI/config names in one step.
- If introducing a canonical replacement, keep compatibility shims available for at least two minor releases.
- Every naming deprecation must include:
  - release-note entry in `RELEASE.txt`
  - help text annotation
  - docs update in root `README.md` and service docs

## Legacy Exceptions (Known and Allowed)

These exist today and are allowed until explicitly migrated:

- `services/vfo/src/Profile/`
- `services/vfo/src/Config/`
- `services/vfo/src/InputHandler/`
- `services/vfo/src/Mezzanine/`
- `services/vfo/src/Source/`
- `services/vfo/src/Source_AS/`
- `services/vfo/src/Utils/`
- `services/vfo/test/ProfileTests/`
- `services/vfo/test/ConfigTests/`
- `services/vfo/test/InputHandlerTests/`
- `services/vfo/test/MezzanineTests/`
- `services/vfo/test/SourceTests/`
- `services/vfo/test/UtilsTests/`
- existing workflow filenames `on-*.yml` under `.github/workflows/`

No additional CamelCase directories should be introduced in these areas.

## PR Checklist for Naming

Before merge, verify:

1. new paths/files follow this naming policy
2. new user-facing text uses canonical terms (`mezzanine`, `source`, `profile`)
3. deprecated terminology is not surfaced in user-facing text
4. docs and help text remain consistent with implemented command/config names
5. every new docs page is reachable from `mkdocs.yml` navigation
