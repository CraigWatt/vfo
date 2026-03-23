# Profile Action Scripts

These scripts are intended to be called from profile scenario commands in `vfo_config.conf`.

Installed defaults:

- `make install` copies these scripts to `/usr/local/bin`
- macOS `.pkg` releases also install them to `/usr/local/bin`

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
- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`
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
  - Dolby Vision retention path with `dovi_tool`
  - when input is DV profile 7, converts metadata to profile 8.1 before injection
  - gracefully falls back to HDR10-compatible output if DV retention fails
  - supports strict mode via `VFO_DV_REQUIRE_DOVI=1`
  - supports profile 7 conversion controls:
    - `VFO_DV_CONVERT_P7_TO_81=1` (default)
    - `VFO_DV_P7_TO_81_MODE=2|5` (default: `2`)
    - `VFO_DV_REQUIRE_P7_TO_81=1` (default)
- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`
  - preserves all audio streams
  - selects one "main subtitle" using english-speaker heuristics:
    forced english -> forced untagged/unknown -> optional default english
  - explicitly skips non-english forced tracks
  - emits MKV when a main subtitle is selected
  - otherwise emits stream-ready MP4, defaulting to fragmented MP4 with init/moov at start
  - optional env:
    - `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1`
    - `VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart` (default: `fmp4_faststart`)

## Hardware selection override

Set `VFO_ENCODER_MODE` to:

- `auto` (default)
- `hw`
- `cpu`

## Important notes

- Prefer command-name usage in config, for example:
  - `PROFILE_FFMPEG_COMMAND="transcode_hevc_4k_profile.sh $vfo_input $vfo_output"`
- Absolute paths still work when you need custom script locations.
- `transcode_hevc_4k_dv_profile.sh` now enforces profile 7 -> 8.1 conversion by default; for advanced mux/timestamp constraints, extend it with your dedicated workflow.
