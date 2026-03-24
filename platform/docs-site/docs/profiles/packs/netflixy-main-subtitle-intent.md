# Netflixy Main Subtitle Intent Pack

This pack targets practical streaming efficiency while preserving viewer-intent essentials.

## Outcome Target

- reduce bitrate with Netflix-like practical intent
- preserve full audio set and director-intent "main subtitle" behavior
- emit container type by viewing-intent need: MKV when subtitle intent applies, fragmented MP4 otherwise

## Guardrails

- 1080 profile lane is SDR-gated (`bt709`) in the 1280x720..1920x1080 envelope.
- 4K profile lane accepts SDR or HDR candidates in the configured 1920x1080..3840x2160 envelope.
- Legacy sub-HD profile lane accepts broad codec/color intake in the 320x240..1279x719 envelope.
- Codec intake is broad across lanes (`any`), so HEVC, H.264 (including rare 10-bit), AV1, VP9, and legacy MPEG-2 style mezzanines can be processed.
- Guardrail misses are written as `*.guardrail_skipped.txt` markers by `profile_guardrail_skip.sh`.

## Focus

- preserve all audio streams
- preserve one "main subtitle" when it appears director-intent oriented
- when subtitle intent is present, emit MKV for robust subtitle compatibility
- when subtitle intent is absent, emit stream-ready MP4 (fragmented + init/moov-at-start by default)
- for legacy sub-HD lane: optional interlace-aware deinterlace + stable black-bar auto-crop

## Included Profiles

- [netflixy_preserve_audio_main_subtitle_intent_4k](../generated/netflixy-preserve-audio-main-subtitle-intent-4k.md)
- [netflixy_preserve_audio_main_subtitle_intent_1080p](../generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md)
- [netflixy_preserve_audio_main_subtitle_intent_legacy_subhd](../generated/netflixy-preserve-audio-main-subtitle-intent-legacy-subhd.md)

## Pack Flow

```mermaid
flowchart TD
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Candidate media]:::stage --> B{Matches 4K envelope?}:::gate
  B -->|Yes| C[4K subtitle-intent lane]:::stage
  B -->|No| D{Matches 1080p envelope?}:::gate
  D -->|Yes| E[1080p subtitle-intent lane]:::stage
  D -->|No| F{Matches legacy sub-HD envelope?}:::gate
  F -->|Yes| G[Legacy sub-HD subtitle-intent lane]:::stage
  F -->|No| X[Handled by other profile or skipped]:::stage

  C --> Cq{Main subtitle intent?}:::gate
  Cq -->|Yes| Cmkv[MKV output]:::output
  Cq -->|No| Cmp4[Fragmented MP4 output]:::output

  E --> Eq{Main subtitle intent?}:::gate
  Eq -->|Yes| Emkv[MKV output]:::output
  Eq -->|No| Emp4[Fragmented MP4 output]:::output

  G --> Gprep[Optional deinterlace + stable auto-crop]:::stage
  Gprep --> Gq{Main subtitle intent?}:::gate
  Gq -->|Yes| Gmkv[MKV output]:::output
  Gq -->|No| Gmp4[Fragmented MP4 output]:::output
```
