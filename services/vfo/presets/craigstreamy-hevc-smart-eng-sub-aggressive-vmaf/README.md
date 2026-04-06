# craigstreamy-hevc-smart-eng-sub-aggressive-vmaf

Fixed-name `craigstreamy` stock pack for:

- HEVC video
- `smart_eng_sub + preserve` subtitle policy
- preserved audio streams
- aggressive VMAF retry policy on video only

This pack keeps the audio strategy identical to `craigstreamy_hevc_selected_english_subtitle_preserve`.
Only the video encode path becomes more aggressive, with bounded VMAF-guided retries when the lane and toolchain support it.
