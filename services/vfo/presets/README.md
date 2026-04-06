# Stock Presets

This folder contains stock profile packs that can be used as starting points.

Current preset packs:

- `balanced_open_audio/`
- `device_targets_open_audio/`
- `craigstreamy-hevc-all-sub-preserve/`
- `craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf/`
- `craigstreamy-hevc-selected-english-subtitle-preserve/`
- `craigstreamy-hevc-smart-eng-sub-audio-conform/`
- `craigstreamy-hevc-smart-eng-sub-subtitle-convert/`

Canonical subtitle policy vocabulary now lives in `platform/docs-site/docs/subtitle-policy-taxonomy.md`.
Quality mode vocabulary now lives in `platform/docs-site/docs/quality-mode-taxonomy.md`.
Pack-selection strategy now lives in `platform/docs-site/docs/profile-pack-strategy.md`.

Recommended mental model:

- packs are fixed named delivery intents
- subtitle, audio, and quality are internally composable policies
- quality modes should layer on top before becoming new stock pack names

## How to use

1. Open your installed config file (for example `/usr/local/bin/vfo_conf_folder/vfo_config.conf`).
2. Copy the profile block from a preset file into your config.
3. Update output paths for your environment.
4. Keep script command names as-is if you installed via `.pkg` or `make install`.
5. Run `vfo doctor`, then `vfo run`.

For predictive playback checks after encoding, use:

- `tests/e2e/validate_device_conformance.sh`
