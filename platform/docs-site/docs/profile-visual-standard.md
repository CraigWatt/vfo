# Profile Visual Standard

Each profile info sheet should answer five things quickly:

1. Input envelope (codec, bits, color, min/max resolution)
2. Scenario map (ordered rules and commands)
3. Runtime behavior (what actually happens)
4. Output container decision path
5. Operator knobs (env vars that alter behavior)

## Standard Diagram Pattern

```mermaid
flowchart TD
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;

  A[Candidate enters profile]:::stage --> B{Matches criteria envelope?}:::gate
  B -->|No| Z[Handled by another profile or skipped]:::skip
  B -->|Yes| C{Evaluate scenarios in order}:::gate
  C --> D[Execute selected command/action]:::stage
  D --> E[Write output artifact]:::output
```

## Selected-Subtitle Variant

For `craigstreamy_hevc_selected_english_subtitle_preserve` and `craigstreamy_hevc_smart_eng_sub_audio_conform` profiles, the output container branch is explicit:

- Selected English subtitle found -> MKV output
- Selected English subtitle not found -> stream-ready MP4 output (fragmented + init/moov at start by default)

This variant is now generated automatically into each selected-subtitle profile sheet.

## Where to See It

- [Stock profile info sheets](profiles/index.md)
- `craigstreamy` active profiles:
  - [4k smart-eng-sub audio-conform sheet](profiles/generated/craigstreamy-hevc-smart-eng-sub-audio-conform-4k.md)
  - [1080p smart-eng-sub audio-conform sheet](profiles/generated/craigstreamy-hevc-smart-eng-sub-audio-conform-1080p.md)
  - [4k selected-subtitle sheet](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-4k.md)
  - [1080p selected-subtitle sheet](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md)
