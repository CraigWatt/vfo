#!/usr/bin/env bash
set -euo pipefail

# Shared subtitle-policy helpers for craigstreamy subtitle-aware profile actions.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=live_encode_tools.sh
. "$SCRIPT_DIR/live_encode_tools.sh"

SUBTITLE_POLICY_SELECTION_SCOPE="${VFO_SUBTITLE_SELECTION_SCOPE:-smart_eng_sub}"
SUBTITLE_POLICY_MODE="${VFO_SUBTITLE_MODE:-preserve}"
SUBTITLE_POLICY_CONVERT_BITMAP_POLICY="${VFO_SUBTITLE_CONVERT_BITMAP_POLICY:-fail}"

SUBTITLE_POLICY_SELECTED_POSITIONS=""
SUBTITLE_POLICY_SELECTED_COUNT=0
SUBTITLE_POLICY_PRIMARY_POSITION=""
SUBTITLE_POLICY_PRIMARY_CODEC=""
SUBTITLE_POLICY_HAS_BITMAP=0
SUBTITLE_POLICY_ALL_TEXT=0
SUBTITLE_POLICY_OUTPUT_CONTAINER="mp4"
SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="none"
SUBTITLE_POLICY_ERROR=""

subtitle_policy_lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

subtitle_policy_is_english_language() {
  case "$(subtitle_policy_lower_text "$1")" in
    en|eng|english|en-us|en-gb) return 0 ;;
    *) return 1 ;;
  esac
}

subtitle_policy_is_unknown_language() {
  case "$(subtitle_policy_lower_text "$1")" in
    ''|und|unk|unknown|n/a|none) return 0 ;;
    *) return 1 ;;
  esac
}

subtitle_policy_is_forced_like_title() {
  case "$(subtitle_policy_lower_text "$1")" in
    *forced*) return 0 ;;
    *) return 1 ;;
  esac
}

subtitle_policy_is_bitmap_codec() {
  case "$(subtitle_policy_lower_text "$1")" in
    hdmv_pgs_subtitle|dvd_subtitle|dvb_subtitle|xsub|pgs|vobsub)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

subtitle_policy_is_text_codec() {
  case "$(subtitle_policy_lower_text "$1")" in
    subrip|srt|ass|ssa|webvtt|mov_text|text|ttml|eia_608|eia_708)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

subtitle_policy_stream_count() {
  ffprobe -v error \
    -select_streams s \
    -show_entries stream=index \
    -of csv=p=0 "$1" 2>/dev/null | wc -l | tr -d ' '
}

subtitle_policy_stream_value() {
  local input="$1"
  local stream_pos="$2"
  local ffprobe_entry="$3"

  ffprobe -v error \
    -select_streams "s:${stream_pos}" \
    -show_entries "$ffprobe_entry" \
    -of default=nw=1:nk=1 \
    "$input" 2>/dev/null | head -n 1 | tr -d '\r'
}

subtitle_policy_stream_codec() {
  subtitle_policy_stream_value "$1" "$2" "stream=codec_name"
}

subtitle_policy_count_positions() {
  local positions="$1"
  local count="0"

  if [ -z "$positions" ]; then
    printf '0\n'
    return 0
  fi

  count="$(printf '%s\n' "$positions" | sed '/^$/d' | wc -l | tr -d ' ')"
  case "$count" in
    ''|*[!0-9]*)
      count="0"
      ;;
  esac

  printf '%s\n' "$count"
}

subtitle_policy_first_position() {
  local positions="$1"

  if [ -z "$positions" ]; then
    return 1
  fi

  printf '%s\n' "$positions" | sed '/^$/d' | head -n 1
}

subtitle_policy_has_any_subtitle() {
  local count=""

  count="$(subtitle_policy_stream_count "$1")"
  case "$count" in
    ''|*[!0-9]*)
      count="0"
      ;;
  esac

  [ "$count" -gt 0 ]
}

