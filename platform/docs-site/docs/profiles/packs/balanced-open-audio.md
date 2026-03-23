# Balanced Open Audio Pack

This pack provides simple baseline lanes for broad usage before device-specific tuning.

## Outcome Target

- produce consistent 4K/1080 outputs with low setup overhead
- keep audio handling open where possible
- provide a stable baseline before moving to stricter device-target packs

## Focus

- easy 4K and 1080p starter targets
- open audio stream strategy where possible
- straightforward criteria envelopes for quick setup

## Included Profiles

- [balanced_4k_open_audio](../generated/balanced-4k-open-audio.md)
- [balanced_1080_open_audio](../generated/balanced-1080-open-audio.md)

## Pack Flow

```mermaid
flowchart TD
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Candidate media]:::stage --> B{Matches 4K balanced envelope?}:::gate
  B -->|Yes| C[balanced_4k_open_audio]:::stage
  B -->|No| D{Matches 1080p balanced envelope?}:::gate
  D -->|Yes| E[balanced_1080_open_audio]:::stage
  D -->|No| F[Handled by other profile or skipped]:::stage
  C --> G[Profile output written]:::output
  E --> H[Profile output written]:::output
```
