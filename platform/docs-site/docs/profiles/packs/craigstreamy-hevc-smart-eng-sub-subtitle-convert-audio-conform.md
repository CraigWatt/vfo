# Craigstreamy HEVC Smart Eng Sub Subtitle Convert Audio Conform Pack

This pack combines the current `subtitle_convert` subtitle policy with the existing `audio_conform` delivery strategy.

It keeps:

- HEVC video outputs
- `smart_eng_sub` subtitle selection
- preserve-first behavior for AAC and Dolby-family audio

It adds:

- text-first subtitle conversion into `mov_text` when MP4 remains viable
- DTS-family and PCM-family audio conform when needed
- explicit MKV-preserve fallback when another safety rule forces MKV instead of pretending subtitle conversion still happened

Included profiles:

- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform_4k](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform-4k.md)
- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform_1080p](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform-1080p.md)
- [craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform_legacy_subhd](../generated/craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform-legacy-subhd.md)
