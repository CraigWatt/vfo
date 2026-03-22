# Profile Action Schema Proposal

This document defines a practical path for hardware-aware profile actions while keeping backward compatibility.

## Goal

Support profile workflows such as:

- 4K HEVC mezzanine -> optimized streaming output
- 1080p mezzanine -> optimized streaming output
- 1080p H.264 device-compatibility outputs
- explicit HDR->SDR conversion actions for SDR-only targets
- 4K HEVC with Dolby Vision metadata retention and explicit profile 7 -> 8.1 conversion
- host-aware execution (Apple Silicon hardware encode when available, software fallback otherwise)

## Phase 1 (available now, no parser change)

Use existing `*_FFMPEG_COMMAND` entries to call action scripts directly.

Recommended default: reference installed command names in PATH.

Example:

```text
ROKU4K_SCENARIO="RES_TOO_HIGH"
ROKU4K_FFMPEG_COMMAND="transcode_hevc_4k_profile.sh $vfo_input $vfo_output"
```

Why this works:

- vfo already substitutes `$vfo_input` and `$vfo_output`.
- vfo already executes scenario commands via shell.
- no code change needed for initial rollout.

## Phase 2 (proposed native config extension)

Add optional keys that map scenario->action name while preserving current command mode.

Proposed keys:

```text
PROFILE_ACTION="hevc_4k_stream"
PROFILE_ACTION_COMMAND="/path/to/script.sh"
PROFILE_ACTION_DEFAULT_ENCODER="auto"   # auto|hw|cpu
```

Scenario usage:

```text
ROKU4K_SCENARIO="RES_TOO_HIGH"
ROKU4K_ACTION="hevc_4k_stream"
```

Runtime behavior:

1. if `*_ACTION` is present, resolve action command and run it
2. else use `*_FFMPEG_COMMAND` (direct command path)

Backward compatibility:

- all existing configs keep working unchanged
- mixed-mode configs are allowed during migration

## Action Script Contract

Every action script should:

1. accept `<input> <output>` args
2. return non-zero exit status on failure
3. write deterministic output path to `<output>`
4. avoid deleting input content

## Why this helps end users

- encapsulates complex codec logic in reusable action scripts
- keeps config readable and target-oriented
- allows hardware-specific tuning without profile explosion
