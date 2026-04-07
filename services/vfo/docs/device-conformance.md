# Device Conformance Checks

This document explains the predictive device compatibility checks used in `tests/e2e/validate_device_conformance.sh`.

## Purpose

Real playback testing on every hardware device is not practical in CI.  
Conformance checks provide a fast "will this likely play?" signal by validating output media properties against conservative per-device caps.

## Current target ids

- `roku_express_1080`
- `roku_4k`
- `fire_tv_stick_lite_1080`
- `fire_tv_stick_4k`
- `fire_tv_stick_4k_max`
- `chromecast_google_tv_hd`
- `chromecast_google_tv_4k`
- `apple_tv_hd`
- `apple_tv_4k`

## What is validated

- primary video codec is in target allowed set
- video resolution is within target max dimensions
- frame rate does not exceed target max
- pixel format is in target allowed set
- HDR transfer compatibility for SDR-only targets
- each audio stream codec is in allowed set
- each audio stream channel count is within cap

## Profile intent guidance

- Use explicit conversion profiles for SDR-only targets (for example HDR->SDR H.264 1080).
- Keep preserve-style HDR/DV profiles separate and explicitly named.
- Avoid mixing transfer-intent behavior inside one generic profile name.

## Pack To Device Mapping

These newer packs are family-scoped, but the profiles inside them map to
concrete device baselines:

| Pack profile | Current conformance target(s) |
| --- | --- |
| `roku_family_hd_all_sub_convert_audio_conform` | `roku_express_1080` |
| `roku_family_4k_all_sub_convert_audio_conform` | `roku_4k` |
| `fire_tv_family_hd_all_sub_convert_audio_conform` | `fire_tv_stick_lite_1080` |
| `fire_tv_family_4k_all_sub_convert_audio_conform` | `fire_tv_stick_4k`, `fire_tv_stick_4k_max` |
| `chromecast_google_tv_family_hd_all_sub_convert_audio_conform` | `chromecast_google_tv_hd` |
| `chromecast_google_tv_family_4k_all_sub_convert_audio_conform` | `chromecast_google_tv_4k` |
| `apple_tv_family_hd_all_sub_convert_audio_conform` | `apple_tv_hd` |
| `apple_tv_family_4k_all_sub_convert_audio_conform` | `apple_tv_4k` |

The DV lane is intentionally separate:

- `fire_tv_stick_4k_dv_all_sub_convert_audio_conform`

That one still needs its own dedicated verification lane instead of only riding
the shared 4K baseline checks.

## Important limits

- This is a conservative baseline model, not a vendor certification.
- Firmware updates, app-level decoder behavior, and passthrough settings can still affect playback.
- Use this as an automated risk reduction tool, then verify with real-device playback for release-critical media.

## Related files

- validator: `tests/e2e/validate_device_conformance.sh`
- e2e harness: `tests/e2e/run_device_conformance_e2e.sh`
- stock presets:
  - `services/vfo/presets/roku-family-all-sub-convert-audio-conform/`
  - `services/vfo/presets/fire-tv-family-all-sub-convert-audio-conform/`
  - `services/vfo/presets/chromecast-google-tv-family-all-sub-convert-audio-conform/`
  - `services/vfo/presets/apple-tv-family-all-sub-convert-audio-conform/`
  - `services/vfo/presets/fire-tv-stick-4k-dv-all-sub-convert-audio-conform/`
  - `services/vfo/presets/device_targets_open_audio/` (legacy compatibility)
