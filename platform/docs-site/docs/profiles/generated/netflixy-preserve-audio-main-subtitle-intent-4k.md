# netflixy_preserve_audio_main_subtitle_intent_4k

Generated from stock preset pack `craigstreamy-hevc-selected-english-subtitle-preserve`.

## Dependencies

| Tool | Needed | Why |
| --- | --- | --- |
| `ffmpeg` | required | scenario execution, encode/transcode, and mux packaging |
| `ffprobe` | required | criteria probing and stream/metadata inspection |
| `mkvmerge` | conditional | used by at least one action path in this profile family (MKV/DV helper path) |
| `mkvextract` | conditional | optional DV extraction helper path for MKV inputs |
| `dovi_tool` | conditional | required for Dolby Vision retention and profile 7 to 8.1 conversion paths |

## E2E Verification

This profile is considered e2e-verified when its mapped suites pass in CI.

| Suite | What it proves | Toolchain version report |
| --- | --- | --- |
| `tests/e2e/run_profile_actions_e2e.sh` | action-level output behavior, guardrails, and subtitle-intent pathways | `tests/e2e/.reports/latest/run_profile_actions_e2e_toolchain_versions.md` |
| `tests/e2e/run_dv_metadata_optional_e2e.sh` | optional DV metadata retention and profile 7 to 8.1 checks | `tests/e2e/.reports/latest/run_dv_metadata_optional_e2e_toolchain_versions.md` |

- Combined toolchain snapshot: [Latest E2E Toolchain Report](../../e2e-toolchain-latest.md)

## Intent

This profile converts candidates into streaming-friendly HEVC outputs while preserving the `smart_eng_sub + preserve` subtitle policy where feasible.

## What It Optimizes For

- practical bitrate efficiency with a consistent HEVC target
- preserve all audio streams by default when packaging permits
- subtitle policy: `smart_eng_sub` + `preserve`
- conditional container selection: MKV when the `smart_eng_sub + preserve` policy selects a subtitle, fragmented MP4 otherwise

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
| `RES_JUST_RIGHT` | `transcode_hevc_4k_main_subtitle_preserve_profile.sh $vfo_input $vfo_output` |
| `ELSE` | `profile_guardrail_skip.sh $vfo_input $vfo_output netflixy_4k_guardrail_mismatch_requires_1920x1080_to_3840x2160_input` |

## Runtime Behavior

- Scenario `RES_JUST_RIGHT` uses action script `transcode_hevc_4k_main_subtitle_preserve_profile.sh`.
- Scenario `ELSE` uses action script `profile_guardrail_skip.sh`.

Action summary from `transcode_hevc_4k_main_subtitle_preserve_profile.sh`:

- Always preserves audio streams with stream copy.
- Selects one "main subtitle" when it appears director-intent oriented:
-   priority: forced english -> forced untagged/unknown -> optional default english.
-   non-english forced tracks are intentionally skipped.
- Preserves dynamic-range signaling for HDR/DV workflows by default:
-   applies metadata-repair defaults when source tags are incomplete.
- If source signals Dolby Vision side data, attempts DV RPU retention/injection.
- If source is DV profile 7.x, attempts profile 8.1 conversion semantics before injection.
- If a main subtitle is selected, output container is MKV for reliable subtitle preservation.
- If no main subtitle is selected, output container is stream-ready MP4:
-   fragmented MP4 with init/moov at the start.

Operator knobs from `transcode_hevc_4k_main_subtitle_preserve_profile.sh`:

- `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1   # include default english subtitle when no forced track exists`
- `VFO_ENCODER_MODE=auto|hw|cpu`
- `VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart`
- `default: fmp4_faststart`
- `VFO_DYNAMIC_METADATA_REPAIR=1|0`
- `default: 1`
- `VFO_DYNAMIC_RANGE_STRICT=1|0`
- `default: 1`
- `VFO_DYNAMIC_RANGE_REPORT=1|0`
- `default: 1`
- `VFO_DV_REQUIRE_DOVI=1|0`
- `default: 1`
- `VFO_DV_CONVERT_P7_TO_81=1|0`
- `default: 1`
- `VFO_DV_P7_TO_81_MODE=2|5`
- `default: 2`
- `VFO_DV_REQUIRE_P7_TO_81=1|0`
- `default: 1`
- `VFO_DV_P7_EXTRACT_MODE=auto|mkvextract|ffmpeg`
- `default: auto`

## Starting Inputs And Expected Outputs

| Aspect | What this profile expects / does |
| --- | --- |
| Starting containers | `mkv, mp4, mov, mxf (anything ffmpeg can demux)` |
| Required codec envelope | `any` / `any-bit` / `any` |
| Required resolution range | `1920x1080` to `3840x2160` |
| If criteria do not match | candidate is routed to another profile or skipped |
| If criteria match | scenario order is evaluated and first match executes |
| Output intent | conditional: MKV when the smart_eng_sub + preserve policy selects a subtitle, otherwise stream-ready MP4 (fragmented + init/moov at start by default) |

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
  D --> E[Execute subtitle-intent action]:::stage
  E --> P[Optional lane-specific pre-processing]:::stage
  P --> F{smart_eng_sub subtitle selected?}:::gate
  F -->|Yes| G[Encode HEVC + preserve audio + preserve smart_eng_sub subtitle]:::stage
  G --> H[Emit MKV output]:::output
  F -->|No| I[Encode HEVC + preserve audio]:::stage
  I --> J[Finalize fragmented MP4 + init/moov at start]:::stage
  J --> K[Emit MP4 output]:::output
```

## What This Profile Does Not Do

- It does not normalize frame rate; source cadence/timebase is preserved by default.
- It does not transcode audio for target-device compatibility by default.
- It does not guarantee every input audio codec is valid for every selected output container.
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
| Audio codecs | `preserved by default` |
| Video codecs | `HEVC transcode target` |
| Interlacing | `legacy lane only; optional deinterlace` |
| Volume normalisation | `not applied by default` |
| Crop | `legacy lane auto-crop enabled` |
| Lowered video bitrate | `yes` |
| Lowered audio bitrate | `no by default` |
| Audio transcoded | `no by default` |
| Video transcoded | `yes` |
| Audio switched | `no; stream copy preferred` |
| Subtitle retained | `smart_eng_sub + preserve` |
| Subtitle transformed | `no; preserve mode only` |
| Container changed | `yes when the smart_eng_sub + preserve policy requires MKV, otherwise fragmented MP4` |
| Container targets | `MKV` / `fragmented MP4` |
| Bitrate targets | `practical efficiency over source bit-for-bit preservation` |
| Audio bitrate targets | `copy/preserve unless a future audio profile says otherwise` |
| Overall bitrate targets | `reduce video bitrate while maintaining viewing intent` |
| Error | `guardrail skip, missing toolchain, strict DV/HDR mismatch, or unknown error placeholder` |

## Source

- Preset file: `services/vfo/presets/craigstreamy-hevc-selected-english-subtitle-preserve/vfo_config.preset.conf`
- Generated by: `infra/scripts/generate-profile-docs.sh`
