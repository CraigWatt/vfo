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

friendly_token() {
  local token="$1"
  case "$token" in
    av1) printf '%s' "AV1" ;;
    hevc) printf '%s' "HEVC" ;;
    h264) printf '%s' "H.264" ;;
    4k) printf '%s' "4K" ;;
    1080p) printf '%s' "1080p" ;;
    1080) printf '%s' "1080" ;;
    hd) printf '%s' "HD" ;;
    dv) printf '%s' "DV" ;;
    tv) printf '%s' "TV" ;;
    eng) printf '%s' "English" ;;
    sub) printf '%s' "Subtitle" ;;
    subhd) printf '%s' "Sub-HD" ;;
    aac) printf '%s' "AAC" ;;
    pcm) printf '%s' "PCM" ;;
    dts) printf '%s' "DTS" ;;
    vmaf) printf '%s' "VMAF" ;;
    *)
      printf '%s%s' \
        "$(printf '%s' "$token" | cut -c1 | tr '[:lower:]' '[:upper:]')" \
        "$(printf '%s' "$token" | cut -c2-)"
      ;;
  esac
}

doc_slug_for_profile() {
  local pack="$1"
  local profile="$2"
  case "$profile" in
    netflixy_preserve_audio_main_subtitle_intent_4k)
      printf '%s' "craigstreamy-hevc-selected-english-subtitle-preserve-4k"
      ;;
    netflixy_preserve_audio_main_subtitle_intent_1080p)
      printf '%s' "craigstreamy-hevc-selected-english-subtitle-preserve-1080p"
      ;;
    netflixy_preserve_audio_main_subtitle_intent_legacy_subhd)
      printf '%s' "craigstreamy-hevc-selected-english-subtitle-preserve-legacy-subhd"
      ;;
    *)
      to_slug "$profile"
      ;;
  esac
}

friendly_lane_label() {
  case "$1" in
    4k) printf '%s' "4K" ;;
    1080p) printf '%s' "1080p" ;;
    1080) printf '%s' "1080" ;;
    legacy_subhd) printf '%s' "Legacy Sub-HD" ;;
    *) printf '%s' "$(friendly_token "$1")" ;;
  esac
}

doc_label_for_profile() {
  local pack="$1"
  local profile="$2"
  local token
  local parts
  local words=()
  local label
  local lane

  case "$profile" in
    netflixy_preserve_audio_main_subtitle_intent_4k)
      printf '%s' "Craigstreamy HEVC Selected English Subtitle Preserve 4K"
      return 0
      ;;
    netflixy_preserve_audio_main_subtitle_intent_1080p)
      printf '%s' "Craigstreamy HEVC Selected English Subtitle Preserve 1080p"
      return 0
      ;;
    netflixy_preserve_audio_main_subtitle_intent_legacy_subhd)
      printf '%s' "Craigstreamy HEVC Selected English Subtitle Preserve Legacy Sub-HD"
      return 0
      ;;
  esac

  case "$profile" in
    craigstreamy_hevc_all_sub_audio_conform_*)
      lane="${profile##craigstreamy_hevc_all_sub_audio_conform_}"
      printf 'Craigstreamy HEVC All Subtitles Audio Conform %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_all_sub_preserve_*)
      lane="${profile##craigstreamy_hevc_all_sub_preserve_}"
      printf 'Craigstreamy HEVC All Subtitles Preserve %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_smart_eng_sub_aggressive_vmaf_*)
      lane="${profile##craigstreamy_hevc_smart_eng_sub_aggressive_vmaf_}"
      printf 'Craigstreamy HEVC Smart English Subtitle Aggressive VMAF %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_*)
      lane="${profile##craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf_}"
      printf 'Craigstreamy HEVC Smart English Subtitle Audio Conform Aggressive VMAF %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_smart_eng_sub_audio_conform_*)
      lane="${profile##craigstreamy_hevc_smart_eng_sub_audio_conform_}"
      printf 'Craigstreamy HEVC Smart English Subtitle Audio Conform %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform_*)
      lane="${profile##craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform_}"
      printf 'Craigstreamy HEVC Smart English Subtitle Convert Audio Conform %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
    craigstreamy_hevc_smart_eng_sub_subtitle_convert_*)
      lane="${profile##craigstreamy_hevc_smart_eng_sub_subtitle_convert_}"
      printf 'Craigstreamy HEVC Smart English Subtitle Convert %s' "$(friendly_lane_label "$lane")"
      return 0
      ;;
  esac

  IFS='_' read -r -a parts <<< "$profile"
  for token in "${parts[@]}"; do
    words+=("$(friendly_token "$token")")
  done
  printf '%s' "${words[*]}"
}

