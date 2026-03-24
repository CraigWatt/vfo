# netflixy_main_subtitle_intent

`netflixy_main_subtitle_intent` is a stock preset focused on:

- Netflix-like practical bitrate reduction
- preserving audio streams
- preserving one "main subtitle" only when it appears director-intent oriented

Input guardrails:

- 1080 lane accepts SDR (`bt709`) inputs in the 1280x720..1920x1080 envelope and skips HDR candidates for this lane.
- 4K lane accepts SDR or HDR candidates in the 1920x1080 to 3840x2160 envelope.
- Legacy sub-HD lane accepts broad codec/color intake in the 320x240..1279x719 envelope.
- Codec intake is intentionally broad (`any`) so mezzanines such as HEVC, H.264 (including rare 10-bit), AV1, and VP9 can be processed.

Main-subtitle heuristic (implemented in action scripts):

- forced english first
- forced untagged/unknown fallback
- optional default english fallback when `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1`
- non-english forced tracks are intentionally skipped

Container behavior:

- main subtitle selected -> MKV output (subtitle-safe preservation)
- no main subtitle selected -> stream-ready MP4 output:
  - default mode: fragmented MP4 + init/moov at start (`VFO_MP4_STREAM_MODE=fmp4_faststart`)
  - optional modes: `fmp4` or `faststart`

Guardrail mismatch behavior:

- candidates that do not satisfy the lane guardrails are marked with a sidecar `*.guardrail_skipped.txt` marker via `profile_guardrail_skip.sh`

Legacy lane processing behavior:

- preserves source frame rate by default (no forced frame-rate conversion)
- deinterlace can run automatically when input is interlaced (`VFO_LEGACY_DEINTERLACE=auto`)
- stable black-bar auto-crop is enabled by default (`VFO_LEGACY_AUTOCROP=1`)
- auto-crop is disabled for files where the selected main subtitle stream is bitmap-based

## Included active profiles (lane 1)

- `netflixy_preserve_audio_main_subtitle_intent_4k`
- `netflixy_preserve_audio_main_subtitle_intent_1080p`
- `netflixy_preserve_audio_main_subtitle_intent_legacy_subhd`

## Reserved profile names (lane 2 scaffold, not enabled by default)

- `netflixy_fmp4_audio_normalized_main_subtitles_4k`
- `netflixy_fmp4_audio_normalized_main_subtitles_1080p`

## Required action scripts

- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`
- `transcode_hevc_legacy_main_subtitle_preserve_profile.sh`
- `profile_guardrail_skip.sh`

Use `vfo_config.preset.conf` as a copy/paste starter block.
