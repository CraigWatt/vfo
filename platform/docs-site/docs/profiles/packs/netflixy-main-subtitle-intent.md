# Netflixy Main Subtitle Intent Pack

This pack targets practical streaming efficiency while preserving viewer-intent essentials.

## Outcome Target

- reduce bitrate with Netflix-like practical intent
- preserve full audio set and director-intent "main subtitle" behavior
- emit container type by viewing-intent need: MKV when subtitle intent applies, fragmented MP4 otherwise

## Focus

- preserve all audio streams
- preserve one "main subtitle" when it appears director-intent oriented
- when subtitle intent is present, emit MKV for robust subtitle compatibility
- when subtitle intent is absent, emit stream-ready MP4 (fragmented + init/moov-at-start by default)

## Included Profiles

- [netflixy_preserve_audio_main_subtitle_intent_4k](../generated/netflixy-preserve-audio-main-subtitle-intent-4k.md)
- [netflixy_preserve_audio_main_subtitle_intent_1080p](../generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md)

## Pack Flow

```mermaid
flowchart TD
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Candidate media]:::stage --> B{Matches 4K envelope?}:::gate
  B -->|Yes| C[4K subtitle-intent profile]:::stage
  B -->|No| D{Matches 1080p envelope?}:::gate
  D -->|Yes| E[1080p subtitle-intent profile]:::stage
  D -->|No| F[Handled by other profile or skipped]:::stage
  C --> G{Main subtitle intent?}:::gate
  E --> H{Main subtitle intent?}:::gate
  G -->|Yes| I[MKV output]:::output
  G -->|No| J[Fragmented MP4 output]:::output
  H -->|Yes| K[MKV output]:::output
  H -->|No| L[Fragmented MP4 output]:::output
```
