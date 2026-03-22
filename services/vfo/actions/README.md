# Profile Action Scripts

These scripts are intended to be called from profile scenario commands in `vfo_config.conf`.

## Action Contract

Each action accepts:

- argument 1: input media path
- argument 2: output media path

vfo provides these through scenario command placeholders:

- `$vfo_input`
- `$vfo_output`

## Included templates

- `transcode_hevc_4k_profile.sh`
- `transcode_hevc_1080_profile.sh`
- `transcode_h264_1080_profile.sh`
- `transcode_h264_1080_hdr_to_sdr_profile.sh`
- `transcode_hevc_4k_dv_profile.sh`

Core templates:

- preserve audio and subtitle streams (`-c:a copy -c:s copy`)
- choose hardware encode (`hevc_videotoolbox`) when available by default
- fall back to software (`libx265`) automatically

Device-target templates:

- `transcode_h264_1080_profile.sh`
  - targets conservative 1080p H.264 compatibility lanes
  - preserves transfer characteristics from source (no explicit HDR->SDR conversion)
  - uses `h264_videotoolbox` when available, `libx264` fallback
- `transcode_h264_1080_hdr_to_sdr_profile.sh`
  - explicit HDR->SDR conversion intent for SDR-only compatibility lanes
  - normalizes output signaling to BT.709
  - uses `h264_videotoolbox` when available, `libx264` fallback
- `transcode_hevc_4k_dv_profile.sh`
  - best-effort Dolby Vision retention path with `dovi_tool`
  - gracefully falls back to HDR10-compatible output if DV retention fails
  - supports strict mode via `VFO_DV_REQUIRE_DOVI=1`

## Hardware selection override

Set `VFO_ENCODER_MODE` to:

- `auto` (default)
- `hw`
- `cpu`

## Important notes

- Use absolute script paths in `*_FFMPEG_COMMAND` entries to avoid path ambiguity.
- `transcode_hevc_4k_dv_profile.sh` is a best-effort DV baseline; for advanced mux/timestamp constraints, extend it with your dedicated workflow.
