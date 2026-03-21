# Profile Action Scripts

These scripts are intended to be called from profile scenario commands in `vfo_config.conf`.

## Action Contract

Each action accepts:

- argument 1: input media path
- argument 2: output media path

vfo provides these through scenario command placeholders:

- `$vfo_input`
- `$vfo_output`

## Included templates

- `transcode_hevc_4k_profile.sh`
- `transcode_hevc_1080_profile.sh`

Both scripts:

- preserve audio and subtitle streams (`-c:a copy -c:s copy`)
- choose hardware encode (`hevc_videotoolbox`) when available by default
- fall back to software (`libx265`) automatically

## Hardware selection override

Set `VFO_ENCODER_MODE` to:

- `auto` (default)
- `hw`
- `cpu`

## Important notes

- Use absolute script paths in `*_FFMPEG_COMMAND` entries to avoid path ambiguity.
- For advanced Dolby Vision workflows (RPU extract/inject, mkvmerge final mux), use a dedicated custom action script like your existing M1 pipeline.
