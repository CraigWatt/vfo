# chromecast_google_tv_family_all_sub_convert_audio_conform

`chromecast_google_tv_family_all_sub_convert_audio_conform` targets the shared
Chromecast with Google TV HD and 4K device envelopes.

It is designed to:

- target H.264 for the HD family lane
- target HEVC for the 4K family lane
- prefer fragmented MP4 delivery when subtitle/audio safety allows it
- preserve all subtitles first, then convert text subtitles when the delivery
  target wants MP4-safe subtitle text
- preserve AAC and Dolby-family audio where already acceptable
- conform DTS-family and PCM-family audio when needed
- keep `aggressive_vmaf` optional and video-only

Included profiles:

- `chromecast_google_tv_family_hd_all_sub_convert_audio_conform`
- `chromecast_google_tv_family_4k_all_sub_convert_audio_conform`