doc_title_for_profile() {
  local pack="$1"
  local profile="$2"
  printf '%s Profile' "$(doc_label_for_profile "$pack" "$profile")"
}

pack_label_for_docs() {
  case "$1" in
    balanced_open_audio) printf '%s' "Balanced Open Audio" ;;
    roku-family-all-sub-convert-audio-conform) printf '%s' "Roku Family All Sub Convert Audio Conform" ;;
    fire-tv-family-all-sub-convert-audio-conform) printf '%s' "Fire TV Family All Sub Convert Audio Conform" ;;
    chromecast-google-tv-family-all-sub-convert-audio-conform) printf '%s' "Chromecast Google TV Family All Sub Convert Audio Conform" ;;
    apple-tv-family-all-sub-convert-audio-conform) printf '%s' "Apple TV Family All Sub Convert Audio Conform" ;;
    fire-tv-stick-4k-dv-all-sub-convert-audio-conform) printf '%s' "Fire TV Stick 4K DV All Sub Convert Audio Conform" ;;
    device_targets_open_audio) printf '%s' "Device Targets Open Audio (Legacy)" ;;
    craigstreamy-hevc-all-sub-preserve) printf '%s' "Craigstreamy HEVC All Sub Preserve" ;;
    craigstreamy-hevc-all-sub-audio-conform) printf '%s' "Craigstreamy HEVC All Sub Audio Conform" ;;
    craigstreamy-hevc-smart-eng-sub-aggressive-vmaf) printf '%s' "Craigstreamy HEVC Smart Eng Sub Aggressive VMAF" ;;
    craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf) printf '%s' "Craigstreamy HEVC Smart Eng Sub Audio Conform Aggressive VMAF" ;;
    craigstreamy-hevc-smart-eng-sub-audio-conform) printf '%s' "Craigstreamy HEVC Smart Eng Sub Audio Conform" ;;
    craigstreamy-hevc-smart-eng-sub-subtitle-convert) printf '%s' "Craigstreamy HEVC Smart Eng Sub Subtitle Convert" ;;
    craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform) printf '%s' "Craigstreamy HEVC Smart Eng Sub Subtitle Convert Audio Conform" ;;
    craigstreamy-hevc-selected-english-subtitle-preserve) printf '%s' "Craigstreamy HEVC Selected English Subtitle Preserve" ;;
    *) printf '%s' "$1" ;;
  esac
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

  behavior="$(awk '
    /^# Behavior:/ {in_block=1; next}
    in_block && /^# Optional env:/ {exit}
    in_block && /^#/ {
      line=$0
      sub(/^# ?/, "", line)
      sub(/^- /, "", line)
      if (line != "") print line
      next
    }
    in_block {exit}
  ' "$action_file" || true)"

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

