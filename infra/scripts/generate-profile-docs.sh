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
    output_intent="conditional: MKV when main subtitle intent is detected, otherwise stream-ready MP4 (fragmented + init/moov at start by default)"
  elif printf '%s' "$first_command" | grep -q "ffmpeg"; then
    output_intent="output container and streams are defined directly by the ffmpeg command"
  fi

  criteria_codec_display="$(normalize_criteria_codec "$criteria_codec")"
  criteria_bits_display="$(normalize_criteria_bits "$criteria_bits")"
  criteria_color_display="$(normalize_criteria_color "$criteria_color")"

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
flowchart TD
  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;
  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;
  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;
  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;

  A[Input candidate: mkv or mp4 or mov or mxf]:::stage --> B[Probe codec, bits, color, resolution]:::stage
  B --> C{Matches profile criteria envelope?}:::gate
  C -->|No| Z[Handled by other profile or skipped]:::skip
  C -->|Yes| D[Run subtitle-intent action script]:::stage
  D --> E{Main subtitle detected by heuristic?}:::gate
  E -->|Yes| F[Encode HEVC, copy audio, copy selected subtitle]:::stage
  F --> G[Emit MKV output]:::output
  E -->|No| H[Encode HEVC, copy audio]:::stage
  H --> I[Finalize stream-ready MP4 packaging]:::stage
  I --> J[Emit fragmented MP4 with init/moov at start]:::output
```
MERMAID
    else
      first_scenario_safe="$(mermaid_text "$first_scenario")"
      first_command_safe="$(mermaid_text "$first_command_label")"
      printf '```mermaid\n'
      printf 'flowchart TD\n'
      printf '  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;\n'
      printf '  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;\n'
      printf '  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;\n'
      printf '  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;\n'
      printf '\n'
      printf '  A[Input candidate: mkv or mp4 or mov or mxf]:::stage --> B[Probe codec, bits, color, resolution]:::stage\n'
      printf '  B --> C{Matches profile criteria envelope?}:::gate\n'
      printf '  C -->|No| Z[Handled by other profile or skipped]:::skip\n'
      printf '  C -->|Yes| D{Evaluate scenarios in order}:::gate\n'
      printf '  D --> E[First match: %s]:::stage\n' "${first_scenario_safe:-ELSE}"
      printf '  E --> F[Execute: %s]:::stage\n' "${first_command_safe:-command}"
      printf '  F --> G[Write profile output]:::output\n'
      printf '```\n'
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
  printf -- '- `netflixy_main_subtitle_intent` currently ships lane 1 as active profiles.\n'
} > "$MATRIX_DOC"

echo "Generated profile docs under: ${PROFILE_OUT_DIR#$REPO_ROOT/}"
echo "Updated index: ${PROFILES_INDEX#$REPO_ROOT/}"
echo "Updated matrix: ${MATRIX_DOC#$REPO_ROOT/}"
