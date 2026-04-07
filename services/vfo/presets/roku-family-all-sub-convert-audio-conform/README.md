# roku_family_all_sub_convert_audio_conform

`roku_family_all_sub_convert_audio_conform` is the explicit Roku-oriented device
family pack.

It is designed to:

- target H.264 for HD Roku playback envelopes
- target HEVC for UHD Roku playback envelopes
- preserve all subtitles first, but convert text subtitles when the delivery
  spec wants MP4-friendly subtitle form
- preserve AAC and Dolby-family audio when already suitable
- conform DTS-family and PCM-family audio when the output would otherwise miss
  the device-friendly delivery envelope
- keep `aggressive_vmaf` as an optional video-only quality mode

Included profiles:

- `roku_family_hd_all_sub_convert_audio_conform`
- `roku_family_4k_all_sub_convert_audio_conform`
