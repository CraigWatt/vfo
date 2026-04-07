# fire_tv_family_all_sub_convert_audio_conform

`fire_tv_family_all_sub_convert_audio_conform` is the shared Fire TV family
pack for the HD and 4K non-DV lanes.

It is designed to:

- target H.264 for HD Fire TV compatibility
- target HEVC for 4K Fire TV compatibility
- prefer fragmented MP4 delivery when subtitle/audio safety allows it
- preserve all subtitles first, but convert text subtitles when the spec wants
  delivery-friendly subtitle text
- preserve AAC and Dolby-family audio where acceptable
- conform DTS-family and PCM-family audio when needed
- keep `aggressive_vmaf` optional and video-only

Included profiles:

- `fire_tv_family_hd_all_sub_convert_audio_conform`
- `fire_tv_family_4k_all_sub_convert_audio_conform`
