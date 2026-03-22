# balanced_open_audio

`balanced_open_audio` is a stock profile preset focused on:

- Netflix-like practical bitrate reduction
- strong visual quality at 4K and 1080p targets
- preserving audio/subtitle streams where possible (`-c:a copy -c:s copy`)

## Included profiles

- `balanced_4k_open_audio`
- `balanced_1080_open_audio`

## Encoder behavior

This preset is designed to call reusable action scripts that:

- use Apple VideoToolbox (`hevc_videotoolbox`) when available
- fall back to software (`libx265`) automatically

See:

- `transcode_hevc_4k_profile.sh`
- `transcode_hevc_1080_profile.sh`

These command names work out of the box after `.pkg` install or `make install`.

## File

Use `vfo_config.preset.conf` as a copy/paste starter block.
