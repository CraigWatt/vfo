# Stock Profile Packs

Current stock packs in vfo:

- `balanced_open_audio`
- `device_targets_open_audio`
- `craigstreamy_hevc_smart_eng_sub_audio_conform`
- `craigstreamy_hevc_selected_english_subtitle_preserve`

Each pack is an outcome preset family. Use packs to choose the behavior you want first, then tune criteria/actions if needed.

Subtitle behavior is now described via the canonical policy taxonomy:

- `smart_eng_sub`
- `all_sub_preserve`
- `subtitle_convert`

Read [Subtitle Policy](subtitle-policy-taxonomy.md) for the policy model and how current packs map to it.

## craigstreamy_hevc_smart_eng_sub_audio_conform

Focus:

- practical HEVC bitrate reduction approach
- subtitle policy: `smart_eng_sub` + `preserve`
- preserve AAC and Dolby-family audio when already acceptable
- conform DTS-family audio into open-source Dolby-aligned delivery codecs when needed
- apply loudness normalization only on DTS-family transcode paths
- emit MKV when subtitle or preserved-audio safety requires it, otherwise stream-ready MP4
- guardrails: 1080 lane is SDR-only (`bt709`) in 1280x720..1920x1080, 4K lane accepts SDR/HDR in 1920x1080..3840x2160, legacy sub-HD lane is 320x240..1279x719 with broad codec/color intake

Included active profiles:

- `craigstreamy_hevc_smart_eng_sub_audio_conform_4k`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_1080p`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_legacy_subhd`
- details + flow: [Craigstreamy HEVC Smart Eng Sub Audio Conform Pack](profiles/packs/craigstreamy-hevc-smart-eng-sub-audio-conform.md)

## craigstreamy_hevc_selected_english_subtitle_preserve

Focus:

- practical HEVC bitrate reduction approach
- preserve audio streams
- subtitle policy: `smart_eng_sub` + `preserve`
- emit MKV when subtitle intent applies, otherwise stream-ready MP4 (fragmented + init/moov at start by default)
- prioritize viewing-experience intent over single-container uniformity
- guardrails: 1080 lane is SDR-only (`bt709`) in 1280x720..1920x1080, 4K lane accepts SDR/HDR in 1920x1080..3840x2160, legacy sub-HD lane is 320x240..1279x719 with broad codec/color intake

Included active profiles:

- `netflixy_preserve_audio_main_subtitle_intent_4k`
- `netflixy_preserve_audio_main_subtitle_intent_1080p`
- `netflixy_preserve_audio_main_subtitle_intent_legacy_subhd`
- details + flow: [Craigstreamy HEVC Selected English Subtitle Preserve Pack](profiles/packs/craigstreamy-hevc-selected-english-subtitle-preserve.md)

## device_targets_open_audio

Focus:

- device-shaped starter profiles
- compatibility-first output envelopes
- open audio stream preservation strategy where possible
- optimize for broad playback success across popular streaming devices
- details + flow: [Device Targets Open Audio Pack](profiles/packs/device-targets-open-audio.md)

## balanced_open_audio

Focus:

- simple balanced 4K and 1080 lanes
- easy starting point before device-specific tuning
- strong baseline when you want predictable outputs with minimal profile complexity
- details + flow: [Balanced Open Audio Pack](profiles/packs/balanced-open-audio.md)
