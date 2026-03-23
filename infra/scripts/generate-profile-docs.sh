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
  local scenario_count
  local first_command
  local profile_doc
  local mermaid_variant

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

  criteria_line="
| Field | Value |
| --- | --- |
| Codec | \`${criteria_codec:-any}\` |
| Bit depth | \`${criteria_bits:-any}\` |
| Color space | \`${criteria_color:-any}\` |
| Min resolution | \`${min_w:-0}x${min_h:-0}\` |
| Max resolution | \`${max_w:-any}x${max_h:-any}\` |
"

  first_command=""
  if [ "${#commands[@]}" -gt 0 ]; then
    first_command="${commands[0]}"
  fi

  mermaid_variant="generic"
  if printf '%s' "$first_command" | grep -q "main_subtitle_preserve_profile.sh"; then
    mermaid_variant="subtitle_intent"
  fi

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
      fi
    fi

    printf '## Flow\n\n'
    if [ "$mermaid_variant" = "subtitle_intent" ]; then
      cat <<'MERMAID'
```mermaid
flowchart TD
  A[Candidate matches profile criteria] --> B[Run action script]
  B --> C{Main subtitle found?}
  C -->|Yes| D[Encode HEVC + copy audio + keep one subtitle]
  D --> E[Output MKV]
  C -->|No| F[Encode HEVC + copy audio]
  F --> G[Output MP4 faststart]
```
MERMAID
    else
      cat <<'MERMAID'
```mermaid
flowchart TD
  A[Candidate matches profile criteria] --> B[Evaluate scenario rules]
  B --> C[Execute selected command]
  C --> D[Write profile output]
```
MERMAID
    fi

    printf '\n## Source\n\n'
    printf -- '- Preset file: `%s`\n' "${file_path#$REPO_ROOT/}"
    printf -- '- Generated by: `infra/scripts/generate-profile-docs.sh`\n'
  } > "$profile_doc"

  printf '%s|%s|%s|%s|%s|%s|%s|%s|%s\n' \
    "$profile" \
    "$pack" \
    "${criteria_codec:-any}" \
    "${criteria_bits:-any}" \
    "${criteria_color:-any}" \
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
