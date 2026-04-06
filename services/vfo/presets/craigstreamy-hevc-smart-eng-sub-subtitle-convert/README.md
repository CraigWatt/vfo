# craigstreamy_hevc_smart_eng_sub_subtitle_convert

`craigstreamy_hevc_smart_eng_sub_subtitle_convert` keeps the craigstreamy smart English subtitle selection logic, but converts selected text subtitles into delivery-friendly text form instead of preserving them verbatim.

It keeps:

- HEVC video outputs
- copied audio streams
- `smart_eng_sub + subtitle_convert` subtitle handling
- MP4 when selected subtitles are text-convertible

Bitmap subtitle conversion is intentionally conservative: it fails by default unless `VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv`.

Included profiles:

- `craigstreamy_hevc_smart_eng_sub_subtitle_convert_4k`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert_1080p`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert_legacy_subhd`
