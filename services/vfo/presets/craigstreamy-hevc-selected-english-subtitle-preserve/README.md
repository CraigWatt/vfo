# craigstreamy_hevc_selected_english_subtitle_preserve

`craigstreamy_hevc_selected_english_subtitle_preserve` is a stock preset focused on:

- practical HEVC bitrate reduction
- preserving audio streams
- subtitle policy: `smart_eng_sub` + `preserve`
- preserving HDR/DV signaling when source essence supports it (strict by default on 4K lane)

Canonical subtitle policy:

- selection scope: `smart_eng_sub`
- handling mode: `preserve`

See `platform/docs-site/docs/subtitle-policy-taxonomy.md` for the shared subtitle-policy taxonomy used across `craigstreamy` packs.

Input guardrails:

- 1080 lane accepts SDR (`bt709`) inputs in the 1280x720..1920x1080 envelope and skips HDR candidates for this lane.
- 4K lane accepts SDR or HDR candidates in the 1920x1080 to 3840x2160 envelope.
- Legacy sub-HD lane accepts broad codec/color intake in the 320x240..1279x719 envelope.
- Codec intake is intentionally broad (`any`) so mezzanines such as HEVC, H.264 (including rare 10-bit), AV1, and VP9 can be processed.

`smart_eng_sub` heuristic (implemented in action scripts):

- forced english first
- forced untagged/unknown fallback
- optional default english fallback when `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1`
- non-english forced tracks are intentionally skipped

Container behavior:

- selected English subtitle found -> MKV output (subtitle-safe preservation)
- no selected English subtitle -> stream-ready MP4 output:
  - default mode: fragmented MP4 + init/moov at start (`VFO_MP4_STREAM_MODE=fmp4_faststart`)
  - optional modes: `fmp4` or `faststart`

Guardrail mismatch behavior:

- candidates that do not satisfy the lane guardrails are marked with a sidecar `*.guardrail_skipped.txt` marker via `profile_guardrail_skip.sh`

Legacy lane processing behavior:

- preserves source frame rate by default (no forced frame-rate conversion)
- deinterlace can run automatically when input is interlaced (`VFO_LEGACY_DEINTERLACE=auto`)
- stable black-bar auto-crop is enabled by default (`VFO_LEGACY_AUTOCROP=1`)
- auto-crop is disabled for files where the selected subtitle stream is bitmap-based
- bitmap subtitles are preserved only; this pack does not implement `subtitle_convert`
- metadata notes are emitted per output in `*.dynamic_range_report.txt`

Dynamic-range behavior:

- 4K lane now attempts Dolby Vision retention/conversion (`P7 -> P8.1`) when source signals DV side data.
- 4K lane includes an extraction fallback for Profile 7 MKV sources (`mkvextract` path, then ffmpeg fallback).
- 4K lane defaults to strict DV retention (`VFO_DV_REQUIRE_DOVI=1`) to avoid silent downgrade.
- HDR/SDR color signaling is preserved with metadata-repair defaults when source tags are incomplete.
- metadata-repair heuristics can be toggled with `VFO_DYNAMIC_METADATA_REPAIR=1|0`.
- strict dynamic-range verification can be controlled with `VFO_DYNAMIC_RANGE_STRICT=1|0`.

## Included active profiles (lane 1)

- `Craigstreamy HEVC Selected English Subtitle Preserve 4K`
- `Craigstreamy HEVC Selected English Subtitle Preserve 1080p`
- `Craigstreamy HEVC Selected English Subtitle Preserve Legacy Sub-HD`

Compatibility note:

- the canonical pack name is `craigstreamy_hevc_selected_english_subtitle_preserve`
- the internal `PROFILE=` ids in this preset block still use older legacy names for compatibility

## Reserved profile names (lane 2 scaffold, not enabled by default)

- `craigstreamy_fmp4_audio_normalized_selected_english_subtitles_4k`
- `craigstreamy_fmp4_audio_normalized_selected_english_subtitles_1080p`

## Required action scripts

- `transcode_hevc_4k_main_subtitle_preserve_profile.sh`
- `transcode_hevc_1080_main_subtitle_preserve_profile.sh`
- `transcode_hevc_legacy_main_subtitle_preserve_profile.sh`
- `dynamic_range_tools.sh`
- `profile_guardrail_skip.sh`

Use `vfo_config.preset.conf` as a copy/paste starter block.
