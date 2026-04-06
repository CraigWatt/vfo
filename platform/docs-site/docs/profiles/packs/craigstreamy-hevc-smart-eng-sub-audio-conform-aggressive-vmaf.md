# Craigstreamy HEVC Smart Eng Sub Audio Conform Aggressive VMAF Pack

This pack keeps the existing craigstreamy audio-conform delivery intent, but adds a bounded aggressive-VMAF retry loop on the video encode stage.

It keeps:

- `smart_eng_sub + preserve`
- `audio_conform` for DTS-family and PCM-family inputs
- the same container branching as the standard audio-conform pack

It adds:

- `QUALITY_MODE=aggressive_vmaf`
- bounded bitrate / CRF pushes until the configured VMAF floor is about to be crossed

Current implementation boundary:

- enabled for SDR-driven retry paths first
- falls back to standard encode when `libvmaf` is unavailable
- audio behavior is unchanged from the standard audio-conform pack

Included profiles:

- [craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_4k](../generated/craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf-4k.md)
- [craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_1080p](../generated/craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf-1080p.md)
- [craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_legacy_subhd](../generated/craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf-legacy-subhd.md)
