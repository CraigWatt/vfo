![GitHub Workflow Status (with branch)](https://img.shields.io/github/actions/workflow/status/CraigWatt/vfo/on-push-test.yml?branch=main)
![GitHub all releases](https://img.shields.io/github/downloads/CraigWatt/vfo/total)

# vfo

`vfo` is a command-line utility for batch-processing a video library with FFmpeg.

Instead of applying one preset to every file, `vfo` reads a configuration file, inspects each input video, and chooses an FFmpeg command based on matching profile rules (currently configured with `ALIAS=` entries). The project is aimed at media-library and streaming workflows where source files vary widely in codec, color space, bit depth, and resolution.

## Why use vfo?

- Batch-process a whole library instead of encoding files one by one.
- Keep different output targets separated with profiles (legacy term in config: aliases).
- Attach multiple scenario rules to each profile so different source files can be handled differently.
- Preserve folder structure while generating output variants.
- Keep the full flexibility of raw FFmpeg commands by defining them directly in the config file.

## Current project status

- **Actively implemented today:** macOS support is the clearest documented path in the repository.
- **Planned / incomplete:** Linux and Windows support are referenced in project docs, but are still marked as work in progress.
- **Configuration-first workflow:** `vfo` does not do much without a `vfo_config.conf` file.

The repository also includes an in-progress sample config at `services/vfo/src/vfo_config.conf`, which is the best reference for how profiles (aliases) and scenarios are currently expressed.

## Terminology transition

vfo is moving toward this wording:

- `Mezzanine` = high-quality working library input (legacy command term: `original`)
- `Source` = normalized intermediate layer
- `Profile` = delivery target definition (legacy config/runtime term: `alias`)

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

### Core concepts

#### Mezzanine and source folders

The sample configuration distinguishes between an `ORIGINAL_LOCATION` and a `SOURCE_LOCATION`. The code also supports a `KEEP_SOURCE` switch, which changes whether profile generation runs from the original set or from the source set.

#### Profiles (legacy: aliases)

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
2. Download the static build zip and copy the binary plus config file into your PATH.

Release notes: [RELEASE.txt](./RELEASE.txt)

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

> **Important:** the current config-loading messages in the source code still tell users that `vfo_config.conf` must live directly in `/usr/local/bin`. If you use `make install`, verify where your config file ends up and move or link it as needed for your environment.

## Quick start

1. Install FFmpeg.
2. Build or download `vfo`.
3. Copy `services/vfo/src/vfo_config.conf` to the location your installation expects.
4. Edit the config file so the folder paths match your machine.
5. Define at least one profile (`ALIAS=` today) and one scenario.
6. Run `vfo` with either a built-in command or a profile name.

Example starter flow:

```bash
cp services/vfo/src/vfo_config.conf /usr/local/bin/vfo_config.conf
$EDITOR /usr/local/bin/vfo_config.conf
vfo --help
vfo all_aliases
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

- `original`
- `source`
- `all_aliases`
- `do_it_all`
- `wipe`
- any profile name defined in `vfo_config.conf` (currently via `ALIAS=`)

A useful mental model is:

- use `all_aliases` to run every configured profile
- use a specific profile name to run only that profile
- use `wipe` together with profile-oriented commands when you want profile outputs removed

## Configuration guide

The sample `services/vfo/src/vfo_config.conf` is long, but the structure is consistent.

### 1. Set required paths

Start by updating the top-level locations:

- `ORIGINAL_LOCATION`
- `SOURCE_LOCATION`
- `SOURCE_AS_LOCATION`

### 2. Decide global behavior

Examples from the sample file include:

- `KEEP_SOURCE`
- `SOURCE_TEST_ACTIVE`
- `SOURCE_TEST_TRIM_START`
- `SOURCE_TEST_TRIM_DURATION`
- `SOURCE_AS_ACTIVATE`
- `ALIASES_ON`

### 3. Define custom folders

The sample uses repeated `CUSTOM_FOLDER="name,type"` entries, where the type is one of the approved values used in code today:

- `films`
- `tv`

### 4. Define a profile (legacy config keyword: `ALIAS=`)

A minimal profile pattern looks like this:

```text
ALIAS="queen"
QUEEN_LOCATION="/path/to/output"
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
vfo all_aliases
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
      Alias/
      Config/
      InputHandler/
      Original/
      Source/
      Source_AS/
      Utils/
    test/
tests/
  e2e/
```

Testing convention:

- Service tests live beside each service in `services/<service-name>/test/`.
- Platform component tests live beside each component in `platform/<component>/test/`.
- Infra validation tests live in `infra/test/`.
- Cross-service end-to-end tests live in `tests/e2e/`.

### Build commands

```bash
make all
make tests
make clean
make clean_tests
```

Note that `make tests` links against `cmocka`, so you may need to install that dependency first.

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
