# vfo Quality Scoring

`vfo` can run a post-profile quality stage to compare generated profile outputs against a reference layer and report objective metrics.

## Purpose

- provide an automated quality signal after profile generation
- make quality regressions visible in CLI observability output
- optionally fail the run when thresholds are not met

## Stage Placement

When enabled, quality scoring runs in `vfo run` after:

1. mezzanine stage
2. source stage (if `KEEP_SOURCE=true`)
3. profile stage

Status key:

- `stage.quality`

Per-profile status keys:

- `profile.<profile_name>.quality`

## Metrics

Enabled by default when `QUALITY_CHECK_ENABLED=true`:

- PSNR
- SSIM

Optional:

- VMAF (`QUALITY_CHECK_INCLUDE_VMAF=true`, requires `ffmpeg` with `libvmaf`)

## Reference Layer Selection

`QUALITY_CHECK_REFERENCE_LAYER` supports:

- `auto` (default): use `source` when `KEEP_SOURCE=true`, otherwise `mezzanine`
- `source`: always compare profile outputs to source layer
- `mezzanine`: always compare profile outputs to mezzanine layer

## Config Keys

- `QUALITY_CHECK_ENABLED`
- `QUALITY_CHECK_INCLUDE_VMAF`
- `QUALITY_CHECK_STRICT_GATE`
- `QUALITY_CHECK_REFERENCE_LAYER`
- `QUALITY_CHECK_MIN_PSNR`
- `QUALITY_CHECK_MIN_SSIM`
- `QUALITY_CHECK_MIN_VMAF`
- `QUALITY_CHECK_MAX_FILES_PER_PROFILE`

Threshold behavior:

- a threshold value of `0` disables that metric threshold
- when `QUALITY_CHECK_STRICT_GATE=true`, threshold failures/missing references/metric errors fail the run
- when `QUALITY_CHECK_STRICT_GATE=false`, issues are surfaced as warnings and run continues

## Notes

- VMAF availability varies by FFmpeg build; if unavailable, vfo logs a warning and continues with PSNR/SSIM only.
- `QUALITY_CHECK_MAX_FILES_PER_PROFILE=0` means score all detected files for each profile.
