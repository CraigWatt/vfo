# Subsystems

This file maps the main repository areas to likely issue classifications and risk levels.

## Core Pipeline Subsystems

| Subsystem | Paths | Typical Work | Default Risk |
| --- | --- | --- | --- |
| input handling | `services/vfo/src/InputHandler/` | path validation, file discovery, ingestion decisions | high |
| mezzanine stage | `services/vfo/src/Mezzanine/`, `services/vfo/src/Mezzanine_Clean/` | mezzanine inspection and hygiene decisions | medium |
| source stage | `services/vfo/src/Source/`, `services/vfo/src/Source_AS/` | source generation and intermediate behavior | high |
| profile routing | `services/vfo/src/Profile/`, `services/vfo/presets/` | profile matching, scenario selection, output intent | high |
| status and observability | `services/vfo/src/Status/` | machine-readable and operator-facing status | medium |
| quality scoring | `services/vfo/src/quality/` | PSNR/SSIM/VMAF gates and quality enforcement | high |
| shared utilities and config | `services/vfo/src/Config/`, `services/vfo/src/Utils/` | config loading, shared helpers, common behavior | medium |

## Execution And Verification Subsystems

| Subsystem | Paths | Typical Work | Default Risk |
| --- | --- | --- | --- |
| profile action scripts | `services/vfo/actions/` | FFmpeg command composition and execution behavior | high |
| unit and service tests | `services/vfo/test/` | unit coverage, service-level verification | medium |
| end-to-end tests | `tests/e2e/` | action-level and media-flow verification | medium |
| docs site and generated docs | `platform/docs-site/`, `infra/scripts/generate-profile-docs.sh` | docs generation and navigation | low |
| GitHub workflows | `.github/workflows/` | CI, release safety, issue automation | high |

## Routing Hints

Use these issue clues to identify likely subsystem ownership:

- bitrate, codec, scenario, profile, or device-target wording usually means `profile routing` or `profile action scripts`
- source, remux, normalize, or intermediate wording usually means `source stage`
- mezzanine, naming, or library hygiene wording usually means `mezzanine stage`
- doctor, status, status-json, or visualize wording usually means `status and observability`
- PSNR, SSIM, VMAF, threshold, or gate wording usually means `quality scoring`
- docs, README, MkDocs, or navigation wording usually means `docs site and generated docs`
- Actions, workflow, release, CI, or autofix wording usually means `GitHub workflows`

## Risk Notes

- Changes in `Profile`, `Source`, `InputHandler`, `quality`, workflow files, or action scripts should be treated as correctness-sensitive until proven otherwise.
- Changes that span more than one of those areas should default to `GPT-5.4 high now` unless the scope is clearly bounded.
- Docs-only work, issue summarization, file discovery, and narrowly scoped cleanup are safe `GPT-5.4 mini first` candidates.
