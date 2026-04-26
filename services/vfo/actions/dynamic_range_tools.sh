#!/usr/bin/env bash

# Shared helpers for dynamic-range metadata detection, repair defaults,
# and output validation/reporting across profile action scripts.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=live_encode_tools.sh
. "$SCRIPT_DIR/live_encode_tools.sh"

dr_lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

dr_probe_stream_value() {
  local file_path="$1"
  local key="$2"
  ffprobe -v error \
    -select_streams v:0 \
    -show_entries "stream=${key}" \
    -of default=nw=1:nk=1 \
    "$file_path" 2>/dev/null | head -n 1 | tr -d '\r'
}

dr_has_dovi_side_data() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$1" 2>/dev/null | grep -qi "dovi"
}

dr_get_dovi_profile() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$1" 2>/dev/null | awk -F= '/^dv_profile=/{print $2; exit}' | tr -d ' \t\r\n'
}

dr_get_dovi_profile_from_tool() {
  if ! command -v dovi_tool >/dev/null 2>&1; then
    return 1
  fi
  dovi_tool info -i "$1" 2>/dev/null \
    | awk '
      /[Pp]rofile/ {
        for (i = 1; i <= NF; i++) {
          if ($i ~ /^[0-9]+(\.[0-9]+)?$/) { print $i; exit }
        }
      }
    ' | head -n 1 | tr -d ' \t\r\n'
}

dr_is_missing_tag() {
  case "$(dr_lower_text "$1")" in
    ''|unknown|unspecified|undef|undefined|reserved|none|na|n/a|null)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

dr_is_hdr_transfer() {
  case "$(dr_lower_text "$1")" in
    smpte2084|arib-std-b67|bt2020-10|bt2020-12)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

dr_is_hdr_primaries() {
  case "$(dr_lower_text "$1")" in
    bt2020)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

dr_is_hdr_colorspace() {
  case "$(dr_lower_text "$1")" in
    bt2020nc|bt2020c)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

dr_append_note() {
  local note="$1"
  if [ -z "${DR_REPAIR_NOTES:-}" ]; then
    DR_REPAIR_NOTES="$note"
  else
    DR_REPAIR_NOTES="${DR_REPAIR_NOTES};${note}"
  fi
}

dr_collect_source_state() {
  local input_path="$1"
  local fallback_dv_profile=""
  DR_INPUT_PATH="$input_path"
  DR_SRC_COLOR_SPACE="$(dr_lower_text "$(dr_probe_stream_value "$input_path" color_space)")"
  DR_SRC_COLOR_TRC="$(dr_lower_text "$(dr_probe_stream_value "$input_path" color_transfer)")"
  DR_SRC_COLOR_PRIMARIES="$(dr_lower_text "$(dr_probe_stream_value "$input_path" color_primaries)")"
  DR_SRC_HAS_DV="0"
  DR_SRC_DV_PROFILE=""
  DR_SRC_DV_DETECTION_SOURCE="none"
  DR_SOURCE_CLASS="sdr"
  DR_REPAIR_NOTES=""

  if dr_has_dovi_side_data "$input_path"; then
    DR_SRC_HAS_DV="1"
    DR_SRC_DV_PROFILE="$(dr_get_dovi_profile "$input_path")"
    DR_SRC_DV_DETECTION_SOURCE="ffprobe_side_data"
    DR_SOURCE_CLASS="dv"
    return 0
  fi

  fallback_dv_profile="$(dr_get_dovi_profile_from_tool "$input_path" || true)"
  if [ -n "$fallback_dv_profile" ]; then
    DR_SRC_HAS_DV="1"
    DR_SRC_DV_PROFILE="$fallback_dv_profile"
    DR_SRC_DV_DETECTION_SOURCE="dovi_tool_fallback"
    DR_SOURCE_CLASS="dv"
    dr_append_note "dv_detected_via_dovi_tool_without_ffprobe_side_data"
    return 0
  fi

  if dr_is_hdr_transfer "$DR_SRC_COLOR_TRC" || dr_is_hdr_primaries "$DR_SRC_COLOR_PRIMARIES" || dr_is_hdr_colorspace "$DR_SRC_COLOR_SPACE"; then
    DR_SOURCE_CLASS="hdr"
    return 0
  fi

  DR_SOURCE_CLASS="sdr"
}

