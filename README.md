![GitHub Workflow Status (with branch)](https://img.shields.io/github/actions/workflow/status/CraigWatt/vfo/on-push-test.yml?branch=main)
![GitHub all releases](https://img.shields.io/github/downloads/CraigWatt/vfo/total)

# vfo

`vfo` is a command-line utility for batch-processing a video library with FFmpeg.

Instead of applying one preset to every file, `vfo` reads a configuration file, inspects each input video, and chooses an FFmpeg command based on matching profile rules (configured with `PROFILE=` entries). The project is aimed at media-library and streaming workflows where source files vary widely in codec, color space, bit depth, and resolution.

## Why use vfo?

- Batch-process a whole library instead of encoding files one by one.
- Keep different output targets separated with profiles.
- Attach multiple scenario rules to each profile so different source files can be handled differently.
- Preserve folder structure while generating output variants.
- Keep the full flexibility of raw FFmpeg commands by defining them directly in the config file.

## Current project status

- **Actively implemented today:** macOS support is the clearest documented path in the repository.
- **Planned / incomplete:** Linux and Windows support are referenced in project docs, but are still marked as work in progress.
- **Configuration-first workflow:** `vfo` does not do much without a `vfo_config.conf` file.

The repository also includes an in-progress sample config at `services/vfo/src/vfo_config.conf`, which is the best reference for how profiles and scenarios are currently expressed.

## Terminology

- `Mezzanine` = high-quality working library input
- `Source` = normalized intermediate layer
- `Profile` = delivery target definition

Recommended pipeline modes:

1. Default: `mezzanine -> source -> profile`
2. Optional: `mezzanine -> profile` (skip source if your library is already normalized)

## How vfo works

At a high level, `vfo` works like this:

1. Read `vfo_config.conf`.
2. Discover your mezzanine/source locations and profile definitions.
3. For each candidate input file, inspect its media properties.
4. Match the file against profile criteria and scenario conditions.
5. Run the FFmpeg command attached to the first matching scenario.
6. Write output into the profile destination while keeping the source folder layout.

Optional hygiene stage (configurable):

- Run `vfo mezzanine-clean` to audit or normalize mezzanine folder/file naming before encode stages.

Optional quality scoring stage (configurable):

- Run post-profile `PSNR`/`SSIM` checks (and optional `VMAF`) against the selected reference layer (`source`, `mezzanine`, or `auto`).
- Surface per-profile scoring status in run observability output.

### Core concepts

#### Mezzanine and source folders

The sample configuration distinguishes between a `MEZZANINE_LOCATION` and a `SOURCE_LOCATION`. The code also supports a `KEEP_SOURCE` switch, which changes whether profile generation runs from the mezzanine set or from the source set.

#### Profiles

A profile represents a target output collection, such as a compatibility tier for a specific playback device or streaming profile.

Each profile can define criteria such as:

- codec
- bit depth
- color space
- minimum resolution
- maximum resolution
- destination folder

#### Scenarios

Each profile can contain one or more scenarios. A scenario is effectively a condition-expression plus an FFmpeg command.

Examples of scenario conditions visible in the sample config include:

- `CODEC_JUST_RIGHT`
- `RES_TOO_HIGH`
- `FULL_HD_OR_HIGHER`
- `NO_VALID_COLOR_SPACE_DETECTED`
- `ELSE`

When a scenario matches, `vfo` runs the FFmpeg command tied to that scenario.

## Installation

### Option 1: install from a release

The repository's release notes describe two macOS-oriented installation paths:

1. Install the `.pkg` release asset.
2. Download the static build zip and copy the bundled `bin/` commands plus config file.

Release notes: [RELEASE.txt](./RELEASE.txt)

## Release channels

vfo uses three delivery lanes:

- Main snapshot artifacts (on `main` pushes) for internal/beta testing.
- Beta prereleases from tags like `v1.2.3-beta.1`.
- Stable releases from tags like `v1.2.3`.
- Successful `main` push validations auto-create the next stable tag (`vX.Y.Z`) for that commit, which triggers the stable release workflow automatically.

CI/CD test integration:

- Validation workflow (`PR + main`) runs `make ci` with hosted synthetic e2e (`VFO_E2E_ASSET_MODE=synthetic`, `VFO_E2E_MAX_SEEDS=1`).
- Tag release workflows (beta + stable) also run `make ci` on hosted runners before packaging/release.
- Full media-backed e2e runs on the self-hosted runner workflow `.github/workflows/on-self-hosted-e2e.yml`.
- The self-hosted full-media workflow runs automatically on same-repo PRs to `main` and on `main` pushes, and can also be triggered manually with custom `assets_dir`, `clip_duration`, `max_seeds`, and optional DV fixture inputs.
- Use local run mode (`VFO_E2E_ASSET_MODE=local`) when you want to validate against your own mounted media library.
- Codex autonomous loop v1 is available via:
  - `.github/workflows/ci-codex-autonomous-loop.yml` (scheduled sweep + `agent-ready` issue trigger)
  - `.github/workflows/ci-codex-pr-autofix.yml` (bounded CI autofix for Codex-owned PRs only)
  - setup and guardrails doc: `infra/docs/codex-autonomous-loop.md`
- Docs site build + Pages deploy is available via:
  - `.github/workflows/ci-docs-pages.yml` (PR build validation + `main` Pages deploy)

### Option 2: build from source

Prerequisites:

- `gcc`
- `make`
- a reasonably recent `ffmpeg`
- `zsh` available as `/bin/zsh` or `zsh` in your shell environment, since the `Makefile` sets `SHELL := zsh`

Build the binary:

```bash
make all
```

The compiled executable is written to:

```text
services/vfo/bin/vfo
```

#### Install target

The repository includes an install target:

```bash
make install
```

By default this copies the binary under `/usr/local/bin` and copies `services/vfo/src/vfo_config.conf` into `/usr/local/bin/vfo_conf_folder`.
It also copies stock profile-action scripts (`transcode_*_profile.sh`) into `/usr/local/bin`.

## Quick start

1. Install FFmpeg and MKVToolNix (`mkvmerge`).
2. Build or download `vfo`.
3. Ensure `vfo_config.conf` exists at `/usr/local/bin/vfo_conf_folder/vfo_config.conf`.
4. Edit the config file so the folder paths match your machine.
5. Define at least one profile (`PROFILE=`) and one scenario.
6. Run `vfo doctor` to validate environment + config.
7. Run `vfo status` for a high-level readiness snapshot.
8. Run `vfo visualize` for a local workflow view (HTML + JSON + Mermaid).
9. Run `vfo mezzanine-clean` to preview mezzanine hygiene changes (or apply if enabled).
10. Run `vfo run` for the default end-to-end pipeline.
11. Run `vfo` with individual stage commands or a specific profile name when needed.

Example starter flow:

```bash
sudo mkdir -p /usr/local/bin/vfo_conf_folder
sudo cp services/vfo/src/vfo_config.conf /usr/local/bin/vfo_conf_folder/vfo_config.conf
$EDITOR /usr/local/bin/vfo_conf_folder/vfo_config.conf
vfo --help
vfo doctor
vfo status
vfo status-json
vfo visualize
vfo mezzanine-clean
vfo run
vfo profiles
```

## Command-line usage

The help text in the repository currently documents this shape:

```text
vfo [argument] || [options]
```

### Options

- `-h`, `--help` — print help text
- `-v`, `--version` — print version information
- `--no-color` — disable colored output

### Common arguments

- `mezzanine`
- `source`
- `show`
- `wizard`
- `doctor`
- `status`
- `status-json`
- `visualize`
- `run`
- `mezzanine-clean`
- `profiles`
- `wipe`
- any profile name defined in `vfo_config.conf` (via `PROFILE=`)

A useful mental model is:

- use `profiles` to run every configured profile
- use a specific profile name to run only that profile
- use `wipe` together with profile-oriented commands when you want profile outputs removed
- use `doctor` before first run (or after machine/config changes)
- use `status` to view a component-level readiness summary before execution
- use `status-json` in automation/tests when you need machine-readable status
- use `visualize` for local workflow artifacts (`status.json`, Mermaid, HTML)
- use `mezzanine-clean` for optional mezzanine filename/folder hygiene and recommendations
- use `run` for the default pipeline (mezzanine -> source if enabled -> profiles -> optional quality scoring)

## Configuration guide

The sample `services/vfo/src/vfo_config.conf` is long, but the structure is consistent.

For hardware-aware profile actions (for example Apple Silicon encode with CPU fallback), see:

- `services/vfo/actions/`
- `services/vfo/docs/profile-action-schema.md`
- `services/vfo/examples/vfo_config.profile_actions.conf`

For stock shipping profile packs, see:

- `services/vfo/presets/`

For device compatibility conformance checks, see:

- `services/vfo/docs/device-conformance.md`
- `tests/e2e/validate_device_conformance.sh`
- `services/vfo/docs/mezzanine-clean.md`
- `services/vfo/docs/quality-scoring.md`

For engine readiness and observability status output, see:

- `services/vfo/docs/status-observability.md`
- `services/vfo/docs/workflow-visualization.md`
- `services/vfo/docs/workflow-engine.bpmn`
- `services/vfo/docs/workflow-decisions.dmn`

### 1. Set required paths

Start by updating the top-level locations:

- `MEZZANINE_LOCATION`
- `MEZZANINE_LOCATIONS` (semicolon-separated, optional but recommended for multi-drive)
- `MEZZANINE_LOCATION_MAX_USAGE_PCT` (semicolon-separated caps aligned to `MEZZANINE_LOCATIONS`)
- `SOURCE_LOCATION`
- `SOURCE_LOCATIONS` (semicolon-separated, optional but recommended for multi-drive)
- `SOURCE_LOCATION_MAX_USAGE_PCT` (semicolon-separated caps aligned to `SOURCE_LOCATIONS`)
- `SOURCE_AS_LOCATION`

For profile outputs, you can now also define:

- `<PROFILE>_LOCATION`
- `<PROFILE>_LOCATIONS`
- `<PROFILE>_LOCATION_MAX_USAGE_PCT`

When multiple locations are configured, `vfo` selects destinations by projected post-write utilization and available free space, honoring each location cap and keeping a reserve so near-full volumes are avoided.

### 2. Decide global behavior

Examples from the sample file include:

- `KEEP_SOURCE`
- `SOURCE_TEST_ACTIVE`
- `SOURCE_TEST_TRIM_START`
- `SOURCE_TEST_TRIM_DURATION`
- `SOURCE_AS_ACTIVATE`
- `MEZZANINE_CLEAN_ENABLED`
- `MEZZANINE_CLEAN_APPLY_CHANGES`
- `MEZZANINE_CLEAN_APPEND_MEDIA_TAGS`
- `MEZZANINE_CLEAN_STRICT_QUALITY_GATE`
- `QUALITY_CHECK_ENABLED`
- `QUALITY_CHECK_INCLUDE_VMAF`
- `QUALITY_CHECK_STRICT_GATE`
- `QUALITY_CHECK_REFERENCE_LAYER`
- `QUALITY_CHECK_MIN_PSNR`
- `QUALITY_CHECK_MIN_SSIM`
- `QUALITY_CHECK_MIN_VMAF`
- `QUALITY_CHECK_MAX_FILES_PER_PROFILE`

### 3. Define custom folders

The sample uses repeated `CUSTOM_FOLDER="name,type"` entries, where the type is one of the approved values used in code today:

- `films`
- `tv`

### 4. Define a profile

A minimal profile pattern looks like this:

```text
PROFILE="queen"
QUEEN_LOCATION="/path/to/output"
QUEEN_LOCATIONS="/path/to/output;/Volumes/Media-2/path/to/output"
QUEEN_LOCATION_MAX_USAGE_PCT="90;95"
QUEEN_CRITERIA_CODEC_NAME="h264"
QUEEN_CRITERIA_COLOR_SPACE="bt709"
QUEEN_CRITERIA_RESOLUTION_MIN_WIDTH="352"
QUEEN_CRITERIA_RESOLUTION_MIN_HEIGHT="240"
QUEEN_CRITERIA_RESOLUTION_MAX_WIDTH="1920"
QUEEN_CRITERIA_RESOLUTION_MAX_HEIGHT="1080"
```

### 5. Add scenarios and FFmpeg commands

Each scenario is paired with one FFmpeg command:

```text
QUEEN_SCENARIO="RES_TOO_HIGH"
QUEEN_FFMPEG_COMMAND="ffmpeg ... $vfo_input ... $vfo_output"
```

Use these placeholders in your command strings:

- `$vfo_input`
- `$vfo_output`

That lets `vfo` substitute the actual source and destination file paths at runtime.

## Example use cases

### Build a 1080p H.264 compatibility tier

Create a profile for older TVs, low-end streamers, or browser playback. Set the max resolution to 1920x1080, prefer `h264`, and add scenarios that:

- remux compliant files
- downscale larger files
- reject files that are too small

### Build multiple output targets from one library

Define separate profiles for:

- a high-quality archive copy
- a 1080p SDR streaming tier
- a smaller mobile-friendly tier

Then run:

```bash
vfo profiles
```

### Test your workflow on short clips first

The sample config includes source test settings so you can generate trimmed outputs while checking that your profile and FFmpeg logic behaves as expected.

## Development

### Repository layout

```text
infra/
  packaging/
    macos/
platform/
services/
  vfo/
    src/
      ...
    test/
tests/
  e2e/
```

Testing convention:

- Service tests live beside each service in `services/<service-name>/test/`.
- Platform component tests live beside each component in `platform/<component>/test/`.
- Infra validation tests live in `infra/test/`.
- Cross-service end-to-end tests live in `tests/e2e/`.

Naming conventions and terminology policy:

- canonical policy: `AGENT.md`
- latest audit snapshot: `infra/docs/naming-conventions-audit.md`

### Build commands

```bash
make all
make tests
make e2e
make ci
make clean
make clean_tests
```

Note that `make tests` links against `cmocka`, so you may need to install that dependency first.

For full local-media e2e:

```bash
VFO_E2E_ASSET_MODE=local VFO_E2E_ASSETS_DIR="/absolute/path/to/open-source-media" VFO_E2E_MAX_SEEDS=4 make e2e
```

### Docs site

The monorepo docs site source is in `platform/docs-site/`.

Local docs workflow:

```bash
make docs-generate
make docs-build
```

Optional local preview:

```bash
make docs-serve
```

Published docs (when GitHub Pages is enabled for the repository):

- `https://craigwatt.github.io/vfo/`

## Known rough edges

- Some documentation in the repository is still marked as work in progress.
- The install path for `vfo_config.conf` appears inconsistent between the `Makefile` and the runtime error/help text.
- The sample configuration contains roadmap comments and experimental notes alongside active settings.

## Contributing

Contributions that improve documentation, example configs, portability, and installation behavior would all make the project easier to adopt.

- Ask broader questions in [GitHub Discussions](https://github.com/CraigWatt/vfo/discussions/27)
- Report bugs or feature ideas in [GitHub Issues](https://github.com/CraigWatt/vfo/issues)

## Support

If the project is useful to you, consider starring the repository.