subtitle_policy_has_any_bitmap_subtitle() {
  local input="$1"
  local count=""
  local pos=0
  local codec=""

  count="$(subtitle_policy_stream_count "$input")"
  case "$count" in
    ''|*[!0-9]*)
      count="0"
      ;;
  esac

  while [ "$pos" -lt "$count" ]; do
    codec="$(subtitle_policy_stream_codec "$input" "$pos" || true)"
    if [ -n "$codec" ] && subtitle_policy_is_bitmap_codec "$codec"; then
      return 0
    fi
    pos=$((pos + 1))
  done

  return 1
}

subtitle_policy_select_all_subs() {
  local input="$1"
  local sub_count=""
  local pos=0

  sub_count="$(subtitle_policy_stream_count "$input")"
  case "$sub_count" in
    ''|*[!0-9]*)
      sub_count="0"
      ;;
  esac
  if [ "$sub_count" -eq 0 ]; then
    return 1
  fi

  while [ "$pos" -lt "$sub_count" ]; do
    printf '%s\n' "$pos"
    pos=$((pos + 1))
  done

  return 0
}

subtitle_policy_select_smart_eng_sub() {
  local input="$1"
  local include_default="${2:-0}"
  local sub_count=""
  local pos=0
  local language=""
  local title=""
  local forced="0"
  local default_disposition="0"
  local forced_like="0"
  local forced_unknown=""
  local default_english=""

  sub_count="$(subtitle_policy_stream_count "$input")"
  case "$sub_count" in
    ''|*[!0-9]*)
      sub_count="0"
      ;;
  esac
  if [ "$sub_count" -eq 0 ]; then
    return 1
  fi

  while [ "$pos" -lt "$sub_count" ]; do
    language="$(subtitle_policy_stream_value "$input" "$pos" "stream_tags=language")"
    title="$(subtitle_policy_stream_value "$input" "$pos" "stream_tags=title")"
    forced="$(subtitle_policy_stream_value "$input" "$pos" "stream_disposition=forced")"
    default_disposition="$(subtitle_policy_stream_value "$input" "$pos" "stream_disposition=default")"

    [ -n "$forced" ] || forced="0"
    [ -n "$default_disposition" ] || default_disposition="0"
    forced_like="0"
    if [ "$forced" = "1" ] || subtitle_policy_is_forced_like_title "$title"; then
      forced_like="1"
    fi

    if [ "$forced_like" = "1" ] && subtitle_policy_is_english_language "$language"; then
      printf '%s\n' "$pos"
      return 0
    fi

    if [ "$forced_like" = "1" ] && subtitle_policy_is_unknown_language "$language" && [ -z "$forced_unknown" ]; then
      forced_unknown="$pos"
    fi

    if [ "$include_default" = "1" ] \
      && [ "$default_disposition" = "1" ] \
      && subtitle_policy_is_english_language "$language" \
      && [ -z "$default_english" ]; then
      default_english="$pos"
    fi

    pos=$((pos + 1))
  done

  if [ -n "$forced_unknown" ]; then
    printf '%s\n' "$forced_unknown"
    return 0
  fi

  if [ -n "$default_english" ]; then
    printf '%s\n' "$default_english"
    return 0
  fi

  return 1
}

subtitle_policy_select_positions() {
  local input="$1"
  local include_default="${2:-0}"
  local scope=""

  scope="$(subtitle_policy_lower_text "$SUBTITLE_POLICY_SELECTION_SCOPE")"

  case "$scope" in
    smart_eng_sub)
      subtitle_policy_select_smart_eng_sub "$input" "$include_default"
      ;;
    all_sub_preserve)
      subtitle_policy_select_all_subs "$input"
      ;;
    *)
      echo "Unsupported VFO_SUBTITLE_SELECTION_SCOPE='${SUBTITLE_POLICY_SELECTION_SCOPE}'" >&2
      return 2
      ;;
  esac
}