dr_compute_target_tags() {
  local policy="$1" # preserve|sdr1080

  DR_TARGET_COLOR_SPACE=""
  DR_TARGET_COLOR_TRC=""
  DR_TARGET_COLOR_PRIMARIES=""

  if [ "$policy" = "sdr1080" ]; then
    DR_TARGET_COLOR_SPACE="bt709"
    DR_TARGET_COLOR_TRC="bt709"
    DR_TARGET_COLOR_PRIMARIES="bt709"
    return 0
  fi

  if [ "$DR_SOURCE_CLASS" = "dv" ] || [ "$DR_SOURCE_CLASS" = "hdr" ]; then
    if dr_is_missing_tag "$DR_SRC_COLOR_SPACE"; then
      DR_TARGET_COLOR_SPACE="bt2020nc"
      dr_append_note "set_color_space=bt2020nc"
    else
      DR_TARGET_COLOR_SPACE="$DR_SRC_COLOR_SPACE"
    fi
    if dr_is_missing_tag "$DR_SRC_COLOR_TRC"; then
      DR_TARGET_COLOR_TRC="smpte2084"
      dr_append_note "set_color_transfer=smpte2084"
    else
      DR_TARGET_COLOR_TRC="$DR_SRC_COLOR_TRC"
    fi
    if dr_is_missing_tag "$DR_SRC_COLOR_PRIMARIES"; then
      DR_TARGET_COLOR_PRIMARIES="bt2020"
      dr_append_note "set_color_primaries=bt2020"
    else
      DR_TARGET_COLOR_PRIMARIES="$DR_SRC_COLOR_PRIMARIES"
    fi
    return 0
  fi

  if dr_is_missing_tag "$DR_SRC_COLOR_SPACE"; then
    DR_TARGET_COLOR_SPACE="bt709"
    dr_append_note "set_color_space=bt709"
  else
    DR_TARGET_COLOR_SPACE="$DR_SRC_COLOR_SPACE"
  fi
  if dr_is_missing_tag "$DR_SRC_COLOR_TRC"; then
    DR_TARGET_COLOR_TRC="bt709"
    dr_append_note "set_color_transfer=bt709"
  else
    DR_TARGET_COLOR_TRC="$DR_SRC_COLOR_TRC"
  fi
  if dr_is_missing_tag "$DR_SRC_COLOR_PRIMARIES"; then
    DR_TARGET_COLOR_PRIMARIES="bt709"
    dr_append_note "set_color_primaries=bt709"
  else
    DR_TARGET_COLOR_PRIMARIES="$DR_SRC_COLOR_PRIMARIES"
  fi
}

dr_collect_output_state() {
  local output_path="$1"
  DR_OUTPUT_PATH="$output_path"
  DR_OUT_COLOR_SPACE="$(dr_lower_text "$(dr_probe_stream_value "$output_path" color_space)")"
  DR_OUT_COLOR_TRC="$(dr_lower_text "$(dr_probe_stream_value "$output_path" color_transfer)")"
  DR_OUT_COLOR_PRIMARIES="$(dr_lower_text "$(dr_probe_stream_value "$output_path" color_primaries)")"
  DR_OUT_HAS_DV="0"
  DR_OUT_DV_PROFILE=""
  DR_OUTPUT_CLASS="sdr"

  if dr_has_dovi_side_data "$output_path"; then
    DR_OUT_HAS_DV="1"
    DR_OUT_DV_PROFILE="$(dr_get_dovi_profile "$output_path")"
    DR_OUTPUT_CLASS="dv"
    return 0
  fi

  if dr_is_hdr_transfer "$DR_OUT_COLOR_TRC" || dr_is_hdr_primaries "$DR_OUT_COLOR_PRIMARIES" || dr_is_hdr_colorspace "$DR_OUT_COLOR_SPACE"; then
    DR_OUTPUT_CLASS="hdr"
    return 0
  fi

  DR_OUTPUT_CLASS="sdr"
}

