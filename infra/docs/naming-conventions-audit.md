# Naming Conventions Audit

Audit date: 2026-03-22

## Objective

Assess current naming conventions across files and code, identify inconsistencies, and define practical migration guidance aligned to end-user experience.

## Method

- reviewed tracked repository paths via `git ls-files`
- sampled all source modules under `services/vfo/src/`
- reviewed user-facing CLI/help/config/docs naming
- compared existing conventions to desired UX terminology (`mezzanine -> source -> profile`)

## Snapshot

- tracked files: 128
- tracked directories (non-root): 47
- directories with uppercase path segments: 20
- files with uppercase names: 17 (all expected top-level/readme/build exceptions)
- dominant C function prefix families:
  - `utils_`, `ih_`, `a_`, `s_`, `o_`, `con_`, `ca_`, `cf_`, `cs_`, `uw_`

## Findings

### 1) Policy filename ambiguity (resolved)

- The repo now uses `AGENT.md` as the single canonical policy file.
- This removes ambiguity and avoids dual-file drift.

### 2) Legacy/internal naming leaks into UX

User-facing intent is moving to:

- mezzanine
- source
- profile

But code/config/commands still heavily use:

- `original`
- `alias`
- `all_aliases`
- `do_it_all`

This is expected for compatibility, but the naming strategy needs explicit rules so we avoid introducing more legacy-only names.

### 3) Source tree path style inconsistency

Under `services/vfo/src/` and `services/vfo/test/`, legacy CamelCase directory names remain:

- `Alias`, `Config`, `InputHandler`, `Source_AS`, etc.
- `AliasTests`, `ConfigTests`, etc.

Newer repository layout (`infra/`, `platform/`, `services/`) is already lowercase and cleaner.

### 4) Mixed command naming patterns

Current commands combine styles:

- plain words: `run`, `doctor`, `wizard`, `show`
- snake_case: `all_aliases`, `do_it_all`
- stage words tied to legacy internals: `original`, `source`

This is workable but should be governed by an explicit canonical/legacy mapping.

Note: stale help placeholders (`joker`, `custom_alias`) were removed during this audit pass to align help text with actual supported command patterns.

### 5) Config naming is internally consistent but legacy-first

Config keys are consistently `UPPER_SNAKE_CASE`, which is good.
However key families are still centered around legacy terms (`ORIGINAL_*`, `ALIAS=*`), while docs increasingly describe canonical terms.

## Recommendations

### Immediate (documented now)

1. Establish canonical policy in `AGENT.md` (done).
2. Keep only `AGENT.md` to avoid policy duplication (done).
3. Require canonical user-facing terminology in new docs/help:
   - `mezzanine`, `source`, `profile`
4. Keep legacy terms as compatibility surface until formal deprecation.

### Near-term (non-breaking improvements)

1. Add canonical command aliases (for example `all-profiles`, `run-all`) while retaining legacy commands.
2. In CLI help, show canonical name first and legacy name second.
3. Add canonical config key aliases (`MEZZANINE_*`, `PROFILE_*`) with fallback to legacy keys.

### Later (breaking or semi-breaking path cleanup)

1. Rename legacy CamelCase source/test directories to lowercase style.
2. Introduce compatibility includes or staged migration for include paths.
3. Execute this only with explicit user approval due to churn risk.

## Naming Direction (Final)

- Paths: lowercase-kebab for docs/workflows directories, snake_case for C files and shell scripts.
- C symbols: snake_case with module prefix.
- Config keys: `UPPER_SNAKE_CASE`.
- User-facing terms: mezzanine/source/profile (legacy terms explicitly marked).

## Risk Notes

- Large-scale symbol/path renames are high-churn and can destabilize active work.
- A staged compatibility-first migration is the safest approach for this codebase.
