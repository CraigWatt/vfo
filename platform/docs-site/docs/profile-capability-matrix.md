# Profile Capability Matrix

Generated from stock preset criteria (`PROFILE=` blocks).

| Profile | Pack | Codec | Bits | Color Space | Min Res | Max Res | Scenarios |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [balanced_4k_open_audio](profiles/generated/balanced-4k-open-audio.md) | `balanced_open_audio` | `hevc` | `any` | `bt709` | `352x240` | `3840x2160` | `3` |
| [balanced_1080_open_audio](profiles/generated/balanced-1080-open-audio.md) | `balanced_open_audio` | `h264` | `any` | `bt709` | `352x240` | `1920x1080` | `3` |
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
| [netflixy_preserve_audio_main_subtitle_intent_4k](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-4k.md) | `netflixy_main_subtitle_intent` | `hevc` | `10` | `bt2020nc` | `1920x1080` | `3840x2160` | `1` |
| [netflixy_preserve_audio_main_subtitle_intent_1080p](profiles/generated/netflixy-preserve-audio-main-subtitle-intent-1080p.md) | `netflixy_main_subtitle_intent` | `h264` | `8` | `bt709` | `352x240` | `1920x1080` | `1` |

## Notes

- This matrix reflects stock presets, not every custom profile a user may define.
- `netflixy_main_subtitle_intent` currently ships lane 1 as active profiles.