dr_validate_output_against_source() {
  local strict="$1" # 0|1
  local policy="$2" # preserve|sdr1080

  if [ "$policy" = "sdr1080" ]; then
    if [ "$DR_OUTPUT_CLASS" != "sdr" ]; then
      if [ "$strict" = "1" ]; then
        echo "Dynamic-range validation failed: 1080 SDR policy requires SDR output, got ${DR_OUTPUT_CLASS}"
        return 1
      fi
      echo "Dynamic-range warning: 1080 SDR policy expected SDR output, got ${DR_OUTPUT_CLASS}"
    fi
    return 0
  fi

  if [ "$DR_SOURCE_CLASS" = "dv" ] && [ "$DR_OUTPUT_CLASS" != "dv" ]; then
    echo "Dynamic-range validation failed: source is DV but output class is ${DR_OUTPUT_CLASS}"
    [ "$strict" = "1" ] && return 1
  fi

  if [ "$DR_SOURCE_CLASS" = "hdr" ] && [ "$DR_OUTPUT_CLASS" = "sdr" ]; then
    echo "Dynamic-range validation failed: source is HDR but output class is SDR"
    [ "$strict" = "1" ] && return 1
  fi

  if [ "$DR_SOURCE_CLASS" = "sdr" ] && [ "$DR_OUTPUT_CLASS" != "sdr" ]; then
    echo "Dynamic-range warning: source is SDR but output class is ${DR_OUTPUT_CLASS}"
  fi

  return 0
}

dr_write_report() {
  local output_path="$1"
  local action_name="$2"
  local policy="$3"
  local strict="$4"
  local report_path="${output_path%.*}.dynamic_range_report.txt"

  {
    printf 'action=%s\n' "$action_name"
    printf 'policy=%s\n' "$policy"
    printf 'strict=%s\n' "$strict"
    printf 'source_class=%s\n' "${DR_SOURCE_CLASS:-unknown}"
    printf 'source_dv=%s\n' "${DR_SRC_HAS_DV:-0}"
    printf 'source_dv_profile=%s\n' "${DR_SRC_DV_PROFILE:-}"
    printf 'source_dv_detection=%s\n' "${DR_SRC_DV_DETECTION_SOURCE:-none}"
    printf 'source_color_space=%s\n' "${DR_SRC_COLOR_SPACE:-}"
    printf 'source_color_transfer=%s\n' "${DR_SRC_COLOR_TRC:-}"
    printf 'source_color_primaries=%s\n' "${DR_SRC_COLOR_PRIMARIES:-}"
    printf 'target_color_space=%s\n' "${DR_TARGET_COLOR_SPACE:-}"
    printf 'target_color_transfer=%s\n' "${DR_TARGET_COLOR_TRC:-}"
    printf 'target_color_primaries=%s\n' "${DR_TARGET_COLOR_PRIMARIES:-}"
    printf 'metadata_repair_notes=%s\n' "${DR_REPAIR_NOTES:-none}"
    printf 'output_class=%s\n' "${DR_OUTPUT_CLASS:-unknown}"
    printf 'output_dv=%s\n' "${DR_OUT_HAS_DV:-0}"
    printf 'output_dv_profile=%s\n' "${DR_OUT_DV_PROFILE:-}"
    printf 'output_color_space=%s\n' "${DR_OUT_COLOR_SPACE:-}"
    printf 'output_color_transfer=%s\n' "${DR_OUT_COLOR_TRC:-}"
    printf 'output_color_primaries=%s\n' "${DR_OUT_COLOR_PRIMARIES:-}"
  } > "$report_path"
  echo "Dynamic-range report written: $report_path"
}
