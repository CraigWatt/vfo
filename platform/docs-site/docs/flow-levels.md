# Flow Levels

These diagrams intentionally show the same pipeline at three levels, so each audience can zoom in without losing context.

## Level 1: Executive View

```mermaid
flowchart LR
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Mezzanine Ingest]:::stage --> B[Mezzanine Clean optional]:::stage
  B --> C{KEEP_SOURCE?}:::gate
  C -->|true| D[Source Normalize]:::stage
  C -->|false| E[Profile Outputs]:::output
  D --> E
  E --> F{QUALITY_CHECK_ENABLED?}:::gate
  F -->|true| G[PSNR / SSIM / optional VMAF]:::stage
  F -->|false| H[Complete]:::output
  G --> H
```

## Level 2: Operator View (CLI + Stages)

```mermaid
flowchart TD
  classDef cmd fill:#f5f3ff,stroke:#7c3aed,color:#4c1d95,stroke-width:1.2px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  C0[`vfo wizard`]:::cmd --> C1[`vfo doctor` + `vfo status`]:::cmd
  C1 --> C2[`vfo run`]:::cmd

  C2 --> S1[Stage: mezzanine-clean]:::stage
  S1 --> S2[Stage: mezzanine]:::stage
  S2 --> G1{KEEP_SOURCE=true?}:::gate
  G1 -->|Yes| S3[Stage: source]:::stage
  G1 -->|No| S4[Stage: profiles]:::stage
  S3 --> S4
  S4 --> G2{QUALITY_CHECK_ENABLED=true?}:::gate
  G2 -->|Yes| S5[Stage: quality scoring]:::stage
  G2 -->|No| O1[Profile outputs ready]:::output
  S5 --> O1
```

## Level 3: Engine View (Status Keys)

```mermaid
flowchart TD
  classDef status fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef warn fill:#fef2f2,stroke:#dc2626,color:#7f1d1d,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  E0[engine.snapshot]:::status --> C0[config.directory]:::status
  C0 --> D0[dependency.ffmpeg / ffprobe / mkvmerge / dovi_tool]:::status
  D0 --> S0[storage.mezzanine[n] / storage.source[n]]:::status
  S0 --> P0[profiles.detected + profile.*.scenarios]:::status
  P0 --> G0{stage.mezzanine ready?}:::gate
  G0 -->|No| X0[stage.execute blocked]:::warn
  G0 -->|Yes| M0[stage.mezzanine]:::status
  M0 --> G1{stage.source enabled?}:::gate
  G1 -->|Yes| SRC[stage.source]:::status
  G1 -->|No| PRF[stage.profiles]:::status
  SRC --> PRF
  PRF --> G2{stage.quality enabled?}:::gate
  G2 -->|Yes| Q0[stage.quality]:::status
  G2 -->|No| DONE[stage.execute complete]:::output
  Q0 --> DONE
```

## Reading Tip

- Use **Level 1** for stakeholder alignment.
- Use **Level 2** when operating vfo.
- Use **Level 3** when debugging CI or stage readiness.
