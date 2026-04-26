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
- `transcode_hevc_legacy_main_subtitle_preserve_profile.sh`
- `transcode_hevc_4k_all_sub_preserve_profile.sh`
- `transcode_hevc_1080_all_sub_preserve_profile.sh`
- `transcode_hevc_legacy_all_sub_preserve_profile.sh`
- `transcode_hevc_4k_smart_eng_sub_subtitle_convert_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_subtitle_convert_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_subtitle_convert_profile.sh`
- `transcode_hevc_4k_all_sub_audio_conform_profile.sh`
- `transcode_hevc_1080_all_sub_audio_conform_profile.sh`
- `transcode_hevc_legacy_all_sub_audio_conform_profile.sh`
- `transcode_hevc_4k_smart_eng_sub_aggressive_vmaf_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_aggressive_vmaf_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_aggressive_vmaf_profile.sh`
- `transcode_hevc_4k_smart_eng_sub_subtitle_convert_audio_conform_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_subtitle_convert_audio_conform_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_subtitle_convert_audio_conform_profile.sh`
- `transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_audio_conform_profile.sh`
- `transcode_hevc_4k_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh`
- `audio_conform_tools.sh`
- `subtitle_policy_tools.sh`
- `quality_mode_tools.sh`
- `live_encode_tools.sh`
- `transcode_h264_1080_profile.sh`
- `transcode_h264_1080_hdr_to_sdr_profile.sh`
- `transcode_hevc_4k_dv_profile.sh`
- `dynamic_range_tools.sh`
- `profile_guardrail_skip.sh`

Core templates:

- preserve audio and subtitle streams (`-c:a copy -c:s copy`)
- choose hardware encode (`hevc_videotoolbox`) when available by default
- fall back to software (`libx265`) automatically
- keep `aggressive_vmaf` scoped to video-only retry behavior when enabled
- append `_lowered_v_bitrate` to the final output name when an aggressive VMAF pass actually selects a reduced-bitrate candidate
- print live ffmpeg progress by default through the shared `live_encode_tools.sh` wrapper
- write encode output back to the controlling terminal even if a caller redirects stdout/stderr, so long-running child work stays visible in the same session
- set `VFO_LIVE_USE_SCRIPT=0` if you need to disable the pseudo-tty wrapper for debugging
- adjust ffmpeg progress cadence with `VFO_FFMPEG_STATS_PERIOD` if you want slower or faster updates

Device-target templates:

- `transcode_h264_1080_profile.sh`
  - targets conservative 1080p H.264 compatibility lanes
  - preserves transfer characteristics from source (no explicit HDR->SDR conversion)
  - uses `h264_videotoolbox` when available, `libx264` fallback
- `transcode_h264_1080_hdr_to_sdr_profile.sh`
  - explicit HDR->SDR conversion intent for SDR-only compatibility lanes
  - normalizes output signaling to BT.709
  - uses `zscale+tonemap` when available, otherwise falls back to an SDR-signaled compatibility transcode for PQ/HLG inputs instead of failing the lane
  - uses `h264_videotoolbox` when available, `libx264` fallback
- `transcode_hevc_4k_dv_profile.sh`
  - Dolby Vision retention path with `dovi_tool`
  - when input is DV profile 7, prepares a fresh P8.1 MKV before later encode stages
  - validates the prepared DV output before the profile continues
  - gracefully falls back to HDR10-compatible output if DV retention fails
  - supports strict mode via `VFO_DV_REQUIRE_DOVI=1`
  - supports profile 7 conversion controls:
    - `VFO_DV_CONVERT_P7_TO_81=1` (default)
    - `VFO_DV_P7_TO_81_MODE=2|5` (default: `2`)
    - `VFO_DV_REQUIRE_P7_TO_81=1` (default)
- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`
- `transcode_hevc_legacy_main_subtitle_preserve_profile.sh`
  - preserves all audio streams
  - selects one "main subtitle" using english-speaker heuristics:
    forced english -> forced untagged/unknown -> optional default english
  - explicitly skips non-english forced tracks
  - emits MKV when a main subtitle is selected
  - otherwise emits stream-ready MP4, defaulting to fragmented MP4 with init/moov at start
  - preserves dynamic-range signaling with metadata-repair defaults
  - writes per-output `*.dynamic_range_report.txt` sidecars
  - 4K lane prepares DV profile 7 MKV sources as P8.1 before encode when conversion is enabled, then attempts DV retention/injection using the prepared input
  - 4K lane defaults to strict DV retention (`VFO_DV_REQUIRE_DOVI=1`)
  - 4K lane supports profile 7 -> 8.1 conversion controls:
    - `VFO_DV_CONVERT_P7_TO_81=1` (default)
    - `VFO_DV_P7_TO_81_MODE=2|5` (default: `2`)
    - `VFO_DV_REQUIRE_P7_TO_81=1` (default)
    - `VFO_DV_P7_EXTRACT_MODE=auto|mkvextract|ffmpeg` (default: `auto`)
  - 4K lane can use `mkvextract` for profile-7 MKV track extraction during P8.1 preparation
  - all subtitle-intent lanes support dynamic-range controls:
    - `VFO_DYNAMIC_METADATA_REPAIR=1|0` (default: `1`)
    - `VFO_DYNAMIC_RANGE_STRICT=1|0` (default: `1`)
    - `VFO_DYNAMIC_RANGE_REPORT=1|0` (default: `1`)
  - legacy lane variant supports optional interlace-aware deinterlace + stable black-bar auto-crop
  - legacy lane automatically disables crop when selected subtitle stream is bitmap-based
  - optional env:
    - `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1`
    - `VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart` (default: `fmp4_faststart`)
    - `VFO_LEGACY_DEINTERLACE=auto|always|off` (legacy script, default: `auto`)
    - `VFO_LEGACY_AUTOCROP=1|0` (legacy script, default: `1`)

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
