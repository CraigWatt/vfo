# Profile Capability Matrix

Generated from stock preset criteria (`PROFILE=` blocks).

| Profile | Pack | Codec | Bits | Color Space | Min Res | Max Res | Scenarios |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [balanced_4k_open_audio](profiles/generated/balanced-4k-open-audio.md) | `balanced_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [balanced_1080_open_audio](profiles/generated/balanced-1080-open-audio.md) | `balanced_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
| [netflixy_preserve_audio_main_subtitle_intent_4k](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-4k.md) | `craigstreamy-hevc-selected-english-subtitle-preserve` | `any` | `any` | `any` | `1920x1080` | `3840x2160` | `2` |
| [netflixy_preserve_audio_main_subtitle_intent_1080p](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md) | `craigstreamy-hevc-selected-english-subtitle-preserve` | `any` | `any` | `bt709` | `1280x720` | `1920x1080` | `2` |
| [netflixy_preserve_audio_main_subtitle_intent_legacy_subhd](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-legacy-subhd.md) | `craigstreamy-hevc-selected-english-subtitle-preserve` | `any` | `any` | `any` | `320x240` | `1279x719` | `2` |
| [craigstreamy_hevc_smart_eng_sub_audio_conform_4k](profiles/generated/craigstreamy-hevc-smart-eng-sub-audio-conform-4k.md) | `craigstreamy-hevc-smart-eng-sub-audio-conform` | `any` | `any` | `any` | `1920x1080` | `3840x2160` | `2` |
| [craigstreamy_hevc_smart_eng_sub_audio_conform_1080p](profiles/generated/craigstreamy-hevc-smart-eng-sub-audio-conform-1080p.md) | `craigstreamy-hevc-smart-eng-sub-audio-conform` | `any` | `any` | `bt709` | `1280x720` | `1920x1080` | `2` |
| [craigstreamy_hevc_smart_eng_sub_audio_conform_legacy_subhd](profiles/generated/craigstreamy-hevc-smart-eng-sub-audio-conform-legacy-subhd.md) | `craigstreamy-hevc-smart-eng-sub-audio-conform` | `any` | `any` | `any` | `320x240` | `1279x719` | `2` |
| [roku_express_1080_open_audio](profiles/generated/roku-express-1080-open-audio.md) | `device_targets_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
| [roku_4k_open_audio](profiles/generated/roku-4k-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [fire_tv_stick_lite_1080_open_audio](profiles/generated/fire-tv-stick-lite-1080-open-audio.md) | `device_targets_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
| [fire_tv_stick_4k_open_audio](profiles/generated/fire-tv-stick-4k-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [fire_tv_stick_4k_max_open_audio](profiles/generated/fire-tv-stick-4k-max-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [chromecast_google_tv_hd_open_audio](profiles/generated/chromecast-google-tv-hd-open-audio.md) | `device_targets_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
| [chromecast_google_tv_4k_open_audio](profiles/generated/chromecast-google-tv-4k-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [apple_tv_hd_open_audio](profiles/generated/apple-tv-hd-open-audio.md) | `device_targets_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
| [apple_tv_4k_open_audio](profiles/generated/apple-tv-4k-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [fire_tv_stick_4k_dv_open_audio](profiles/generated/fire-tv-stick-4k-dv-open-audio.md) | `device_targets_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |

## Notes

- This matrix reflects stock presets, not every custom profile a user may define.
- `craigstreamy_hevc_selected_english_subtitle_preserve` remains the preserve-audio subtitle-intent pack.
- `craigstreamy_hevc_smart_eng_sub_audio_conform` adds DTS/PCM delivery-conform behavior on top of the subtitle-intent family.
