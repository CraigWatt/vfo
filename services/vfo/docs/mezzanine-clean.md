# Mezzanine Clean

`vfo mezzanine-clean` provides an optional mezzanine hygiene pass focused on naming and folder structure only.

It does **not** transcode, remux, or modify media streams.

## Goals

- Normalize mezzanine catalog structure for movie vs TV content.
- Keep filenames Jellyfin-friendly and consistent.
- Optionally append probe-derived tags to filenames.
- Surface warnings when mezzanine candidates look weak for long-term source quality.

## Scope

For each configured mezzanine root, vfo inspects the mezzanine workspace:

- preferred workspace: `<MEZZANINE_ROOT>/start`
- fallback workspace: `<MEZZANINE_ROOT>` if `start` does not exist

## Normalization rules

- Movie target: `Movies/<Movie Title>/<Movie Title [tags]>.ext`
- TV target: `TV Shows/<Show Name>/Season NN/<Show Name - SxxEyy [tags]>.ext`
- TV detection uses `SxxEyy` in the filename.

When media tags are enabled, tags are appended from ffprobe hints:

- resolution: `UHD`, `FHD`, `HD`, `SD`
- dynamic range: `DV`, `HDR10PLUS`, `HDR10`, `HLG`, `SDR`
- video codec token
- first audio codec token

## Quality recommendations

The hygiene pass warns (without failing by default) when it detects signals such as:

- non-mezzanine-grade video codec
- bit depth below 10-bit
- missing transfer metadata
- resolution below 1080p
- lossy first audio codec

Enable strict gating to fail on warnings.

## Config keys

- `MEZZANINE_CLEAN_ENABLED` (`true|false`)
- `MEZZANINE_CLEAN_APPLY_CHANGES` (`true|false`)
- `MEZZANINE_CLEAN_APPEND_MEDIA_TAGS` (`true|false`)
- `MEZZANINE_CLEAN_STRICT_QUALITY_GATE` (`true|false`)

Recommended default for first adoption:

- enabled: `true`
- apply changes: `false` (audit mode)
- append media tags: `true`
- strict gate: `false`

Then switch `MEZZANINE_CLEAN_APPLY_CHANGES=true` once audit output looks right.
