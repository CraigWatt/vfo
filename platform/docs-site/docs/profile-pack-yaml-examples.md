# Profile Pack YAML Examples

This page sketches a pack-per-file declarative shape using the stock packs VFO
already ships.

It is a discussion draft, not the runtime contract yet. Today, `vfo_config.conf`
and the current stock preset blocks remain the source of truth for execution.
The goal here is to show how a future planner/compiler could express the same
pack intent in a cleaner declarative form.

## Why This Shape

- one YAML file per stock pack keeps the pack easy to review
- each pack can list the concrete devices it covers
- the YAML can capture intent before the executor details are chosen
- current `vfo_config.conf` behavior can still be generated or bridged from it

## Balanced Open Audio

This example mirrors the current balanced starter pack:

- `balanced_4k_open_audio`
- `balanced_1080_open_audio`

```yaml
pack:
  id: balanced_open_audio
  title: Balanced Open Audio
  source: stock_pack
  intent:
    audio_policy: preserve
    subtitle_policy: preserve
    quality_mode: standard
    container_preference: mp4
  profiles:
    - id: balanced_4k_open_audio
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_profile
        fallback: transcode_hevc_4k_profile
    - id: balanced_1080_open_audio
      target:
        codec: h264
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 1920
            height: 1080
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_1080_profile
        fallback: transcode_hevc_1080_profile
```

## Device Targets Open Audio

This example keeps the family pack explicit while still listing the concrete
device baselines that each lane is meant to cover.

```yaml
pack:
  id: device_targets_open_audio
  title: Device Targets Open Audio
  source: stock_pack
  intent:
    audio_policy: preserve
    subtitle_policy: preserve
    quality_mode: standard
    container_preference: fmp4
  profiles:
    - id: roku_family_hd_open_audio
      covered_devices:
        - roku_express_1080
      target:
        codec: h264
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 1920
            height: 1080
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_h264_1080_hdr_to_sdr_profile
        fallback: transcode_h264_1080_hdr_to_sdr_profile
    - id: roku_family_4k_open_audio
      covered_devices:
        - roku_4k
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_profile
        fallback: transcode_hevc_4k_profile
    - id: fire_tv_family_hd_open_audio
      covered_devices:
        - fire_tv_stick_lite_1080
      target:
        codec: h264
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 1920
            height: 1080
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_h264_1080_hdr_to_sdr_profile
        fallback: transcode_h264_1080_hdr_to_sdr_profile
    - id: fire_tv_family_4k_open_audio
      covered_devices:
        - fire_tv_stick_4k
        - fire_tv_stick_4k_max
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_profile
        fallback: transcode_hevc_4k_profile
    - id: chromecast_google_tv_hd_open_audio
      covered_devices:
        - chromecast_google_tv_hd
      target:
        codec: h264
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 1920
            height: 1080
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_h264_1080_hdr_to_sdr_profile
        fallback: transcode_h264_1080_hdr_to_sdr_profile
    - id: chromecast_google_tv_4k_open_audio
      covered_devices:
        - chromecast_google_tv_4k
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_profile
        fallback: transcode_hevc_4k_profile
    - id: apple_tv_hd_open_audio
      covered_devices:
        - apple_tv_hd
      target:
        codec: h264
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 1920
            height: 1080
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_h264_1080_hdr_to_sdr_profile
        fallback: transcode_h264_1080_hdr_to_sdr_profile
    - id: apple_tv_4k_open_audio
      covered_devices:
        - apple_tv_4k
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_profile
        fallback: transcode_hevc_4k_profile
    - id: fire_tv_stick_4k_dv_open_audio
      covered_devices:
        - fire_tv_stick_4k_dv
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 352
            height: 240
          max:
            width: 3840
            height: 2160
      runtime_bridge:
        exact_match: copy
        res_too_high: transcode_hevc_4k_dv_profile
        fallback: transcode_hevc_4k_dv_profile
```

## Craigstreamy HEVC Selected English Subtitle Preserve

This example keeps the current intent-driven HEVC pack visible, including the
legacy compatibility bridge that the runtime still understands today.

```yaml
pack:
  id: craigstreamy_hevc_selected_english_subtitle_preserve
  title: Craigstreamy HEVC Selected English Subtitle Preserve
  source: stock_pack
  intent:
    audio_policy: preserve
    subtitle_policy: smart_eng_sub
    quality_mode: standard
    container_preference:
      preserve_with_subtitles: mkv
      otherwise: fmp4
  profiles:
    - id: craigstreamy_hevc_selected_english_subtitle_preserve_4k
      runtime_profile_id: netflixy_preserve_audio_main_subtitle_intent_4k
      target:
        codec: hevc
        color_space: any
        resolution:
          min:
            width: 1920
            height: 1080
          max:
            width: 3840
            height: 2160
      notes:
        - preserve subtitles when the selected English subtitle is intent-oriented
        - emit mkv when subtitle intent applies
    - id: craigstreamy_hevc_selected_english_subtitle_preserve_1080p
      runtime_profile_id: netflixy_preserve_audio_main_subtitle_intent_1080p
      target:
        codec: hevc
        color_space: bt709
        resolution:
          min:
            width: 1280
            height: 720
          max:
            width: 1920
            height: 1080
      notes:
        - sdr-only 1080p lane
    - id: craigstreamy_hevc_selected_english_subtitle_preserve_legacy_subhd
      runtime_profile_id: netflixy_preserve_audio_main_subtitle_intent_legacy_subhd
      target:
        codec: hevc
        color_space: any
        resolution:
          min:
            width: 320
            height: 240
          max:
            width: 1279
            height: 719
      notes:
        - legacy sub-hd lane with broad codec and color intake
```

## What This Would Buy Us

- one pack file could describe intent, lane coverage, and fallback shape
- docs could be generated from the same pack source
- the current stock `vfo_config.conf` blocks could become generated or bridged
- the wizard could still surface the same stock packs, but from a more declarative model

