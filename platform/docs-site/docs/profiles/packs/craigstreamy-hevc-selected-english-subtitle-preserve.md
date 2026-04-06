# Craigstreamy HEVC Selected English Subtitle Preserve Pack

This pack targets practical streaming efficiency while preserving the `smart_eng_sub + preserve` subtitle policy.

## Intent

This pack standardizes mixed-library inputs into streaming-friendly HEVC outputs while preserving the `smart_eng_sub + preserve` subtitle policy.

- preserve full audio when possible
- preserve the `smart_eng_sub` subtitle when detected
- use MKV when subtitle intent applies, otherwise emit stream-ready fragmented MP4

## Subtitle Policy

Canonical subtitle policy for this pack:

- selection scope: `smart_eng_sub`
- handling mode: `preserve`

This pack keeps its existing legacy-facing profile ids, but its subtitle behavior should now be understood through the shared [Subtitle Policy](../../subtitle-policy-taxonomy.md) taxonomy.

Compatibility note:

- the pack name is now canonical `craigstreamy`
- the generated profile ids inside this pack still use the legacy `netflixy_preserve_audio_main_subtitle_intent_*` names
- that legacy profile-id surface remains intentional for compatibility

## What It Optimizes For

- practical HEVC bitrate reduction posture
- consistent HEVC delivery across 4K, 1080p SDR, and legacy sub-HD lanes
- audio preservation first, without forcing broad audio transcoding
- subtitle policy: `smart_eng_sub` + `preserve`
- subtitle-policy-sensitive container selection
- optional legacy remediation (deinterlace + stable black-bar crop) in the sub-HD lane

## Guardrails

- 1080 profile lane is SDR-gated (`bt709`) in the 1280x720..1920x1080 envelope.
- 4K profile lane accepts SDR or HDR candidates in the configured 1920x1080..3840x2160 envelope.
- Legacy sub-HD profile lane accepts broad codec/color intake in the 320x240..1279x719 envelope.
- Codec intake is broad across lanes (`any`), so HEVC, H.264 (including rare 10-bit), AV1, VP9, and legacy MPEG-2 style mezzanines can be processed.
- Guardrail misses are written as `*.guardrail_skipped.txt` markers by `profile_guardrail_skip.sh`.

## Focus

- preserve all audio streams
- preserve the `smart_eng_sub` subtitle when it appears intent-oriented
- when subtitle intent is present, emit MKV for robust subtitle compatibility
- when subtitle intent is absent, emit stream-ready MP4 (fragmented + init/moov-at-start by default)
- for legacy sub-HD lane: optional interlace-aware deinterlace + stable black-bar auto-crop

## High-Level Assessments

These are the labels used to summarize what the pack takes in and what it tends to emit:

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

## Included Profiles

- [netflixy_preserve_audio_main_subtitle_intent_4k](../generated/netflixy-preserve-audio-main-subtitle-intent-4k.md)
- [netflixy_preserve_audio_main_subtitle_intent_1080p](../generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md)
- [netflixy_preserve_audio_main_subtitle_intent_legacy_subhd](../generated/netflixy-preserve-audio-main-subtitle-intent-legacy-subhd.md)

## Pack Flow

```mermaid
flowchart LR
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;

  subgraph Intake["Lane Classification"]
    A[Candidate media]:::stage --> B{Profile envelope match}:::gate
    B -->|4K 1920x1080..3840x2160| C[4K lane]:::stage
    B -->|1080 SDR bt709 1280x720..1920x1080| D[1080p lane]:::stage
    B -->|Legacy sub-HD 320x240..1279x719| E[Legacy lane]:::stage
    B -->|No match| X[Routed elsewhere or guardrail-skipped]:::skip
  end

  subgraph Processing["Per-Lane Processing"]
    C --> C0[Run subtitle-intent action]:::stage
    D --> D0[Run subtitle-intent action]:::stage
    E --> E0[Optional deinterlace and stable auto-crop]:::stage
    E0 --> E1[Run subtitle-intent action]:::stage
  end

  C0 --> Cq{Selected English subtitle intent detected?}:::gate
  D0 --> Dq{Selected English subtitle intent detected?}:::gate
  E1 --> Eq{Selected English subtitle intent detected?}:::gate

  Cq -->|Yes| Cmkv[Emit MKV preserving subtitle intent]:::output
  Cq -->|No| Cmp4[Emit fragmented MP4 stream-ready]:::output

  Dq -->|Yes| Dmkv[Emit MKV preserving subtitle intent]:::output
  Dq -->|No| Dmp4[Emit fragmented MP4 stream-ready]:::output

  Eq -->|Yes| Emkv[Emit MKV preserving subtitle intent]:::output
  Eq -->|No| Emp4[Emit fragmented MP4 stream-ready]:::output
```

## What This Pack Does Not Do

- It does not normalize frame rate; source cadence/timebase is preserved by default.
- It does not transcode audio for target-device compatibility by default.
- It does not guarantee every mezzanine audio codec can be muxed into every output container.
- It does not semantically understand subtitle meaning; subtitle selection uses metadata/flag heuristics.
- It does not OCR or convert bitmap subtitles to text subtitles.
- It does not generate ABR ladders (HLS/DASH); outputs are single-file delivery artifacts.
- It does not certify playback on every device model; profiles are compatibility-oriented guardrails.
- It does not enforce PSNR/SSIM/VMAF thresholds unless quality checks are explicitly enabled and configured.
- It does not invent missing HDR/DV essence; metadata repair is heuristic and can be disabled.
- It depends on source integrity and toolchain support for DV/HDR retention; strict mode may fail instead of silently downgrading.
