# device_targets_open_audio

`device_targets_open_audio` is a stock preset pack for practical streaming-device targets.

Design goals:

- conservative playback compatibility baselines for common OTT sticks/boxes
- preserve audio/subtitle streams when possible (`-c:a copy -c:s copy`)
- keep profile blocks simple and action-script-driven

Included aliases:

- `roku_express_1080_open_audio`
- `roku_4k_open_audio`
- `fire_tv_stick_lite_1080_open_audio`
- `fire_tv_stick_4k_open_audio`
- `fire_tv_stick_4k_max_open_audio`
- `chromecast_google_tv_hd_open_audio`
- `chromecast_google_tv_4k_open_audio`
- `apple_tv_hd_open_audio`
- `apple_tv_4k_open_audio`
- `fire_tv_stick_4k_dv_open_audio` (best-effort DV metadata retention action)

Important:

- These are conservative baseline profiles, not a guarantee for every firmware/model revision.
- 1080 SDR-target aliases use explicit HDR->SDR conversion action:
  - `services/vfo/actions/transcode_h264_1080_hdr_to_sdr_profile.sh`
- 4K HDR/DV-oriented aliases keep preserve-style behavior unless explicitly named otherwise.
- For predictive validation, pair these profiles with:
  - `tests/e2e/validate_device_conformance.sh`
  - `tests/e2e/run_device_conformance_e2e.sh`

Use `vfo_config.preset.conf` as a copy/paste starter block.
