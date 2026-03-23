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

## Subtitle-Intent Variant

For `netflixy_main_subtitle_intent` profiles, the output container branch is explicit:

- Main subtitle found -> MKV output
- Main subtitle not found -> MP4 faststart output

This variant is now generated automatically into each subtitle-intent profile sheet.

## Where to See It

- [Stock profile info sheets](profiles/index.md)
- `netflixy` active profiles:
  - [4k subtitle-intent sheet](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-4k.md)
  - [1080p subtitle-intent sheet](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md)
