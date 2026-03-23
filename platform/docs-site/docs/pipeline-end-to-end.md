# Pipeline End To End

The default vfo pipeline is shown below with explicit decision gates and completion paths:

```mermaid
flowchart TD
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef warn fill:#fef2f2,stroke:#dc2626,color:#7f1d1d,stroke-width:1.2px;

  A[Mezzanine input]:::stage --> B[Mezzanine-clean optional]:::stage
  B --> C[Mezzanine stage]:::stage
  C --> D{KEEP_SOURCE=true?}:::gate
  D -->|Yes| E[Source stage]:::stage
  D -->|No| F[Profiles stage]:::stage
  E --> F
  F --> G{QUALITY_CHECK_ENABLED=true?}:::gate
  G -->|Yes| H[Quality scoring stage]:::stage
  G -->|No| I[Run complete]:::output
  H --> I

  C -. stage failure .-> X[Run aborted with error]:::warn
  E -. stage failure .-> X
  F -. stage failure .-> X
  H -. strict gate failure .-> X
```

## Visual Levels

For alternative views optimized for different audiences, see [Flow Levels](flow-levels.md):

- executive view
- operator (CLI) view
- engine/status-key view

## What Content Can Be Accommodated?

vfo is criteria-driven, so "accommodated" means "matches at least one configured profile criteria envelope and scenario."

For stock presets, this includes practical lanes for:

- 4K HEVC target outputs
- 1080p compatibility outputs
- subtitle-intent profile outputs (MKV when main subtitle should be preserved)
- device-oriented profiles (Roku/Fire TV/Chromecast/Apple TV families)

Use the [Capability Matrix](profile-capability-matrix.md) for exact codec/bit-depth/color/resolution envelopes.
