# fire_tv_stick_4k_dv_all_sub_convert_audio_conform

`fire_tv_stick_4k_dv_all_sub_convert_audio_conform` is the explicit Dolby
Vision-capable Fire TV Stick 4K lane.

It is designed to:

- target HEVC 4K output with Dolby Vision retention when possible
- preserve all subtitles first, then convert text subtitles when MP4-safe
  delivery is still possible
- preserve AAC and Dolby-family audio where already acceptable
- conform DTS-family and PCM-family audio when needed
- keep fragmented MP4 as the preferred container, while falling back to MKV when
  subtitle/audio safety demands it
- keep quality mode standard today so DV handling stays predictable

Included profiles:

- `fire_tv_stick_4k_dv_all_sub_convert_audio_conform`
