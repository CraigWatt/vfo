# netflixy_main_subtitle_intent

`netflixy_main_subtitle_intent` is a stock preset focused on:

- Netflix-like practical bitrate reduction
- preserving audio streams
- preserving one "main subtitle" only when it appears director-intent oriented

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

## Included active profiles (lane 1)

- `netflixy_preserve_audio_main_subtitle_intent_4k`
- `netflixy_preserve_audio_main_subtitle_intent_1080p`

## Reserved profile names (lane 2 scaffold, not enabled by default)

- `netflixy_fmp4_audio_normalized_main_subtitles_4k`
- `netflixy_fmp4_audio_normalized_main_subtitles_1080p`

## Required action scripts

- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`

Use `vfo_config.preset.conf` as a copy/paste starter block.
