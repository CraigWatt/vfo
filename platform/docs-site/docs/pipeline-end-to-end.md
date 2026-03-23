# Pipeline End To End

The default vfo pipeline is:

```mermaid
flowchart LR
  A[Mezzanine] --> B[Mezzanine Clean optional]
  B --> C{KEEP_SOURCE?}
  C -->|true| D[Source]
  C -->|false| E[Profile Stage]
  D --> E
  E --> F{QUALITY_CHECK_ENABLED?}
  F -->|true| G[PSNR/SSIM optional VMAF]
  F -->|false| H[Done]
  G --> H
```

## What Content Can Be Accommodated?

vfo is criteria-driven, so "accommodated" means "matches at least one configured profile criteria envelope and scenario." 

For stock presets, this includes practical lanes for:

- 4K HEVC target outputs
- 1080p compatibility outputs
- subtitle-intent profile outputs (MKV when main subtitle should be preserved)
- device-oriented profiles (Roku/Fire TV/Chromecast/Apple TV families)

Use the [Capability Matrix](profile-capability-matrix.md) for exact codec/bit-depth/color/resolution envelopes.
