# Stock Presets

This folder contains stock profile packs that can be used as starting points.

Current preset packs:

- `balanced_open_audio/`
- `device_targets_open_audio/`

## How to use

1. Open your installed config file (for example `/usr/local/bin/vfo_conf_folder/vfo_config.conf`).
2. Copy the profile block from a preset file into your config.
3. Update output paths and action script paths for your environment.
4. Run `vfo doctor`, then `vfo run`.

For predictive playback checks after encoding, use:

- `tests/e2e/validate_device_conformance.sh`
