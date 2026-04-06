# Craigstreamy HEVC Smart Eng Sub Aggressive VMAF Pack

This pack is the video-only aggressive quality alias for the preserve-audio craigstreamy baseline.

It keeps:

- `smart_eng_sub + preserve`
- preserved audio streams
- the same container branching as the standard subtitle-intent preserve pack

It adds:

- `QUALITY_MODE=aggressive_vmaf`
- bounded bitrate / CRF pushes on video only
- fallback to standard encode when `libvmaf` is unavailable or the lane is outside the supported retry boundary

Included profiles:

- [craigstreamy_hevc_smart_eng_sub_aggressive_vmaf_4k](../generated/craigstreamy-hevc-smart-eng-sub-aggressive-vmaf-4k.md)
- [craigstreamy_hevc_smart_eng_sub_aggressive_vmaf_1080p](../generated/craigstreamy-hevc-smart-eng-sub-aggressive-vmaf-1080p.md)
- [craigstreamy_hevc_smart_eng_sub_aggressive_vmaf_legacy_subhd](../generated/craigstreamy-hevc-smart-eng-sub-aggressive-vmaf-legacy-subhd.md)
