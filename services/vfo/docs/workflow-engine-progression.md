# vfo Workflow Engine Progression

This map describes how vfo progresses through runtime selections, pipeline branches, and stage outcomes.

Formal model artifacts:

- BPMN: `services/vfo/docs/workflow-engine.bpmn`
- DMN: `services/vfo/docs/workflow-decisions.dmn`
- Local visualization report: `vfo visualize` (see `services/vfo/docs/workflow-visualization.md`)

## Camunda-style progression map

```mermaid
flowchart TD
  A["Start: User runs vfo command"] --> B{"Command type?"}

  B -->|"wizard"| W1["Interactive config setup"]
  W1 --> W2{"Profile setup mode?"}
  W2 -->|"stock"| W3["Select preset packs (single or multi-select)"]
  W2 -->|"custom"| W4["Define custom profile + criteria + scenario commands"]
  W3 --> Z["Config written"]
  W4 --> Z
  Z --> End1["End"]

  B -->|"status/show/doctor"| O1["Observability path"]
  O1 --> O2["Print health + config + dependency state"]
  O2 --> End2["End"]

  B -->|"run / mezzanine/source/profile/revert"| P0["Pipeline execution path"]
  P0 --> P1["Load config + validate dependencies"]
  P1 --> P2{"Pipeline mode?"}

  P2 -->|"mezzanine -> source -> profile (default)"| S1["Stage: mezzanine -> source"]
  P2 -->|"mezzanine -> profile (skip source)"| S2["Skip source stage"]
  S1 --> MUX
  S2 --> MUX

  MUX["Profile stage starts"] --> R0{"Select target profile(s)"}
  R0 --> R1["Match source attributes to scenario rules"]
  R1 --> R2["Execute selected action/script"]

  R2 --> DV0{"DV action path?"}
  DV0 -->|"No"| SUB0
  DV0 -->|"Yes"| DV1{"Source contains DOVI side data?"}
  DV1 -->|"No"| SUB0
  DV1 -->|"Yes"| DV2{"DV profile == 7?"}
  DV2 -->|"No"| DV5["Extract RPU + inject into encoded stream"]
  DV2 -->|"Yes"| DV3["Convert profile 7 RPU to 8.1 (dovi_tool mode 2/5)"]
  DV3 --> DV4{"Conversion successful?"}
  DV4 -->|"No + require_p7_to_81=1"| E1["Fail profile action"]
  DV4 -->|"No + require_p7_to_81=0"| H1["Fallback HDR10-compatible output"]
  DV4 -->|"Yes"| DV5
  DV5 --> DV6{"DV retained?"}
  DV6 -->|"No + require_dovi=1"| E1
  DV6 -->|"No + require_dovi=0"| H1
  DV6 -->|"Yes"| SUB0

  SUB0{"Main-subtitle intent profile?"} -->|"No"| OUT1["Emit faststart MP4/MKV based on profile action"]
  SUB0 -->|"Yes"| SUB1{"Main subtitle found?"}
  SUB1 -->|"Yes"| OUT2["Emit MKV with preserved audio + selected main subtitle"]
  SUB1 -->|"No"| OUT3["Emit faststart MP4 (audio preserved)"]

  OUT1 --> Q0
  OUT2 --> Q0
  OUT3 --> Q0

  Q0{"Quality scoring enabled?"}
  Q0 -->|"No"| FIN["Run summary + status"]
  Q0 -->|"Yes"| Q1["Compare output against source/mezzanine reference"]
  Q1 --> Q2["Compute PSNR/SSIM (+VMAF when enabled)"]
  Q2 --> Q3{"Threshold gates pass?"}
  Q3 -->|"No + strict_gate=1"| E2["Fail run"]
  Q3 -->|"No + strict_gate=0"| WN["Warn + continue"]
  Q3 -->|"Yes"| FIN
  WN --> FIN

  FIN --> End3["End"]
  E1 --> EndErr["End (failed)"]
  E2 --> EndErr
  H1 --> Q0
```

## Selection-driven use cases

- Library normalization first:
  - choose default mode `mezzanine -> source -> profile`
  - use when source layer standardization is part of your workflow
- Direct delivery generation:
  - choose `mezzanine -> profile`
  - use when mezzanine is already normalized and you want speed
- Device compatibility:
  - select device target stock profiles (`roku_*`, `fire_tv_*`, `chromecast_*`, `apple_tv_*`)
  - conformance checks validate codec/resolution/audio boundaries in E2E
- Subtitle intent preservation:
  - use `craigstreamy_hevc_selected_english_subtitle_preserve_*` profiles
  - forced/default english subtitle intent drives MKV vs MP4 container branch
- DV profile 7 source handling:
  - `transcode_hevc_4k_dv_profile.sh` now converts profile 7 metadata to 8.1 before injection
  - strict controls can fail run if conversion or retention is not achieved
