# craigstreamy_hevc_smart_eng_sub_audio_conform

`craigstreamy_hevc_smart_eng_sub_audio_conform` is a stock preset focused on:

- practical HEVC bitrate reduction
- preserving AAC and Dolby-family audio streams when they are already acceptable
- conforming DTS-family and PCM-family audio streams into open-source Dolby-aligned delivery codecs
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
- Codec intake is intentionally broad (`any`) so mezzanines such as HEVC, H.264 (including rare 10-bit), AV1, VP9, and legacy MPEG-2 style assets can be processed.

`smart_eng_sub` heuristic (implemented in action scripts):

- forced english first
- forced untagged/unknown fallback
- optional default english fallback when `VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1`
- non-english forced tracks are intentionally skipped

Audio conform strategy:

- `aac`, `ac3`, `eac3`, and `truehd` are preserved as-is
- DTS-family and PCM-family inputs are conformed stream-by-stream:
  - mono/stereo DTS or PCM -> AAC + loudnorm
  - DTS or PCM 3.0/4.0/5.0/5.1 -> E-AC-3 when available, else AC-3, with loudnorm
  - DTS or PCM > 5.1 -> 5.1 E-AC-3/AC-3 downmix, with loudnorm
- preserved non-MP4-safe audio forces MKV output
- no broad audio bitrate lowering pass is enabled yet; audio policy is conform-only when necessary

Container behavior:

- smart English subtitle found -> MKV output (subtitle-safe preservation)
- no smart English subtitle and preserved audio is MP4-safe -> stream-ready MP4 output:
  - default mode: fragmented MP4 + init/moov at start (`VFO_MP4_STREAM_MODE=fmp4_faststart`)
  - optional modes: `fmp4` or `faststart`
  - E-AC-3 audio falls back to `faststart` MP4 packaging because ffmpeg does not finalize the fragmented mode reliably for that codec
- no smart English subtitle and preserved audio is not MP4-safe -> MKV output

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

- 4K lane attempts Dolby Vision retention/conversion (`P7 -> P8.1`) when source signals DV side data
- 4K lane includes an extraction fallback for Profile 7 MKV sources (`mkvextract` path, then ffmpeg fallback)
- 4K lane defaults to strict DV retention (`VFO_DV_REQUIRE_DOVI=1`) to avoid silent downgrade
- HDR/SDR color signaling is preserved with metadata-repair defaults when source tags are incomplete
- metadata-repair heuristics can be toggled with `VFO_DYNAMIC_METADATA_REPAIR=1|0`
- strict dynamic-range verification can be controlled with `VFO_DYNAMIC_RANGE_STRICT=1|0`

## Included active profiles

- `craigstreamy_hevc_smart_eng_sub_audio_conform_4k`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_1080p`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_legacy_subhd`

## Required action scripts

- `transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh`
- `transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh`
- `transcode_hevc_legacy_smart_eng_sub_audio_conform_profile.sh`
- `audio_conform_tools.sh`
- `dynamic_range_tools.sh`
- `profile_guardrail_skip.sh`

Use `vfo_config.preset.conf` as a copy/paste starter block.
