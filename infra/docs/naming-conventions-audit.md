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

- The repo now uses `AGENTS.md` as the single canonical policy file.
- `AGENT.md` is retained only as a compatibility shim that points to `AGENTS.md`.
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

Under `services/vfo/src/` and `services/vfo/test/`, CamelCase directory names remain:

- `Profile`, `Config`, `InputHandler`, `Source_AS`, etc.
- `ProfileTests`, `ConfigTests`, etc.

This is functionally fine, but still differs from the lowercase style used in newer top-level repository layout (`infra/`, `platform/`, `services/`).

Current legacy inventory:

- `services/vfo/src/Config/`
- `services/vfo/src/InputHandler/`
- `services/vfo/src/Mezzanine/`
- `services/vfo/src/Mezzanine_Clean/`
- `services/vfo/src/Profile/`
- `services/vfo/src/Source/`
- `services/vfo/src/Source_AS/`
- `services/vfo/src/Utils/`
- `services/vfo/test/ConfigTests/`
- `services/vfo/test/InputHandlerTests/`
- `services/vfo/test/MezzanineTests/`
- `services/vfo/test/ProfileTests/`
- `services/vfo/test/SourceTests/`
- `services/vfo/test/UtilsTests/`

These are the only known CamelCase directory families that remain intentionally tracked.

### 4) Mixed command naming patterns

Current commands combine styles:

- plain words: `run`, `doctor`, `wizard`, `show`
- snake_case: `all_aliases`, `do_it_all`
- stage words tied to legacy internals: `original`, `source`

This is workable but should be governed by an explicit canonical/legacy mapping.

Note: stale help placeholders (`joker`, `custom_alias`) were removed during this audit pass to align help text with actual supported command patterns.

Current compatibility terminology hotspots:

- user-facing legacy stage words still present in compatibility paths: `original`, `alias`
- internal compatibility command families: `all_aliases`, `do_it_all`
- legacy file/path names that should not be copied into new code: the CamelCase directory families listed above

### 5) Config naming is internally consistent but legacy-first

Config keys are consistently `UPPER_SNAKE_CASE`, which is good.
However key families are still centered around legacy terms (`ORIGINAL_*`, `ALIAS=*`), while docs increasingly describe canonical terms.

### 6) Terminology is already cleaner at the docs boundary than in the C internals

The repo's user-facing docs and policy files mostly use canonical language now:

- `AGENTS.md` is the canonical policy file.
- README and docs pages center `mezzanine`, `source`, and `profile`.
- the docs site and task packet guidance use the modern lane vocabulary.

The internal C tree still carries a very large compatibility surface:

- `original` and `alias` remain embedded in function names, structs, comments, and config parsing paths.
- `all_aliases` and `do_it_all` still exist as internal command families and parser branches.
- the legacy names are mostly internal compatibility debt rather than active docs leakage, but they are widespread enough that automated edits should treat them carefully.

Practical takeaway:

- new docs should stay canonical
- new code should avoid adding more legacy names
- broad renames should remain staged, because the compatibility surface is still large

### 7) CLI help already speaks the canonical language

The current help/usage strings are already aligned with the modern vocabulary:

- `mezzanine-clean`
- `mezzanine`
- `source`
- `profiles`
- `status`
- `status-json`

The parser still accepts legacy aliases for compatibility, but those names are no longer presented in the help surface. That means the visible UX is cleaner than the internal detection logic.

Compatibility-only parser aliases currently retained:

- `original` for `mezzanine`
- `all_aliases` for `profiles`
- `do_it_all` as an internal legacy command family
- `mezzanine_clean` as the underscore variant of `mezzanine-clean`
- `status_json` as the underscore variant of `status-json`

## Recommendations

### Immediate (documented now)

1. Establish canonical policy in `AGENTS.md` (done).
2. Keep compatibility shim `AGENT.md` pointing to `AGENTS.md` (done).
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
