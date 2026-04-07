# Fire TV Family All Sub Convert Audio Conform Pack

This pack provides shared Fire TV HD and 4K lanes with explicit subtitle/audio
delivery behavior.

## Outcome Target

- target Fire TV family playback envelopes directly
- use H.264 for HD delivery and HEVC for 4K delivery
- keep all subtitles visible in policy terms, but convert text subtitles when
  MP4-safe delivery is available

## Focus

- Fire TV family-specific output envelopes
- fragmented MP4 preferred, MKV fallback when subtitle/audio safety requires it
- `audio_conform` for DTS-family and PCM-family sources
- optional video-only `aggressive_vmaf`

## Covered Device Baselines

| Profile | Current device baseline | Notes |
| --- | --- | --- |
| `fire_tv_family_hd_all_sub_convert_audio_conform` | Fire TV Stick Lite | Conservative HD Fire TV baseline |
| `fire_tv_family_4k_all_sub_convert_audio_conform` | Fire TV Stick 4K and Fire TV Stick 4K Max | Shared conservative UHD baseline; no AV1-specialized path yet |

This pack is family-scoped, but the baselines inside it are still device-driven.
If Fire TV 4K Max or a later Fire TV line needs a materially different spec, it
should get its own profile or pack.

## Included Profiles

- [fire_tv_family_hd_all_sub_convert_audio_conform](../generated/fire-tv-family-hd-all-sub-convert-audio-conform.md)
- [fire_tv_family_4k_all_sub_convert_audio_conform](../generated/fire-tv-family-4k-all-sub-convert-audio-conform.md)

## Pack Flow

```mermaid
flowchart TD
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;

  A[Candidate media]:::stage --> B{Matches Fire TV HD envelope?}:::gate
  B -->|Yes| C[fire_tv_family_hd_all_sub_convert_audio_conform]:::stage
  B -->|No| D{Matches Fire TV 4K envelope?}:::gate
  D -->|Yes| E[fire_tv_family_4k_all_sub_convert_audio_conform]:::stage
  D -->|No| F[Handled by other profile or skipped]:::stage
  C --> G[Profile output written]:::output
  E --> H[Profile output written]:::output
```
