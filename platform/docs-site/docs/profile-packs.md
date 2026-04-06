# Stock Profile Packs

Current stock packs in vfo:

- `balanced_open_audio`
- `device_targets_open_audio`
- `craigstreamy_hevc_all_sub_audio_conform`
- `craigstreamy_hevc_all_sub_preserve`
- `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf`
- `craigstreamy_hevc_smart_eng_sub_audio_conform`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform`
- `craigstreamy_hevc_selected_english_subtitle_preserve`

Each pack is an outcome preset family. Use packs to choose the behavior you want first, then tune criteria/actions if needed.

In the sections below, each heading uses a friendly label while the exact selectable pack id is shown inline.

Read [Profile Pack Strategy](profile-pack-strategy.md) for the rule of the road:

- fixed named packs for user-facing selection
- internal policy composition under the hood
- quality modes layered on top instead of multiplying pack names too early

Subtitle behavior is now described via the canonical policy taxonomy:

- `smart_eng_sub`
- `all_sub_preserve`
- `subtitle_convert`

Read [Subtitle Policy](subtitle-policy-taxonomy.md) for the policy model and how current packs map to it.

Quality behavior is also orthogonal to pack names:

- `standard`
- `aggressive_vmaf`

Read [Quality Modes](quality-mode-taxonomy.md) for the quality-mode model and the current implementation boundary.

## Selection Model

Use this shorthand:

- choose a pack for subtitle + audio + delivery intent
- choose a quality mode for how hard video optimization should push

Today that means:

- pack choice is explicit
- `aggressive_vmaf` now exists both as a reusable quality mode and fixed named craigstreamy packs
- subtitle handling is now split more clearly across preserve vs convert variants
- both aggressive packs keep audio policy unchanged and only push video harder

Further pack evolution is documented in [Profile Pack Strategy](profile-pack-strategy.md).

## Craigstreamy HEVC All Sub Preserve

Pack id: `craigstreamy_hevc_all_sub_preserve`

Focus:

- practical HEVC bitrate reduction approach
- subtitle policy: `all_sub_preserve` + `preserve`
- preserve audio streams
- emit MKV when subtitle carry-over is active, otherwise stream-ready MP4
- details + flow: [Craigstreamy HEVC All Sub Preserve Pack](profiles/packs/craigstreamy-hevc-all-sub-preserve.md)

## Craigstreamy HEVC All Sub Audio Conform

Pack id: `craigstreamy_hevc_all_sub_audio_conform`

Focus:

- practical HEVC bitrate reduction approach
- subtitle policy: `all_sub_preserve` + `preserve`
- preserve AAC and Dolby-family audio when already acceptable
- conform DTS-family and PCM-family audio into open-source Dolby-aligned delivery codecs when needed
- keep MKV whenever subtitle carry-over or preserved-audio safety requires it
- details + flow: [Craigstreamy HEVC All Sub Audio Conform Pack](profiles/packs/craigstreamy-hevc-all-sub-audio-conform.md)

## Craigstreamy HEVC Smart Eng Sub Aggressive VMAF

Pack id: `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`

Focus:

- the same `smart_eng_sub + preserve` subtitle posture as the preserve-audio craigstreamy baseline
- preserve audio streams unchanged
- bounded aggressive-VMAF retries on video only
- same container decisions as the preserve-audio baseline
- details + flow: [Craigstreamy HEVC Smart Eng Sub Aggressive VMAF Pack](profiles/packs/craigstreamy-hevc-smart-eng-sub-aggressive-vmaf.md)

## Craigstreamy HEVC Smart Eng Sub Audio Conform Aggressive VMAF

Pack id: `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf`

Focus:

- the same subtitle + audio intent as the standard audio-conform pack
- bounded aggressive-VMAF retries on the video encode stage
- keep audio behavior unchanged while pushing video harder
- details + flow: [Craigstreamy HEVC Smart Eng Sub Audio Conform Aggressive VMAF Pack](profiles/packs/craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf.md)

## Craigstreamy HEVC Smart Eng Sub Audio Conform

Pack id: `craigstreamy_hevc_smart_eng_sub_audio_conform`

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

## Craigstreamy HEVC Smart Eng Sub Subtitle Convert

Pack id: `craigstreamy_hevc_smart_eng_sub_subtitle_convert`

Focus:

- practical HEVC bitrate reduction approach
- subtitle policy: `smart_eng_sub` + `subtitle_convert`
- preserve audio streams
- convert selected text subtitles into delivery-friendly subtitle text
- details + flow: [Craigstreamy HEVC Smart Eng Sub Subtitle Convert Pack](profiles/packs/craigstreamy-hevc-smart-eng-sub-subtitle-convert.md)

## Craigstreamy HEVC Smart Eng Sub Subtitle Convert Audio Conform

Pack id: `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform`

Focus:

- practical HEVC bitrate reduction approach
- subtitle policy: `smart_eng_sub` + `subtitle_convert`
- preserve AAC and Dolby-family audio when already acceptable
- conform DTS-family and PCM-family audio when needed
- convert selected text subtitles into delivery-friendly text when MP4 remains viable
- preserve selected subtitles in MKV rather than pretending conversion succeeded when another constraint forces MKV
- details + flow: [Craigstreamy HEVC Smart Eng Sub Subtitle Convert Audio Conform Pack](profiles/packs/craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform.md)

## Craigstreamy HEVC Selected English Subtitle Preserve

Pack id: `craigstreamy_hevc_selected_english_subtitle_preserve`

Focus:

- practical HEVC bitrate reduction approach
- preserve audio streams
- subtitle policy: `smart_eng_sub` + `preserve`
- emit MKV when subtitle intent applies, otherwise stream-ready MP4 (fragmented + init/moov at start by default)
- prioritize viewing-experience intent over single-container uniformity
- guardrails: 1080 lane is SDR-only (`bt709`) in 1280x720..1920x1080, 4K lane accepts SDR/HDR in 1920x1080..3840x2160, legacy sub-HD lane is 320x240..1279x719 with broad codec/color intake

Included active profiles:

- `Craigstreamy HEVC Selected English Subtitle Preserve 4K`
- `Craigstreamy HEVC Selected English Subtitle Preserve 1080p`
- `Craigstreamy HEVC Selected English Subtitle Preserve Legacy Sub-HD`
- these map to legacy internal profile ids for compatibility, but the docs surface stays canonical `craigstreamy`
- details + flow: [Craigstreamy HEVC Selected English Subtitle Preserve Pack](profiles/packs/craigstreamy-hevc-selected-english-subtitle-preserve.md)

## Device Targets Open Audio

Pack id: `device_targets_open_audio`

Focus:

- device-shaped starter profiles
- compatibility-first output envelopes
- open audio stream preservation strategy where possible
- optimize for broad playback success across popular streaming devices
- details + flow: [Device Targets Open Audio Pack](profiles/packs/device-targets-open-audio.md)

## Balanced Open Audio

Pack id: `balanced_open_audio`

Focus:

- simple balanced 4K and 1080 lanes
- easy starting point before device-specific tuning
- strong baseline when you want predictable outputs with minimal profile complexity
- details + flow: [Balanced Open Audio Pack](profiles/packs/balanced-open-audio.md)
