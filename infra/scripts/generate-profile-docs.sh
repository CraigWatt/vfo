#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
PRESETS_DIR="$REPO_ROOT/services/vfo/presets"
ACTIONS_DIR="$REPO_ROOT/services/vfo/actions"
DOCS_ROOT="$REPO_ROOT/platform/docs-site/docs"
PROFILE_OUT_DIR="$DOCS_ROOT/profiles/generated"
PROFILES_INDEX="$DOCS_ROOT/profiles/index.md"
MATRIX_DOC="$DOCS_ROOT/profile-capability-matrix.md"

mkdir -p "$PROFILE_OUT_DIR"
rm -f "$PROFILE_OUT_DIR"/*.md

extract_value() {
  sed -E 's/^[^=]+="(.*)"/\1/'
}

to_upper_prefix() {
  printf '%s' "$1" | tr '[:lower:]' '[:upper:]' | tr -c 'A-Z0-9' '_'
}

to_slug() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | tr '_' '-' | tr -cd 'a-z0-9-'
}

normalize_criteria_codec() {
  local value="${1:-}"
  case "$value" in
    ''|any|ANY|Any|'*') printf '%s' "any" ;;
    *) printf '%s' "$value" ;;
  esac
}

normalize_criteria_bits() {
  local value="${1:-}"
  case "$value" in
    ''|0|any|ANY|Any|'*') printf '%s' "any" ;;
    *) printf '%s' "$value" ;;
  esac
}

normalize_criteria_color() {
  local value="${1:-}"
  case "$value" in
    ''|any|ANY|Any|'*') printf '%s' "any" ;;
    *) printf '%s' "$value" ;;
  esac
}

parse_behavior_block() {
  local action_file="$1"
  local behavior

  if [ ! -f "$action_file" ]; then
    return 1
  fi

  behavior="$(sed -n '/^# Behavior:/,/^# Optional env:/p' "$action_file" \
    | sed '1d;$d' \
    | sed -E 's/^# ?//' \
    | sed -E 's/^- +//' \
    | sed '/^$/d' \
    || true)"

  if [ -n "$behavior" ]; then
    printf '%s\n' "$behavior"
    return 0
  fi

  return 1
}

parse_optional_env_block() {
  local action_file="$1"
  local optional_env

  if [ ! -f "$action_file" ]; then
    return 1
  fi

  optional_env="$(sed -n '/^# Optional env:/,/^if \[/p' "$action_file" \
    | sed '1d;$d' \
    | sed -E 's/^# ?//' \
    | sed -E 's/^- +//' \
    | sed -E 's/^[[:space:]]+//' \
    | sed '/^$/d' \
    || true)"

  if [ -n "$optional_env" ]; then
    printf '%s\n' "$optional_env"
    return 0
  fi

  return 1
}

command_label() {
  local command="$1"
  local token

  token="${command%% *}"
  if [ "$token" = "ffmpeg" ]; then
    printf 'direct ffmpeg'
    return 0
  fi

  if [[ "$token" == *.sh ]]; then
    printf '%s' "$token"
    return 0
  fi

  printf '%s' "${token:-command}"
}

mermaid_text() {
  printf '%s' "$1" | tr '"' "'" | tr '|' '/'
}

action_in_suite_profile_actions() {
  case "$1" in
    transcode_hevc_1080_profile.sh|\
    transcode_hevc_4k_profile.sh|\
    transcode_hevc_1080_main_subtitle_preserve_profile.sh|\
    transcode_hevc_4k_main_subtitle_preserve_profile.sh|\
    transcode_hevc_legacy_main_subtitle_preserve_profile.sh|\
    profile_guardrail_skip.sh)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

action_in_suite_device_conformance() {
  case "$1" in
    transcode_hevc_1080_profile.sh|\
    transcode_hevc_4k_profile.sh|\
    transcode_h264_1080_hdr_to_sdr_profile.sh)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

action_in_suite_dv_optional() {
  case "$1" in
    transcode_hevc_4k_dv_profile.sh|\
    transcode_hevc_4k_main_subtitle_preserve_profile.sh)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

write_profile_doc() {
  local pack="$1"
  local profile="$2"
  local criteria_codec="$3"
  local criteria_bits="$4"
  local criteria_color="$5"
  local min_w="$6"
  local min_h="$7"
  local max_w="$8"
  local max_h="$9"
  local file_path="${10}"

  local slug
  local prefix
  local scenarios
  local commands
  local criteria_line
  local heading
  local i
  local scenario
  local command
  local command_token
  local action_file
  local behavior_text
  local optional_env_text
  local scenario_count
  local first_command
  local first_scenario
  local first_command_label
  local profile_doc
  local mermaid_variant
  local typical_input_containers
  local output_intent
  local criteria_codec_display
  local criteria_bits_display
  local criteria_color_display
  local is_craigstreamy_selected_english_subtitle_pack
  local dep_ffmpeg
  local dep_ffprobe
  local dep_mkvmerge
  local dep_mkvextract
  local dep_dovi_tool
  local covered_by_profile_actions
  local covered_by_device_conformance
  local covered_by_dv_optional

  slug="$(to_slug "$profile")"
  prefix="$(to_upper_prefix "$profile")"
  profile_doc="$PROFILE_OUT_DIR/${slug}.md"

  scenarios=()
  while IFS= read -r line; do
    scenarios+=("$line")
  done < <(grep -E "^${prefix}_SCENARIO=\"" "$file_path" | extract_value || true)

  commands=()
  while IFS= read -r line; do
    commands+=("$line")
  done < <(grep -E "^${prefix}_FFMPEG_COMMAND=\"" "$file_path" | extract_value || true)
  scenario_count="${#scenarios[@]}"

  first_command=""
  if [ "${#commands[@]}" -gt 0 ]; then
    first_command="${commands[0]}"
  fi

  first_scenario=""
  if [ "${#scenarios[@]}" -gt 0 ]; then
    first_scenario="${scenarios[0]}"
  fi

  first_command_label="$(command_label "$first_command")"
  mermaid_variant="generic"
  if printf '%s' "$first_command" | grep -q "main_subtitle_preserve_profile.sh"; then
    mermaid_variant="subtitle_intent"
  fi

  typical_input_containers="mkv, mp4, mov, mxf (anything ffmpeg can demux)"
  output_intent="profile-specific output written by selected scenario command"
  if [ "$mermaid_variant" = "subtitle_intent" ]; then
    output_intent="conditional: MKV when selected English subtitle intent is detected, otherwise stream-ready MP4 (fragmented + init/moov at start by default)"
  elif printf '%s' "$first_command" | grep -q "ffmpeg"; then
    output_intent="output container and streams are defined directly by the ffmpeg command"
  fi

  criteria_codec_display="$(normalize_criteria_codec "$criteria_codec")"
  criteria_bits_display="$(normalize_criteria_bits "$criteria_bits")"
  criteria_color_display="$(normalize_criteria_color "$criteria_color")"
  dep_ffmpeg="1"
  dep_ffprobe="1"
  dep_mkvmerge="0"
  dep_mkvextract="0"
  dep_dovi_tool="0"
  covered_by_profile_actions="0"
  covered_by_device_conformance="0"
  covered_by_dv_optional="0"

  i=0
  while [ "$i" -lt "${#commands[@]}" ]; do
    command="${commands[$i]}"
    command_token="${command%% *}"

    if [ "$command_token" = "ffmpeg" ]; then
      dep_ffmpeg="1"
    fi

    if [ -n "$command_token" ] && [[ "$command_token" == *.sh ]]; then
      action_file="$ACTIONS_DIR/$command_token"
      if [ -f "$action_file" ]; then
        if grep -q "mkvmerge" "$action_file"; then
          dep_mkvmerge="1"
        fi
        if grep -q "mkvextract" "$action_file"; then
          dep_mkvextract="1"
        fi
        if grep -q "dovi_tool" "$action_file"; then
          dep_dovi_tool="1"
        fi
      fi

      if action_in_suite_profile_actions "$command_token"; then
        covered_by_profile_actions="1"
      fi
      if action_in_suite_device_conformance "$command_token"; then
        covered_by_device_conformance="1"
      fi
      if action_in_suite_dv_optional "$command_token"; then
        covered_by_dv_optional="1"
      fi
    fi

    i=$((i + 1))
  done

  is_craigstreamy_selected_english_subtitle_pack="0"
  if [ "$pack" = "craigstreamy-hevc-selected-english-subtitle-preserve" ] && [ "$mermaid_variant" = "subtitle_intent" ]; then
    is_craigstreamy_selected_english_subtitle_pack="1"
  fi

  criteria_line="
| Field | Value |
| --- | --- |
| Codec | \`${criteria_codec_display}\` |
| Bit depth | \`${criteria_bits_display}\` |
| Color space | \`${criteria_color_display}\` |
| Min resolution | \`${min_w:-0}x${min_h:-0}\` |
| Max resolution | \`${max_w:-any}x${max_h:-any}\` |
"

  {
    printf '# %s\n\n' "$profile"
    printf 'Generated from stock preset pack `%s`.\n\n' "$pack"
    printf '## Dependencies\n\n'
    printf '| Tool | Needed | Why |\n'
    printf '| --- | --- | --- |\n'
    if [ "$dep_ffmpeg" = "1" ]; then
      printf '| `ffmpeg` | required | scenario execution, encode/transcode, and mux packaging |\n'
    fi
    if [ "$dep_ffprobe" = "1" ]; then
      printf '| `ffprobe` | required | criteria probing and stream/metadata inspection |\n'
    fi
    if [ "$dep_mkvmerge" = "1" ]; then
      printf '| `mkvmerge` | conditional | used by at least one action path in this profile family (MKV/DV helper path) |\n'
    fi
    if [ "$dep_mkvextract" = "1" ]; then
      printf '| `mkvextract` | conditional | optional DV extraction helper path for MKV inputs |\n'
    fi
    if [ "$dep_dovi_tool" = "1" ]; then
      printf '| `dovi_tool` | conditional | required for Dolby Vision retention and profile 7 to 8.1 conversion paths |\n'
    fi
    printf '\n'

    printf '## E2E Verification\n\n'
    printf 'This profile is considered e2e-verified when its mapped suites pass in CI.\n\n'
    if [ "$covered_by_profile_actions" = "0" ] && [ "$covered_by_device_conformance" = "0" ] && [ "$covered_by_dv_optional" = "0" ]; then
      printf -- '- No direct stock-suite mapping currently detected for this profile command set.\n'
    else
      printf '| Suite | What it proves | Toolchain version report |\n'
      printf '| --- | --- | --- |\n'
      if [ "$covered_by_profile_actions" = "1" ]; then
        printf '| `tests/e2e/run_profile_actions_e2e.sh` | action-level output behavior, guardrails, and subtitle-intent pathways | `tests/e2e/.reports/latest/run_profile_actions_e2e_toolchain_versions.md` |\n'
      fi
      if [ "$covered_by_device_conformance" = "1" ]; then
        printf '| `tests/e2e/run_device_conformance_e2e.sh` | conservative device-target conformance for stock targets | `tests/e2e/.reports/latest/run_device_conformance_e2e_toolchain_versions.md` |\n'
      fi
      if [ "$covered_by_dv_optional" = "1" ]; then
        printf '| `tests/e2e/run_dv_metadata_optional_e2e.sh` | optional DV metadata retention and profile 7 to 8.1 checks | `tests/e2e/.reports/latest/run_dv_metadata_optional_e2e_toolchain_versions.md` |\n'
      fi
    fi
    printf '\n'
    printf -- '- Combined toolchain snapshot: [Latest E2E Toolchain Report](../../e2e-toolchain-latest.md)\n\n'

    if [ "$is_craigstreamy_selected_english_subtitle_pack" = "1" ]; then
      printf '## Intent\n\n'
      printf 'This profile converts candidates into streaming-friendly HEVC outputs while preserving selected-English subtitle intent where feasible.\n\n'
      printf '## What It Optimizes For\n\n'
      printf -- '- practical bitrate efficiency with a consistent HEVC target\n'
      printf -- '- preserve all audio streams by default when packaging permits\n'
      printf -- '- preserve one selected English subtitle when detected\n'
      printf -- '- conditional container selection: MKV when selected-English subtitle intent applies, fragmented MP4 otherwise\n'
      if printf '%s' "$first_command" | grep -q "legacy_main_subtitle_preserve_profile.sh"; then
        printf -- '- for legacy sub-HD intake: optional deinterlace and persistent black-bar auto-crop\n'
      fi
      printf '\n'
    fi

    printf '## Input Envelope\n'
    printf '%b\n' "$criteria_line"

    printf '## Scenario Map\n\n'
    if [ "$scenario_count" -eq 0 ]; then
      printf 'No scenarios were detected for this profile in the preset file.\n\n'
    else
      printf '| Scenario | Command |\n'
      printf '| --- | --- |\n'
      i=0
      while [ "$i" -lt "$scenario_count" ]; do
        scenario="${scenarios[$i]}"
        command=""
        if [ "$i" -lt "${#commands[@]}" ]; then
          command="${commands[$i]}"
        fi
        printf '| `%s` | `%s` |\n' "$scenario" "$command"
        i=$((i + 1))
      done
      printf '\n'
    fi

    printf '## Runtime Behavior\n\n'
    if [ "$scenario_count" -gt 0 ]; then
      i=0
      while [ "$i" -lt "$scenario_count" ]; do
        command=""
        if [ "$i" -lt "${#commands[@]}" ]; then
          command="${commands[$i]}"
        fi
        command_token="${command%% *}"
        if [ -n "$command_token" ] && [[ "$command_token" == *.sh ]]; then
          action_file="$ACTIONS_DIR/$command_token"
          if [ -f "$action_file" ]; then
            printf -- '- Scenario `%s` uses action script `%s`.\n' "${scenarios[$i]}" "$command_token"
          else
            printf -- '- Scenario `%s` uses command `%s`.\n' "${scenarios[$i]}" "$command"
          fi
        else
          printf -- '- Scenario `%s` uses direct ffmpeg command execution.\n' "${scenarios[$i]}"
        fi
        i=$((i + 1))
      done
      printf '\n'
    fi

    if [ -n "$first_command" ]; then
      command_token="${first_command%% *}"
      if [ -n "$command_token" ] && [[ "$command_token" == *.sh ]]; then
        action_file="$ACTIONS_DIR/$command_token"
        behavior_text="$(parse_behavior_block "$action_file" || true)"
        if [ -n "$behavior_text" ]; then
          printf 'Action summary from `%s`:\n\n' "$command_token"
          while IFS= read -r line; do
            printf -- '- %s\n' "$line"
          done <<< "$behavior_text"
          printf '\n'
        fi

        optional_env_text="$(parse_optional_env_block "$action_file" || true)"
        if [ -n "$optional_env_text" ]; then
          printf 'Operator knobs from `%s`:\n\n' "$command_token"
          while IFS= read -r line; do
            printf -- '- `%s`\n' "$line"
          done <<< "$optional_env_text"
          printf '\n'
        fi
      fi
    fi

    printf '## Starting Inputs And Expected Outputs\n\n'
    printf '| Aspect | What this profile expects / does |\n'
    printf '| --- | --- |\n'
    printf '| Starting containers | `%s` |\n' "$typical_input_containers"
    printf '| Required codec envelope | `%s` / `%s-bit` / `%s` |\n' "$criteria_codec_display" "$criteria_bits_display" "$criteria_color_display"
    printf '| Required resolution range | `%sx%s` to `%sx%s` |\n' "${min_w:-0}" "${min_h:-0}" "${max_w:-any}" "${max_h:-any}"
    printf '| If criteria do not match | candidate is routed to another profile or skipped |\n'
    printf '| If criteria match | scenario order is evaluated and first match executes |\n'
    printf '| Output intent | %s |\n' "$output_intent"
    printf '\n'

    printf '## Flow\n\n'
    if [ "$mermaid_variant" = "subtitle_intent" ]; then
      cat <<'MERMAID'
```mermaid
flowchart LR
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;

  A[Input candidate: mkv / mp4 / mov / mxf]:::stage --> B[Probe codec bits color resolution]:::stage
  B --> C{Matches profile criteria envelope?}:::gate
  C -->|No| Z[Handled by other profile or guardrail skipped]:::skip
  C -->|Yes| D{Evaluate scenarios in order}:::gate
  D --> E[Execute subtitle-intent action]:::stage
  E --> P[Optional lane-specific pre-processing]:::stage
  P --> F{Selected English subtitle intent detected?}:::gate
  F -->|Yes| G[Encode HEVC + preserve audio + preserve selected English subtitle]:::stage
  G --> H[Emit MKV output]:::output
  F -->|No| I[Encode HEVC + preserve audio]:::stage
  I --> J[Finalize fragmented MP4 + init/moov at start]:::stage
  J --> K[Emit MP4 output]:::output
```
MERMAID
    else
      first_scenario_safe="$(mermaid_text "$first_scenario")"
      first_command_safe="$(mermaid_text "$first_command_label")"
      printf '```mermaid\n'
      printf 'flowchart LR\n'
      printf '  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;\n'
      printf '  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;\n'
      printf '  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;\n'
      printf '  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;\n'
      printf '\n'
      printf '  A[Input candidate: mkv / mp4 / mov / mxf]:::stage --> B[Probe codec bits color resolution]:::stage\n'
      printf '  B --> C{Matches profile criteria envelope?}:::gate\n'
      printf '  C -->|No| Z[Handled by other profile or skipped]:::skip\n'
      printf '  C -->|Yes| D{Evaluate scenarios in order}:::gate\n'
      printf '  D --> E[First match: %s]:::stage\n' "${first_scenario_safe:-ELSE}"
      printf '  E --> F[Execute: %s]:::stage\n' "${first_command_safe:-command}"
      printf '  F --> G[Write profile output artifact]:::output\n'
      printf '```\n'
    fi

    if [ "$is_craigstreamy_selected_english_subtitle_pack" = "1" ]; then
      printf '\n## What This Profile Does Not Do\n\n'
      printf -- '- It does not normalize frame rate; source cadence/timebase is preserved by default.\n'
      printf -- '- It does not transcode audio for target-device compatibility by default.\n'
      printf -- '- It does not guarantee every input audio codec is valid for every selected output container.\n'
      printf -- '- It does not semantically understand subtitle meaning; subtitle selection is metadata and flag driven.\n'
      printf -- '- It does not OCR or convert bitmap subtitles to text subtitles.\n'
      printf -- '- It does not generate ABR ladders (HLS/DASH); output is a single-file artifact.\n'
      printf -- '- It does not certify playback on every device model; profile criteria are compatibility-oriented guardrails.\n'
      printf -- '- It does not enforce PSNR/SSIM/VMAF thresholds unless quality checks are explicitly enabled and configured.\n'
      printf -- '- It does not invent missing HDR/DV essence; metadata repair is heuristic and can be disabled.\n'
      printf -- '- It depends on source integrity and toolchain support for DV/HDR retention; strict mode may fail instead of silently downgrading.\n'
    fi

    printf '\n## Source\n\n'
    printf -- '- Preset file: `%s`\n' "${file_path#$REPO_ROOT/}"
    printf -- '- Generated by: `infra/scripts/generate-profile-docs.sh`\n'
  } > "$profile_doc"

  printf '%s|%s|%s|%s|%s|%s|%s|%s|%s\n' \
    "$profile" \
    "$pack" \
    "$criteria_codec_display" \
    "$criteria_bits_display" \
    "$criteria_color_display" \
    "${min_w:-0}x${min_h:-0}" \
    "${max_w:-any}x${max_h:-any}" \
    "$slug" \
    "$scenario_count"
}

declare -a MATRIX_ROWS=()
declare -a PROFILE_LINKS=()

while IFS= read -r preset_file; do
  [ -f "$preset_file" ] || continue
  pack="$(basename "$(dirname "$preset_file")")"

  profiles=()
  while IFS= read -r line; do
    profiles+=("$line")
  done < <(grep -E '^PROFILE="' "$preset_file" | extract_value || true)

  for profile in "${profiles[@]}"; do
    prefix="$(to_upper_prefix "$profile")"

    criteria_codec="$(grep -E "^${prefix}_CRITERIA_CODEC_NAME=\"" "$preset_file" | head -n1 | extract_value || true)"
    criteria_bits="$(grep -E "^${prefix}_CRITERIA_BITS=\"" "$preset_file" | head -n1 | extract_value || true)"
    criteria_color="$(grep -E "^${prefix}_CRITERIA_COLOR_SPACE=\"" "$preset_file" | head -n1 | extract_value || true)"
    min_w="$(grep -E "^${prefix}_CRITERIA_RESOLUTION_MIN_WIDTH=\"" "$preset_file" | head -n1 | extract_value || true)"
    min_h="$(grep -E "^${prefix}_CRITERIA_RESOLUTION_MIN_HEIGHT=\"" "$preset_file" | head -n1 | extract_value || true)"
    max_w="$(grep -E "^${prefix}_CRITERIA_RESOLUTION_MAX_WIDTH=\"" "$preset_file" | head -n1 | extract_value || true)"
    max_h="$(grep -E "^${prefix}_CRITERIA_RESOLUTION_MAX_HEIGHT=\"" "$preset_file" | head -n1 | extract_value || true)"

    row="$(write_profile_doc "$pack" "$profile" "$criteria_codec" "$criteria_bits" "$criteria_color" "$min_w" "$min_h" "$max_w" "$max_h" "$preset_file")"
    MATRIX_ROWS+=("$row")

    slug="$(to_slug "$profile")"
    PROFILE_LINKS+=("- [$profile](generated/${slug}.md) (pack: $pack)")
  done
done < <(find "$PRESETS_DIR" -type f -name 'vfo_config.preset.conf' | sort)

{
  printf '# Stock Profile Info Sheets\n\n'
  printf 'These pages are generated from stock preset files and linked action scripts.\n\n'
  printf 'Regenerate with:\n\n'
  printf '```bash\n'
  printf 'bash infra/scripts/generate-profile-docs.sh\n'
  printf '```\n\n'
  printf '## Profiles\n\n'
  for line in "${PROFILE_LINKS[@]}"; do
    printf '%s\n' "$line"
  done
} > "$PROFILES_INDEX"

{
  printf '# Profile Capability Matrix\n\n'
  printf 'Generated from stock preset criteria (`PROFILE=` blocks).\n\n'
  printf '| Profile | Pack | Codec | Bits | Color Space | Min Res | Max Res | Scenarios |\n'
  printf '| --- | --- | --- | --- | --- | --- | --- | --- |\n'
  for row in "${MATRIX_ROWS[@]}"; do
    IFS='|' read -r profile pack codec bits color min_res max_res slug scenario_count <<< "$row"
    printf '| [%s](profiles/generated/%s.md) | `%s` | `%s` | `%s` | `%s` | `%s` | `%s` | `%s` |\n' \
      "$profile" "$slug" "$pack" "$codec" "$bits" "$color" "$min_res" "$max_res" "$scenario_count"
  done

  printf '\n## Notes\n\n'
  printf -- '- This matrix reflects stock presets, not every custom profile a user may define.\n'
  printf -- '- `craigstreamy_hevc_selected_english_subtitle_preserve` currently ships lane 1 as active profiles.\n'
} > "$MATRIX_DOC"

echo "Generated profile docs under: ${PROFILE_OUT_DIR#$REPO_ROOT/}"
echo "Updated index: ${PROFILES_INDEX#$REPO_ROOT/}"
echo "Updated matrix: ${MATRIX_DOC#$REPO_ROOT/}"
