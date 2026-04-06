# Craigstreamy HEVC Smart Eng Sub Subtitle Convert Pack

This pack keeps the `smart_eng_sub` selection heuristic, but changes subtitle handling from preserve-mode to convert-mode.

It keeps:

- HEVC video outputs
- copied audio streams
- `smart_eng_sub + subtitle_convert` subtitle handling
- MP4 when selected subtitles are text-convertible

It is intentionally conservative for bitmap subtitles:

- default behavior is explicit failure
- optional fallback is `VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv`

Included profiles:

- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_4k](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-4k.md)
- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_1080p](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-1080p.md)
- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_legacy_subhd](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-legacy-subhd.md)
