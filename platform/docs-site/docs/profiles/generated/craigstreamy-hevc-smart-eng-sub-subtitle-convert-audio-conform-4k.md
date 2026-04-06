# Craigstreamy HEVC Smart English Subtitle Convert Audio Conform 4K Profile

Generated from stock preset pack `craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform`.

## Dependencies

| Tool | Needed | Why |
| --- | --- | --- |
| `ffmpeg` | required | scenario execution, encode/transcode, and mux packaging |
| `ffprobe` | required | criteria probing and stream/metadata inspection |

## E2E Verification

This profile is considered e2e-verified when its mapped suites pass in CI.

| Suite | What it proves | Toolchain version report |
| --- | --- | --- |
| `tests/e2e/run_profile_actions_e2e.sh` | action-level output behavior, guardrails, and subtitle-intent pathways | `tests/e2e/.reports/latest/run_profile_actions_e2e_toolchain_versions.md` |

- Combined toolchain snapshot: [Latest E2E Toolchain Report](../../e2e-toolchain-latest.md)

## Intent

This profile converts candidates into streaming-friendly HEVC outputs while keeping the `smart_eng_sub` subtitle selection heuristic and converting selected text subtitles into delivery-friendly text form when the final container remains MP4-friendly.

## What It Optimizes For

- practical bitrate efficiency with a consistent HEVC target
- preserve AAC and Dolby-family audio streams when already acceptable
- conform DTS-family or PCM-family audio into open-source Dolby-aligned delivery codecs when needed
- subtitle policy: `smart_eng_sub` + `subtitle_convert`
- conditional container selection: stream-ready MP4 when selected subtitles are text-convertible, explicit fallback for bitmap subtitles

## Input Envelope

| Field | Value |
| --- | --- |
| Codec | `any` |
| Bit depth | `any` |
| Color space | `any` |
| Min resolution | `1920x1080` |
| Max resolution | `3840x2160` |

## Scenario Map

| Scenario | Command |
| --- | --- |
| `RES_JUST_RIGHT` | `transcode_hevc_4k_smart_eng_sub_subtitle_convert_audio_conform_profile.sh` |
| `ELSE` | `profile_guardrail_skip.sh (requires 1920x1080 to 3840x2160 input)` |

## Runtime Behavior

- Scenario `RES_JUST_RIGHT` uses action script `transcode_hevc_4k_smart_eng_sub_subtitle_convert_audio_conform_profile.sh`.
- Scenario `ELSE` uses action script `profile_guardrail_skip.sh`.

## Starting Inputs And Expected Outputs

| Aspect | What this profile expects / does |
| --- | --- |
| Starting containers | `mkv, mp4, mov, mxf (anything ffmpeg can demux)` |
| Required codec envelope | `any` / `any-bit` / `any` |
| Required resolution range | `1920x1080` to `3840x2160` |
| If criteria do not match | candidate is routed to another profile or skipped |
| If criteria match | scenario order is evaluated and first match executes |
| Output intent | conditional: stream-ready MP4 with converted text subtitles when the smart_eng_sub policy selects text subtitles and audio remains MP4-safe; fallback to MKV subtitle preservation when preserved audio safety forces MKV |

## Flow

```mermaid
flowchart LR
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;

  A[Input candidate: mkv / mp4 / mov / mxf]:::stage --> B[Probe codec bits color resolution]:::stage
  B --> C{Matches profile criteria envelope?}:::gate
  C -->|No| Z[Handled by other profile or guardrail skipped]:::skip
  C -->|Yes| D{Evaluate scenarios in order}:::gate
  D --> E[Execute subtitle-policy action]:::stage
  E --> P[Optional lane-specific pre-processing]:::stage
  P --> F{Selected subtitle is text-convertible and MP4 remains viable?}:::gate
  F -->|Yes| G[Encode HEVC + conform audio if needed + convert selected subtitle to mov_text]:::stage
  G --> H[Emit MP4 output with converted subtitle text]:::output
  F -->|No| I[Encode HEVC + conform audio if needed + preserve selected subtitle by explicit fallback]:::stage
  I --> J[Emit explicit fallback output (usually MKV preserve or fail)]:::stage
  J --> K[Emit final profile artifact]:::output
```

## What This Profile Does Not Do

- It does not normalize frame rate; source cadence/timebase is preserved by default.
- It does not invent Atmos or proprietary immersive metadata.
- It does not transcode already-acceptable AAC or Dolby-family audio just to make everything uniform.
- It does not apply a broad audio bitrate-lowering strategy yet.
- It does not semantically understand subtitle meaning; subtitle selection is metadata and flag driven.
- It does not OCR or convert bitmap subtitles to text subtitles.
- It does not generate ABR ladders (HLS/DASH); output is a single-file artifact.
- It does not certify playback on every device model; profile criteria are compatibility-oriented guardrails.
- It does not enforce PSNR/SSIM/VMAF thresholds unless quality checks are explicitly enabled and configured.
- It does not invent missing HDR/DV essence; metadata repair is heuristic and can be disabled.
- It depends on source integrity and toolchain support for DV/HDR retention; strict mode may fail instead of silently downgrading.

## High-Level Assessments

| Label | Assessment |
| --- | --- |
| Dynamic range | `HDR/DV aware` on 4K, SDR-gated on 1080p, broad intake on legacy sub-HD |
| Resolution | `4K / 1080p / legacy sub-HD` lane family |
| Audio codecs | `AAC + Dolby preserve`, `DTS/PCM conform` |
| Video codecs | `HEVC transcode target` |
| Interlacing | `legacy lane only; optional deinterlace` |
| Volume normalisation | `applied when DTS/PCM-family audio is transcoded` |
| Crop | `legacy lane auto-crop enabled` |
| Lowered video bitrate | `yes` |
| Lowered audio bitrate | `not as a general policy; only codec-target defaults for DTS/PCM conform` |
| Audio transcoded | `DTS/PCM-family only` |
| Video transcoded | `yes` |
| Audio switched | `DTS/PCM -> AAC / E-AC-3 / AC-3 when needed` |
| Subtitle retained | `smart_eng_sub + subtitle_convert` |
| Subtitle transformed | `selected text subtitles -> mov_text` |
| Container changed | `yes when subtitle or preserved-audio safety requires MKV, otherwise fragmented MP4 with faststart fallback for E-AC-3` |
| Container targets | `MKV` / `fragmented MP4` |
| Bitrate targets | `practical video efficiency; audio preserve-first` |
| Audio bitrate targets | `codec-target defaults only when DTS/PCM-family audio is conformed` |
| Overall bitrate targets | `reduce video bitrate while preserving viewing intent and sane audio delivery` |
| Error | `guardrail skip, missing toolchain, strict DV/HDR mismatch, or unknown error placeholder` |

## Source

- Preset file: `services/vfo/presets/craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform/vfo_config.preset.conf`
- Generated by: `infra/scripts/generate-profile-docs.sh`