doc_command_display() {
  local command="$1"
  local token
  local marker

  token="${command%% *}"
  if [ "$token" = "profile_guardrail_skip.sh" ]; then
    marker="${command##* }"
    case "$marker" in
      *requires_1920x1080_to_3840x2160_input)
        printf '%s' "profile_guardrail_skip.sh (requires 1920x1080 to 3840x2160 input)"
        ;;
      *requires_sdr_bt709_and_1280x720_to_1920x1080_input)
        printf '%s' "profile_guardrail_skip.sh (requires SDR bt709 and 1280x720 to 1920x1080 input)"
        ;;
      *requires_320x240_to_1279x719_input)
        printf '%s' "profile_guardrail_skip.sh (requires 320x240 to 1279x719 input)"
        ;;
      *)
        printf '%s' "profile_guardrail_skip.sh (profile guardrail skip)"
        ;;
    esac
    return 0
  fi

  if [ "$token" = "ffmpeg" ]; then
    printf '%s' "ffmpeg (inline command)"
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
    transcode_hevc_1080_all_sub_preserve_profile.sh|\
    transcode_hevc_1080_all_sub_audio_conform_profile.sh|\
    transcode_hevc_4k_all_sub_preserve_profile.sh|\
    transcode_hevc_4k_all_sub_audio_conform_profile.sh|\
    transcode_hevc_1080_main_subtitle_preserve_profile.sh|\
    transcode_hevc_1080_smart_eng_sub_aggressive_vmaf_profile.sh|\
    transcode_hevc_1080_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh|\
    transcode_hevc_4k_main_subtitle_preserve_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_aggressive_vmaf_profile.sh|\
    transcode_hevc_1080_smart_eng_sub_subtitle_convert_profile.sh|\
    transcode_hevc_1080_smart_eng_sub_subtitle_convert_audio_conform_profile.sh|\
    transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_subtitle_convert_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_subtitle_convert_audio_conform_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh|\
    transcode_hevc_legacy_all_sub_preserve_profile.sh|\
    transcode_hevc_legacy_all_sub_audio_conform_profile.sh|\
    transcode_hevc_legacy_smart_eng_sub_aggressive_vmaf_profile.sh|\
    transcode_hevc_legacy_smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh|\
    transcode_hevc_legacy_smart_eng_sub_audio_conform_profile.sh|\
    transcode_hevc_legacy_smart_eng_sub_subtitle_convert_audio_conform_profile.sh|\
    transcode_hevc_legacy_smart_eng_sub_subtitle_convert_profile.sh|\
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
    transcode_hevc_4k_main_subtitle_preserve_profile.sh|\
    transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh)
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
  local doc_slug
  local doc_label
  local doc_title
  local legacy_profile_note
  local prefix
  local scenarios
  local commands
  local criteria_line
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
  local subtitle_gate_label
  local yes_stage_label
  local yes_output_label
  local no_stage_label
  local no_output_label
  local typical_input_containers
  local output_intent
  local criteria_codec_display
  local criteria_bits_display
  local criteria_color_display
  local is_craigstreamy_subtitle_pack
  local is_craigstreamy_audio_conform_pack
  local is_craigstreamy_all_sub_pack
  local is_craigstreamy_subtitle_convert_pack
  local is_craigstreamy_aggressive_vmaf_pack
  local dep_ffmpeg
  local dep_ffprobe
  local dep_mkvmerge
  local dep_mkvextract
  local dep_dovi_tool
  local covered_by_profile_actions
  local covered_by_device_conformance
  local covered_by_dv_optional

  slug="$(to_slug "$profile")"
  doc_slug="$(doc_slug_for_profile "$pack" "$profile")"
  doc_label="$(doc_label_for_profile "$pack" "$profile")"
  doc_title="$(doc_title_for_profile "$pack" "$profile")"
  legacy_profile_note=""
  if [[ "$profile" == netflixy_preserve_audio_main_subtitle_intent_* ]]; then
    legacy_profile_note="This profile still uses a legacy internal config id for compatibility."
  fi
  prefix="$(to_upper_prefix "$profile")"
  profile_doc="$PROFILE_OUT_DIR/${doc_slug}.md"

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
  subtitle_gate_label="smart_eng_sub subtitle selected?"
  yes_stage_label="Encode HEVC + preserve audio + preserve smart_eng_sub subtitle"
  yes_output_label="Emit MKV output"
  no_stage_label="Encode HEVC + preserve audio"
  no_output_label="Finalize fragmented MP4 + init/moov at start"
  if printf '%s' "$first_command" | grep -Eq "main_subtitle_preserve_profile.sh|all_sub_preserve_profile.sh|all_sub_audio_conform_profile.sh|smart_eng_sub_audio_conform_profile.sh|smart_eng_sub_audio_conform_aggressive_vmaf_profile.sh|smart_eng_sub_aggressive_vmaf_profile.sh|smart_eng_sub_subtitle_convert_profile.sh|smart_eng_sub_subtitle_convert_audio_conform_profile.sh"; then
    mermaid_variant="subtitle_intent"
  fi

  subtitle_gate_label="smart_eng_sub subtitle selected?"
  yes_stage_label="Encode HEVC + preserve audio + preserve smart_eng_sub subtitle"
  yes_output_label="Emit MKV output"
  no_stage_label="Encode HEVC + preserve audio"
  no_output_label="Finalize fragmented MP4 + init/moov at start"

  typical_input_containers="mkv, mp4, mov, mxf (anything ffmpeg can demux)"
  output_intent="profile-specific output written by selected scenario command"
  if printf '%s' "$first_command" | grep -q "subtitle_convert_audio_conform_profile.sh"; then
    output_intent="conditional: stream-ready MP4 with converted text subtitles when the smart_eng_sub policy selects text subtitles and audio remains MP4-safe; fallback to MKV subtitle preservation when preserved audio safety forces MKV"
  elif printf '%s' "$first_command" | grep -q "subtitle_convert_profile.sh"; then
    output_intent="conditional: stream-ready MP4 with converted text subtitles when the smart_eng_sub policy selects text subtitles; fallback behavior is controlled by VFO_SUBTITLE_CONVERT_BITMAP_POLICY"
  elif printf '%s' "$first_command" | grep -q "all_sub_audio_conform_profile.sh"; then
    output_intent="conditional: MKV when subtitle streams are present for carry-over or when preserved-audio safety forces MKV, otherwise stream-ready MP4 with conformed delivery audio"
  elif printf '%s' "$first_command" | grep -q "all_sub_preserve_profile.sh"; then
    output_intent="conditional: MKV when subtitle streams are present for carry-over, otherwise stream-ready MP4"
  elif [ "$mermaid_variant" = "subtitle_intent" ]; then
    output_intent="conditional: MKV when the smart_eng_sub + preserve policy selects a subtitle, otherwise stream-ready MP4 (fragmented + init/moov at start by default)"
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

  is_craigstreamy_subtitle_pack="0"
  is_craigstreamy_audio_conform_pack="0"
  is_craigstreamy_all_sub_pack="0"
  is_craigstreamy_subtitle_convert_pack="0"
  is_craigstreamy_aggressive_vmaf_pack="0"
  if [ "$mermaid_variant" = "subtitle_intent" ]; then
    case "$pack" in
      craigstreamy-hevc-selected-english-subtitle-preserve|\
      craigstreamy-hevc-smart-eng-sub-aggressive-vmaf|\
      craigstreamy-hevc-smart-eng-sub-audio-conform|\
      craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf|\
      craigstreamy-hevc-all-sub-preserve|\
      craigstreamy-hevc-all-sub-audio-conform|\
      craigstreamy-hevc-smart-eng-sub-subtitle-convert|\
      craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform)
        is_craigstreamy_subtitle_pack="1"
        ;;
    esac
    if [ "$pack" = "craigstreamy-hevc-smart-eng-sub-audio-conform" ] \
      || [ "$pack" = "craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf" ] \
      || [ "$pack" = "craigstreamy-hevc-all-sub-audio-conform" ] \
      || [ "$pack" = "craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform" ]; then
      is_craigstreamy_audio_conform_pack="1"
    fi
    if [ "$pack" = "craigstreamy-hevc-all-sub-preserve" ] \
      || [ "$pack" = "craigstreamy-hevc-all-sub-audio-conform" ]; then
      is_craigstreamy_all_sub_pack="1"
    fi
    if [ "$pack" = "craigstreamy-hevc-smart-eng-sub-subtitle-convert" ] \
      || [ "$pack" = "craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform" ]; then
      is_craigstreamy_subtitle_convert_pack="1"
    fi
    if [ "$pack" = "craigstreamy-hevc-smart-eng-sub-aggressive-vmaf" ] \
      || [ "$pack" = "craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf" ]; then
      is_craigstreamy_aggressive_vmaf_pack="1"
    fi
  fi

  if [ "$is_craigstreamy_all_sub_pack" = "1" ]; then
    subtitle_gate_label="Any subtitle streams present?"
    if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
      yes_stage_label="Encode HEVC + conform audio if needed + preserve all subtitle streams"
      no_stage_label="Encode HEVC + conform audio if needed"
    else
      yes_stage_label="Encode HEVC + preserve audio + preserve all subtitle streams"
      no_stage_label="Encode HEVC + preserve audio"
    fi
    yes_output_label="Emit MKV output carrying all subtitle streams"
    no_output_label="Finalize fragmented MP4 + init/moov at start"
  elif [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
    subtitle_gate_label="Selected subtitle is text-convertible and MP4 remains viable?"
    if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
      yes_stage_label="Encode HEVC + conform audio if needed + convert selected subtitle to mov_text"
      no_stage_label="Encode HEVC + conform audio if needed + preserve selected subtitle by explicit fallback"
    else
      yes_stage_label="Encode HEVC + preserve audio + convert selected subtitle to mov_text"
      no_stage_label="Encode HEVC + preserve audio + apply bitmap/fallback subtitle policy"
      no_stage_label="Encode HEVC + preserve audio + apply bitmap or fallback subtitle policy"
    fi
    yes_output_label="Emit MP4 output with converted subtitle text"
    no_output_label="Emit explicit fallback output (usually MKV preserve or fail)"
  elif [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
    yes_stage_label="Encode HEVC + conform audio if needed + preserve smart_eng_sub subtitle"
    no_stage_label="Encode HEVC + conform audio if needed"
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
    printf '# %s\n\n' "$doc_title"
    printf 'Generated from stock preset pack `%s`.\n\n' "$pack"
    if [ -n "$legacy_profile_note" ]; then
      printf '%s\n\n' "$legacy_profile_note"
    fi
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

    if [ "$is_craigstreamy_subtitle_pack" = "1" ]; then
      printf '## Intent\n\n'
      if [ -n "$legacy_profile_note" ]; then
        printf 'Compatibility note: this profile belongs to the canonical `craigstreamy` pack family even though the current config profile id is legacy-shaped.\n\n'
      fi
      if [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
        printf 'This profile converts candidates into streaming-friendly HEVC outputs while keeping the `smart_eng_sub` subtitle selection heuristic and converting selected text subtitles into delivery-friendly text form when the final container remains MP4-friendly.\n\n'
      elif [ "$is_craigstreamy_all_sub_pack" = "1" ]; then
        printf 'This profile converts candidates into streaming-friendly HEVC outputs while preserving all subtitle streams for carry-over oriented workflows.\n\n'
      elif [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf 'This profile converts candidates into streaming-friendly HEVC outputs while preserving the `smart_eng_sub + preserve` subtitle policy and conforming DTS-family or PCM-family audio when needed.\n\n'
      else
        printf 'This profile converts candidates into streaming-friendly HEVC outputs while preserving the `smart_eng_sub + preserve` subtitle policy where feasible.\n\n'
      fi
      printf '## What It Optimizes For\n\n'
      printf -- '- practical bitrate efficiency with a consistent HEVC target\n'
      if [ "$is_craigstreamy_aggressive_vmaf_pack" = "1" ]; then
        printf -- '- bounded aggressive-VMAF retries on the video encode path only\n'
      fi
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf -- '- preserve AAC and Dolby-family audio streams when already acceptable\n'
        printf -- '- conform DTS-family or PCM-family audio into open-source Dolby-aligned delivery codecs when needed\n'
      else
        printf -- '- preserve all audio streams by default when packaging permits\n'
      fi
      if [ "$is_craigstreamy_all_sub_pack" = "1" ]; then
        printf -- '- subtitle policy: `all_sub_preserve` + `preserve`\n'
        printf -- '- conditional container selection: MKV when subtitle streams are present, stream-ready MP4 otherwise\n'
      elif [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
        printf -- '- subtitle policy: `smart_eng_sub` + `subtitle_convert`\n'
        printf -- '- conditional container selection: stream-ready MP4 when selected subtitles are text-convertible, explicit fallback for bitmap subtitles\n'
      else
        printf -- '- subtitle policy: `smart_eng_sub` + `preserve`\n'
        printf -- '- conditional container selection: MKV when the `smart_eng_sub + preserve` policy selects a subtitle, fragmented MP4 otherwise\n'
      fi
      if printf '%s' "$first_command" | grep -q "legacy_main_subtitle_preserve_profile.sh"; then
        printf -- '- for legacy sub-HD intake: optional deinterlace and persistent black-bar auto-crop\n'
      elif printf '%s' "$first_command" | grep -q "legacy_smart_eng_sub_audio_conform_profile.sh"; then
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
        command_display=""
        if [ "$i" -lt "${#commands[@]}" ]; then
          command="${commands[$i]}"
          command_display="$(doc_command_display "$command")"
        fi
        printf '| `%s` | `%s` |\n' "$scenario" "$command_display"
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
      printf '```mermaid\n'
      printf 'flowchart LR\n'
      printf '  classDef gate fill:#fff7ed,stroke:#f59e0b,color:#7c2d12,stroke-width:1.5px;\n'
      printf '  classDef stage fill:#e0f2fe,stroke:#0284c7,color:#0c4a6e,stroke-width:1.2px;\n'
      printf '  classDef output fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:1.2px;\n'
      printf '  classDef skip fill:#f3f4f6,stroke:#6b7280,color:#1f2937,stroke-width:1.2px;\n'
      printf '\n'
      printf '  A[Input candidate: mkv / mp4 / mov / mxf]:::stage --> B[Probe codec bits color resolution]:::stage\n'
      printf '  B --> C{Matches profile criteria envelope?}:::gate\n'
      printf '  C -->|No| Z[Handled by other profile or guardrail skipped]:::skip\n'
      printf '  C -->|Yes| D{Evaluate scenarios in order}:::gate\n'
      printf '  D --> E[Execute subtitle-policy action]:::stage\n'
      printf '  E --> P[Optional lane-specific pre-processing]:::stage\n'
      printf '  P --> F{%s}:::gate\n' "$(mermaid_text "$subtitle_gate_label")"
      printf '  F -->|Yes| G[%s]:::stage\n' "$(mermaid_text "$yes_stage_label")"
      printf '  G --> H[%s]:::output\n' "$(mermaid_text "$yes_output_label")"
      printf '  F -->|No| I[%s]:::stage\n' "$(mermaid_text "$no_stage_label")"
      printf '  I --> J[%s]:::stage\n' "$(mermaid_text "$no_output_label")"
      printf '  J --> K[Emit final profile artifact]:::output\n'
      printf '```\n'
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

    if [ "$is_craigstreamy_subtitle_pack" = "1" ]; then
      printf '\n## What This Profile Does Not Do\n\n'
      printf -- '- It does not normalize frame rate; source cadence/timebase is preserved by default.\n'
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf -- '- It does not invent Atmos or proprietary immersive metadata.\n'
        printf -- '- It does not transcode already-acceptable AAC or Dolby-family audio just to make everything uniform.\n'
        printf -- '- It does not apply a broad audio bitrate-lowering strategy yet.\n'
      else
        printf -- '- It does not transcode audio for target-device compatibility by default.\n'
        printf -- '- It does not guarantee every input audio codec is valid for every selected output container.\n'
      fi
      if [ "$is_craigstreamy_aggressive_vmaf_pack" = "1" ]; then
        printf -- '- It does not use VMAF to change audio behavior; aggressive mode is video-only.\n'
      fi
      printf -- '- It does not semantically understand subtitle meaning; subtitle selection is metadata and flag driven.\n'
      printf -- '- It does not OCR or convert bitmap subtitles to text subtitles.\n'
      printf -- '- It does not generate ABR ladders (HLS/DASH); output is a single-file artifact.\n'
      printf -- '- It does not certify playback on every device model; profile criteria are compatibility-oriented guardrails.\n'
      printf -- '- It does not enforce PSNR/SSIM/VMAF thresholds unless quality checks are explicitly enabled and configured.\n'
      printf -- '- It does not invent missing HDR/DV essence; metadata repair is heuristic and can be disabled.\n'
      printf -- '- It depends on source integrity and toolchain support for DV/HDR retention; strict mode may fail instead of silently downgrading.\n'
      printf '\n## High-Level Assessments\n\n'
      printf '| Label | Assessment |\n'
      printf '| --- | --- |\n'
      printf '| Dynamic range | `HDR/DV aware` on 4K, SDR-gated on 1080p, broad intake on legacy sub-HD |\n'
      printf '| Resolution | `4K / 1080p / legacy sub-HD` lane family |\n'
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Audio codecs | `AAC + Dolby preserve`, `DTS/PCM conform` |\n'
      else
        printf '| Audio codecs | `preserved by default` |\n'
      fi
      printf '| Video codecs | `HEVC transcode target` |\n'
      printf '| Interlacing | `legacy lane only; optional deinterlace` |\n'
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Volume normalisation | `applied when DTS/PCM-family audio is transcoded` |\n'
      else
        printf '| Volume normalisation | `not applied by default` |\n'
      fi
      printf '| Crop | `legacy lane auto-crop enabled` |\n'
      if [ "$is_craigstreamy_aggressive_vmaf_pack" = "1" ]; then
        printf '| Lowered video bitrate | `yes; bounded aggressive-VMAF retry policy` |\n'
      else
        printf '| Lowered video bitrate | `yes` |\n'
      fi
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Lowered audio bitrate | `not as a general policy; only codec-target defaults for DTS/PCM conform` |\n'
        printf '| Audio transcoded | `DTS/PCM-family only` |\n'
      else
        printf '| Lowered audio bitrate | `no by default` |\n'
        printf '| Audio transcoded | `no by default` |\n'
      fi
      printf '| Video transcoded | `yes` |\n'
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Audio switched | `DTS/PCM -> AAC / E-AC-3 / AC-3 when needed` |\n'
      else
        printf '| Audio switched | `no; stream copy preferred` |\n'
      fi
      if [ "$is_craigstreamy_all_sub_pack" = "1" ]; then
        printf '| Subtitle retained | `all_sub_preserve + preserve` |\n'
        printf '| Subtitle transformed | `no; preserve mode only` |\n'
      elif [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
        printf '| Subtitle retained | `smart_eng_sub + subtitle_convert` |\n'
        printf '| Subtitle transformed | `selected text subtitles -> mov_text` |\n'
      else
        printf '| Subtitle retained | `smart_eng_sub + preserve` |\n'
        printf '| Subtitle transformed | `no; preserve mode only` |\n'
      fi
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Container changed | `yes when subtitle or preserved-audio safety requires MKV, otherwise fragmented MP4 with faststart fallback for E-AC-3` |\n'
      elif [ "$is_craigstreamy_all_sub_pack" = "1" ]; then
        printf '| Container changed | `yes when subtitle carry-over requires MKV, otherwise fragmented MP4` |\n'
      elif [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
        printf '| Container changed | `yes when subtitle conversion succeeds into MP4 text form; bitmap conversion follows explicit fallback policy` |\n'
      else
        printf '| Container changed | `yes when the smart_eng_sub + preserve policy requires MKV, otherwise fragmented MP4` |\n'
      fi
      printf '| Container targets | `MKV` / `fragmented MP4` |\n'
      if [ "$is_craigstreamy_audio_conform_pack" = "1" ]; then
        printf '| Bitrate targets | `practical video efficiency; audio preserve-first` |\n'
        printf '| Audio bitrate targets | `codec-target defaults only when DTS/PCM-family audio is conformed` |\n'
        printf '| Overall bitrate targets | `reduce video bitrate while preserving viewing intent and sane audio delivery` |\n'
      elif [ "$is_craigstreamy_aggressive_vmaf_pack" = "1" ]; then
        printf '| Bitrate targets | `video-first bounded search for lower bitrate at the configured VMAF floor` |\n'
        printf '| Audio bitrate targets | `copy/preserve unless a future audio profile says otherwise` |\n'
        printf '| Overall bitrate targets | `reduce video bitrate more aggressively while keeping audio untouched` |\n'
      elif [ "$is_craigstreamy_subtitle_convert_pack" = "1" ]; then
        printf '| Bitrate targets | `practical efficiency with delivery-friendly subtitle text outputs` |\n'
        printf '| Audio bitrate targets | `copy/preserve unless a future audio profile says otherwise` |\n'
        printf '| Overall bitrate targets | `reduce video bitrate while maintaining viewing intent and subtitle compatibility` |\n'
      else
        printf '| Bitrate targets | `practical efficiency over source bit-for-bit preservation` |\n'
        printf '| Audio bitrate targets | `copy/preserve unless a future audio profile says otherwise` |\n'
        printf '| Overall bitrate targets | `reduce video bitrate while maintaining viewing intent` |\n'
      fi
      printf '| Error | `guardrail skip, missing toolchain, strict DV/HDR mismatch, or unknown error placeholder` |\n'
    fi

    printf '\n## Source\n\n'
    printf -- '- Preset file: `%s`\n' "${file_path#$REPO_ROOT/}"
    printf -- '- Generated by: `infra/scripts/generate-profile-docs.sh`\n'
  } > "$profile_doc"

  printf '%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n' \
    "$profile" \
    "$doc_label" \
    "$pack" \
    "$criteria_codec_display" \
    "$criteria_bits_display" \
    "$criteria_color_display" \
    "${min_w:-0}x${min_h:-0}" \
    "${max_w:-any}x${max_h:-any}" \
    "$doc_slug" \
    "$scenario_count"
}

declare -a MATRIX_ROWS=()
declare -a PROFILE_LINKS=()
last_pack=""

while IFS= read -r preset_file; do
  [ -f "$preset_file" ] || continue
  pack="$(basename "$(dirname "$preset_file")")"

  if [ "$pack" != "$last_pack" ]; then
    if [ "${#PROFILE_LINKS[@]}" -gt 0 ]; then
      PROFILE_LINKS+=("")
    fi
    PROFILE_LINKS+=("## $(pack_label_for_docs "$pack")")
    last_pack="$pack"
  fi

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

    doc_slug="$(doc_slug_for_profile "$pack" "$profile")"
    doc_label="$(doc_label_for_profile "$pack" "$profile")"
    PROFILE_LINKS+=("- [$doc_label](generated/${doc_slug}.md)")
  done
done < <(find "$PRESETS_DIR" -type f -name 'vfo_config.preset.conf' | sort)

{
  printf '# Stock Profile Info Sheets\n\n'
  printf 'These pages are generated from stock preset files and linked action scripts.\n\n'
  printf 'Compatibility note: the canonical pack `craigstreamy_hevc_selected_english_subtitle_preserve` still maps to legacy internal profile ids for compatibility, but the generated docs use canonical `craigstreamy` labels.\n\n'
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
    IFS='|' read -r profile doc_label pack codec bits color min_res max_res slug scenario_count <<< "$row"
    printf '| [%s](profiles/generated/%s.md) | `%s` | `%s` | `%s` | `%s` | `%s` | `%s` | `%s` |\n' \
      "$doc_label" "$slug" "$pack" "$codec" "$bits" "$color" "$min_res" "$max_res" "$scenario_count"
  done

  printf '\n## Notes\n\n'
  printf -- '- This matrix reflects stock presets, not every custom profile a user may define.\n'
  printf -- '- `craigstreamy_hevc_selected_english_subtitle_preserve` remains the preserve-audio subtitle-intent pack.\n'
  printf -- '- Its selected-English generated docs are labeled with canonical `craigstreamy` names even though the current config profile ids remain legacy-shaped for compatibility.\n'
  printf -- '- `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf` adds video-only aggressive-VMAF behavior on top of the preserve-audio subtitle-intent family.\n'
  printf -- '- `craigstreamy_hevc_smart_eng_sub_audio_conform` adds DTS/PCM delivery-conform behavior on top of the subtitle-intent family.\n'
  printf -- '- `craigstreamy_hevc_all_sub_audio_conform` and `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform` complete the first audio-conform subtitle matrix.\n'
} > "$MATRIX_DOC"

echo "Generated profile docs under: ${PROFILE_OUT_DIR#$REPO_ROOT/}"
echo "Updated index: ${PROFILES_INDEX#$REPO_ROOT/}"
echo "Updated matrix: ${MATRIX_DOC#$REPO_ROOT/}"