subtitle_policy_analyze_positions() {
  local input="$1"
  local positions="$2"
  local pos=""
  local codec=""
  local has_bitmap=0
  local all_text=1
  local primary_codec=""

  while IFS= read -r pos; do
    [ -n "$pos" ] || continue
    codec="$(subtitle_policy_stream_codec "$input" "$pos" || true)"
    [ -n "$codec" ] || codec="unknown"
    if [ -z "$primary_codec" ]; then
      primary_codec="$codec"
    fi
    if subtitle_policy_is_bitmap_codec "$codec"; then
      has_bitmap=1
      all_text=0
    elif ! subtitle_policy_is_text_codec "$codec"; then
      all_text=0
    fi
  done <<< "$positions"

  SUBTITLE_POLICY_HAS_BITMAP="$has_bitmap"
  SUBTITLE_POLICY_ALL_TEXT="$all_text"
  SUBTITLE_POLICY_PRIMARY_CODEC="$primary_codec"
}

subtitle_policy_resolve_plan() {
  local input="$1"
  local include_default="${2:-0}"
  local selection_rc=0
  local mode=""
  local first_pos=""

  SUBTITLE_POLICY_SELECTED_POSITIONS=""
  SUBTITLE_POLICY_SELECTED_COUNT=0
  SUBTITLE_POLICY_PRIMARY_POSITION=""
  SUBTITLE_POLICY_PRIMARY_CODEC=""
  SUBTITLE_POLICY_HAS_BITMAP=0
  SUBTITLE_POLICY_ALL_TEXT=0
  SUBTITLE_POLICY_OUTPUT_CONTAINER="mp4"
  SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="none"
  SUBTITLE_POLICY_ERROR=""

  if ! SUBTITLE_POLICY_SELECTED_POSITIONS="$(subtitle_policy_select_positions "$input" "$include_default" 2>/dev/null)"; then
    selection_rc=$?
    if [ "$selection_rc" -eq 2 ]; then
      SUBTITLE_POLICY_ERROR="unsupported subtitle selection scope: ${SUBTITLE_POLICY_SELECTION_SCOPE}"
      return 1
    fi
    SUBTITLE_POLICY_SELECTED_POSITIONS=""
  fi

  SUBTITLE_POLICY_SELECTED_COUNT="$(subtitle_policy_count_positions "$SUBTITLE_POLICY_SELECTED_POSITIONS")"
  if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
    first_pos="$(subtitle_policy_first_position "$SUBTITLE_POLICY_SELECTED_POSITIONS" || true)"
    SUBTITLE_POLICY_PRIMARY_POSITION="$first_pos"
    subtitle_policy_analyze_positions "$input" "$SUBTITLE_POLICY_SELECTED_POSITIONS"
  fi

  mode="$(subtitle_policy_lower_text "$SUBTITLE_POLICY_MODE")"
  case "$mode" in
    preserve)
      if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
        SUBTITLE_POLICY_OUTPUT_CONTAINER="mkv"
        SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="copy"
      fi
      ;;
    subtitle_convert)
      if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -eq 0 ]; then
        SUBTITLE_POLICY_OUTPUT_CONTAINER="mp4"
        SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="none"
      elif [ "$SUBTITLE_POLICY_HAS_BITMAP" = "1" ]; then
        case "$(subtitle_policy_lower_text "$SUBTITLE_POLICY_CONVERT_BITMAP_POLICY")" in
          preserve_mkv)
            SUBTITLE_POLICY_OUTPUT_CONTAINER="mkv"
            SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="copy"
            ;;
          *)
            SUBTITLE_POLICY_ERROR="subtitle_convert cannot convert bitmap subtitles without OCR; selected codec=${SUBTITLE_POLICY_PRIMARY_CODEC:-unknown}"
            return 1
            ;;
        esac
      else
        SUBTITLE_POLICY_OUTPUT_CONTAINER="mp4"
        SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC="mov_text"
      fi
      ;;
    *)
      SUBTITLE_POLICY_ERROR="unsupported subtitle mode: ${SUBTITLE_POLICY_MODE}"
      return 1
      ;;
  esac

  return 0
}
