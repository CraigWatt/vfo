# Roku Family All Sub Convert Audio Conform Pack

This pack provides explicit Roku-oriented HD and 4K delivery lanes with
delivery-friendly subtitle/audio handling.

## Outcome Target

- target broad Roku playback success without using a vague open-video umbrella
- use H.264 for HD delivery and HEVC for UHD delivery
- preserve all subtitles first, but convert text subtitles when MP4-safe
  delivery is possible

## Focus

- Roku family-specific output envelopes
- fragmented MP4 preferred, MKV fallback when subtitle/audio safety requires it
- `audio_conform` for DTS-family and PCM-family sources
- optional video-only `aggressive_vmaf`

## Covered Device Baselines

| Profile | Current device baseline | Notes |
| --- | --- | --- |
| `roku_family_hd_all_sub_convert_audio_conform` | Roku Express 1080-class devices | Conservative HD Roku baseline |
| `roku_family_4k_all_sub_convert_audio_conform` | Roku 4K-class devices | Conservative UHD Roku baseline |

If more Roku models diverge materially later, we should split them into another
profile rather than pretending one shared envelope fits everything forever.

## Included Profiles

- [roku_family_hd_all_sub_convert_audio_conform](../generated/roku-family-hd-all-sub-convert-audio-conform.md)
- [roku_family_4k_all_sub_convert_audio_conform](../generated/roku-family-4k-all-sub-convert-audio-conform.md)

## Pack Flow

```mermaid
flowchart TD
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Candidate media]:::stage --> B{Matches Roku HD envelope?}:::gate
  B -->|Yes| C[roku_family_hd_all_sub_convert_audio_conform]:::stage
  B -->|No| D{Matches Roku 4K envelope?}:::gate
  D -->|Yes| E[roku_family_4k_all_sub_convert_audio_conform]:::stage
  D -->|No| F[Handled by other profile or skipped]:::stage
  C --> G[Profile output written]:::output
  E --> H[Profile output written]:::output
```
