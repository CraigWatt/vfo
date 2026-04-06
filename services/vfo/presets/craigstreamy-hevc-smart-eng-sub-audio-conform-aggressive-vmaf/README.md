# craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf

`craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf` keeps the existing audio-conform pack intent, but adds a bounded aggressive-VMAF retry loop on the video encode stage.

It keeps:

- HEVC video outputs
- `smart_eng_sub + preserve` subtitle handling
- `audio_conform` for DTS-family and PCM-family inputs
- the same output-container rules as the standard audio-conform pack

It adds:

- `QUALITY_MODE=aggressive_vmaf`
- bounded retry search for a more aggressive video encode that still meets the configured VMAF floor

Included profiles:

- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_4k`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_1080p`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_legacy_subhd`
