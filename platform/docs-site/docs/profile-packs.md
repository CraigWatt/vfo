# Stock Profile Packs

Current stock packs in vfo:

- `balanced_open_audio`
- `device_targets_open_audio`
- `netflixy_main_subtitle_intent`

## netflixy_main_subtitle_intent

Focus:

- practical Netflix-like bitrate reduction approach
- preserve audio streams
- preserve one "main subtitle" when it appears director-intent oriented
- emit MKV when subtitle intent applies, otherwise stream-ready MP4 (fragmented + init/moov at start by default)

Included active profiles:

- `netflixy_preserve_audio_main_subtitle_intent_4k`
- `netflixy_preserve_audio_main_subtitle_intent_1080p`

## device_targets_open_audio

Focus:

- device-shaped starter profiles
- compatibility-first output envelopes
- open audio stream preservation strategy where possible

## balanced_open_audio

Focus:

- simple balanced 4K and 1080 lanes
- easy starting point before device-specific tuning
