# device_targets_open_audio

`device_targets_open_audio` is the older stock preset umbrella for practical
streaming-device targets.

Design goals:

- conservative playback compatibility baselines for common OTT sticks/boxes
- preserve audio/subtitle streams when possible (`-c:a copy -c:s copy`)
- keep profile blocks simple and action-script-driven
- remain available for compatibility while the newer explicit family/device
  packs take over as the clearer default

Included profiles:

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
- Prefer the newer explicit device packs when you want fixed subtitle/audio
  policy in the pack name:
  - `roku_family_all_sub_convert_audio_conform`
  - `fire_tv_family_all_sub_convert_audio_conform`
  - `chromecast_google_tv_family_all_sub_convert_audio_conform`
  - `apple_tv_family_all_sub_convert_audio_conform`
  - `fire_tv_stick_4k_dv_all_sub_convert_audio_conform`
- 1080 SDR-target profiles use explicit HDR->SDR conversion action:
  - `transcode_h264_1080_hdr_to_sdr_profile.sh`
  - when the runner lacks the full HDR tonemap filters, that action now falls back to an SDR-signaled compatibility transcode rather than failing PQ/HLG inputs outright
- 4K HDR/DV-oriented profiles keep preserve-style behavior unless explicitly named otherwise.
- For predictive validation, pair these profiles with:
  - `tests/e2e/validate_device_conformance.sh`
  - `tests/e2e/run_device_conformance_e2e.sh`

These command names work out of the box after `.pkg` install or `make install`.

Use `vfo_config.preset.conf` as a copy/paste starter block.
