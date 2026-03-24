#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 4K HEVC profile action with "main subtitle intent" handling.
#
# Contract (called from vfo config):
#   transcode_hevc_4k_main_subtitle_preserve_profile.sh <input_file> <output_file>
#
# Behavior:
# - Always preserves audio streams with stream copy.
# - Selects one "main subtitle" when it appears director-intent oriented:
#   priority: forced english -> forced untagged/unknown -> optional default english.
#   non-english forced tracks are intentionally skipped.
# - Preserves dynamic-range signaling for HDR/DV workflows by default:
#   applies metadata-repair defaults when source tags are incomplete.
# - If source signals Dolby Vision side data, attempts DV RPU retention/injection.
# - If a main subtitle is selected, output container is MKV for reliable subtitle preservation.
# - If no main subtitle is selected, output container is stream-ready MP4:
#   fragmented MP4 with init/moov at the start.
#
# Optional env:
#   VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1   # include default english subtitle when no forced track exists
#   VFO_ENCODER_MODE=auto|hw|cpu
#   VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart
#     default: fmp4_faststart
#   VFO_DYNAMIC_METADATA_REPAIR=1|0
#     default: 1
#   VFO_DYNAMIC_RANGE_STRICT=1|0
#     default: 1
#   VFO_DYNAMIC_RANGE_REPORT=1|0
#     default: 1
#   VFO_DV_REQUIRE_DOVI=1|0
#     default: 1
#   VFO_DV_CONVERT_P7_TO_81=1|0
#     default: 1
#   VFO_DV_P7_TO_81_MODE=2|5
#     default: 2
#   VFO_DV_REQUIRE_P7_TO_81=1|0
#     default: 1

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <input_file> <output_file>"
  exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [ ! -f "$INPUT" ]; then
  echo "Input file not found: $INPUT"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=dynamic_range_tools.sh
. "$SCRIPT_DIR/dynamic_range_tools.sh"

ENCODER_MODE="${VFO_ENCODER_MODE:-auto}" # auto|hw|cpu
INCLUDE_DEFAULT_MAIN_SUB="${VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT:-0}"
MP4_STREAM_MODE="${VFO_MP4_STREAM_MODE:-fmp4_faststart}"
PROBE_SIZE="${PROBE_SIZE:-200M}"
ANALYZE_DUR="${ANALYZE_DUR:-200M}"
AVG_K="${AVG_K:-15000}"
MAXRATE_K="${MAXRATE_K:-20000}"
BUFSIZE_K="${BUFSIZE_K:-40000}"
CRF_4K="${CRF_4K:-18}"
CPU_PRESET="${CPU_PRESET:-slow}"
MP4_TIMESCALE="${MP4_TIMESCALE:-24000}"
VFO_DYNAMIC_METADATA_REPAIR="${VFO_DYNAMIC_METADATA_REPAIR:-1}"
VFO_DYNAMIC_RANGE_STRICT="${VFO_DYNAMIC_RANGE_STRICT:-1}"
VFO_DYNAMIC_RANGE_REPORT="${VFO_DYNAMIC_RANGE_REPORT:-1}"
VFO_DV_REQUIRE_DOVI="${VFO_DV_REQUIRE_DOVI:-1}"
VFO_DV_CONVERT_P7_TO_81="${VFO_DV_CONVERT_P7_TO_81:-1}"
VFO_DV_P7_TO_81_MODE="${VFO_DV_P7_TO_81_MODE:-2}"
VFO_DV_REQUIRE_P7_TO_81="${VFO_DV_REQUIRE_P7_TO_81:-1}"

lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

is_english_language() {
  case "$(lower_text "$1")" in
    en|eng|english|en-us|en-gb) return 0 ;;
    *) return 1 ;;
  esac
}

is_unknown_language() {
  case "$(lower_text "$1")" in
    ''|und|unk|unknown|n/a|none) return 0 ;;
    *) return 1 ;;
  esac
}

is_forced_like_title() {
  case "$(lower_text "$1")" in
    *forced*) return 0 ;;
    *) return 1 ;;
  esac
}

get_sub_stream_value() {
  local sub_pos="$1"
  local ffprobe_entry="$2"
  ffprobe -v error \
    -select_streams "s:${sub_pos}" \
    -show_entries "$ffprobe_entry" \
    -of default=nw=1:nk=1 \
    "$INPUT" 2>/dev/null | head -n 1 | tr -d '\r'
}

resolve_main_subtitle_position() {
  local sub_count
  local pos
  local language=""
  local title=""
  local forced="0"
  local default_disposition="0"
  local forced_like="0"
  local forced_unknown=""
  local default_english=""

  sub_count="$(ffprobe -v error -select_streams s -show_entries stream=index -of csv=p=0 "$INPUT" 2>/dev/null | wc -l | tr -d ' ')"
  if [ -z "$sub_count" ] || [ "$sub_count" -eq 0 ]; then
    return 1
  fi

  pos=0
  while [ "$pos" -lt "$sub_count" ]; do
    language="$(get_sub_stream_value "$pos" "stream_tags=language")"
    title="$(get_sub_stream_value "$pos" "stream_tags=title")"
    forced="$(get_sub_stream_value "$pos" "stream_disposition=forced")"
    default_disposition="$(get_sub_stream_value "$pos" "stream_disposition=default")"

    [ -n "$forced" ] || forced="0"
    [ -n "$default_disposition" ] || default_disposition="0"
    forced_like="0"
    if [ "$forced" = "1" ] || is_forced_like_title "$title"; then
      forced_like="1"
    fi

    if [ "$forced_like" = "1" ] && is_english_language "$language"; then
      printf '%s\n' "$pos"
      return 0
    fi

    if [ "$forced_like" = "1" ] && is_unknown_language "$language" && [ -z "$forced_unknown" ]; then
      forced_unknown="$pos"
    fi

    if [ "$INCLUDE_DEFAULT_MAIN_SUB" = "1" ] \
      && [ "$default_disposition" = "1" ] \
      && is_english_language "$language" \
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

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "hevc_videotoolbox"
}

resolve_mp4_movflags() {
  case "$1" in
    fmp4_faststart)
      # init/moov-first + fragmented moof chunks for stream-oriented single-file MP4
      printf '%s' "+faststart+frag_keyframe+empty_moov+default_base_moof"
      ;;
    fmp4)
      printf '%s' "+frag_keyframe+empty_moov+default_base_moof"
      ;;
    faststart)
      printf '%s' "+faststart"
      ;;
    *)
      echo "Invalid VFO_MP4_STREAM_MODE value: $1"
      echo "Expected one of: fmp4_faststart, fmp4, faststart"
      exit 1
      ;;
  esac
}

finalize_streamable_mp4() {
  local input_video_mp4="$1"
  local input_source_media="$2"
  local output_mp4="$3"
  local movflags

  movflags="$(resolve_mp4_movflags "$MP4_STREAM_MODE")"
  echo "Finalizing MP4 stream packaging mode=${MP4_STREAM_MODE} movflags=${movflags}"
  ffmpeg -hide_banner -nostdin -y \
    -i "$input_video_mp4" -i "$input_source_media" \
    -map 0:v:0 -map 1:a? \
    -sn -dn \
    -c copy \
    -write_tmcd 0 \
    -movflags "$movflags" \
    -max_muxing_queue_size 4096 \
    "$output_mp4"
}

get_fps() {
  local src="$1"
  local fps
  fps="$(ffprobe -v error -select_streams v:0 \
    -show_entries stream=avg_frame_rate -of default=nk=1:nw=1 "$src" 2>/dev/null || true)"
  fps="$(printf '%s' "$fps" | tr -d ' \t\r\n')"
  [ -n "$fps" ] || fps="24000/1001"
  printf '%s' "$fps"
}

dr_collect_source_state "$INPUT"
dr_compute_target_tags "preserve"
COLOR_ARGS=()
if [ "$VFO_DYNAMIC_METADATA_REPAIR" = "1" ]; then
  COLOR_ARGS=(
    -colorspace "$DR_TARGET_COLOR_SPACE"
    -color_trc "$DR_TARGET_COLOR_TRC"
    -color_primaries "$DR_TARGET_COLOR_PRIMARIES"
  )
fi
if [ -n "${DR_REPAIR_NOTES:-}" ]; then
  echo "Dynamic-range metadata repair hints: ${DR_REPAIR_NOTES}"
fi

HW_VIDEO_ARGS=(
  -c:v hevc_videotoolbox
  -pix_fmt p010le
  -b:v "${AVG_K}k"
  -maxrate "${MAXRATE_K}k"
  -bufsize "${BUFSIZE_K}k"
  "${COLOR_ARGS[@]}"
)

CPU_VIDEO_ARGS=(
  -c:v libx265
  -preset "$CPU_PRESET"
  -crf "$CRF_4K"
  -x265-params "vbv-maxrate=${MAXRATE_K}:vbv-bufsize=${BUFSIZE_K}:aq-mode=3"
  -pix_fmt yuv420p10le
  "${COLOR_ARGS[@]}"
)

VIDEO_ARGS=()
using_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=("${HW_VIDEO_ARGS[@]}")
  using_hw=1
else
  VIDEO_ARGS=("${CPU_VIDEO_ARGS[@]}")
fi

workdir="$(mktemp -d "${TMPDIR:-/tmp}/vfo-main-sub-4k-XXXXXX")"
trap 'rm -rf "$workdir"' EXIT

enc_mp4="$workdir/enc_video.mp4"
enc_hevc="$workdir/enc.hevc"
src_hevc="$workdir/src.hevc"
rpu_bin="$workdir/rpu.bin"
dv_hevc="$workdir/dv.hevc"
dv_mp4="$workdir/dv_video.mp4"
fps="$(get_fps "$INPUT")"

MAIN_SUB_POS=""
if MAIN_SUB_POS="$(resolve_main_subtitle_position)"; then
  echo "MAIN subtitle detected (s:${MAIN_SUB_POS}); final container will be MKV"
else
  echo "No main subtitle detected; final container will be MP4"
fi

if ! ffmpeg -hide_banner -nostdin -y \
  -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
  -i "$INPUT" \
  -map 0:v:0 \
  "${VIDEO_ARGS[@]}" \
  -movflags +faststart \
  -video_track_timescale "$MP4_TIMESCALE" \
  "$enc_mp4" >/dev/null 2>&1; then
  if [ "$ENCODER_MODE" = "auto" ] && [ "$using_hw" -eq 1 ]; then
    echo "VideoToolbox encode failed; retrying with CPU fallback" >&2
    ffmpeg -hide_banner -nostdin -y \
      -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
      -i "$INPUT" \
      -map 0:v:0 \
      "${CPU_VIDEO_ARGS[@]}" \
      -movflags +faststart \
      -video_track_timescale "$MP4_TIMESCALE" \
      "$enc_mp4" >/dev/null 2>&1
  else
    echo "Failed to encode source to HEVC for main-subtitle workflow" >&2
    exit 1
  fi
fi

new_video="$enc_mp4"
source_is_dv=0
source_dv_profile=""
p7_to_81_requested=0
p7_to_81_applied=0

if [ "$DR_SOURCE_CLASS" = "dv" ]; then
  source_is_dv=1
  source_dv_profile="${DR_SRC_DV_PROFILE:-}"
fi

if [ "$source_is_dv" -eq 1 ]; then
  if ! command -v dovi_tool >/dev/null 2>&1; then
    if [ "$VFO_DV_REQUIRE_DOVI" = "1" ]; then
      echo "Source contains Dolby Vision but dovi_tool is not available (VFO_DV_REQUIRE_DOVI=1)"
      exit 1
    fi
    echo "WARN: source contains Dolby Vision but dovi_tool is missing; continuing without DV retention"
  else
    if [ "$VFO_DV_P7_TO_81_MODE" != "2" ] && [ "$VFO_DV_P7_TO_81_MODE" != "5" ]; then
      echo "Invalid VFO_DV_P7_TO_81_MODE='${VFO_DV_P7_TO_81_MODE}' (allowed: 2 or 5)"
      exit 1
    fi

    ffmpeg -hide_banner -nostdin -y \
      -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
      -i "$INPUT" \
      -map 0:v:0 -c:v copy -bsf:v hevc_mp4toannexb -f hevc \
      "$src_hevc" >/dev/null 2>&1 || true

    if [ "$source_dv_profile" = "7" ] && [ "$VFO_DV_CONVERT_P7_TO_81" = "1" ]; then
      p7_to_81_requested=1
      if [ -s "$src_hevc" ] && dovi_tool -m "$VFO_DV_P7_TO_81_MODE" extract-rpu -i "$src_hevc" -o "$rpu_bin" >/dev/null 2>&1; then
        p7_to_81_applied=1
      fi
    elif [ -s "$src_hevc" ] && dovi_tool extract-rpu -i "$src_hevc" -o "$rpu_bin" >/dev/null 2>&1; then
      p7_to_81_applied=1
    fi

    if [ "$p7_to_81_applied" -eq 1 ]; then
      ffmpeg -hide_banner -nostdin -y \
        -i "$enc_mp4" \
        -map 0:v:0 -c:v copy -bsf:v hevc_mp4toannexb -f hevc \
        "$enc_hevc" >/dev/null 2>&1 || true

      if [ -s "$enc_hevc" ] && dovi_tool inject-rpu -i "$enc_hevc" --rpu-in "$rpu_bin" -o "$dv_hevc" >/dev/null 2>&1; then
        if ffmpeg -hide_banner -nostdin -y \
          -fflags +genpts -r "$fps" -f hevc -i "$dv_hevc" \
          -c:v copy -movflags +faststart \
          -video_track_timescale "$MP4_TIMESCALE" \
          "$dv_mp4" >/dev/null 2>&1 \
          && ffprobe -v error "$dv_mp4" >/dev/null 2>&1; then
          if [ "$p7_to_81_requested" -eq 1 ]; then
            output_dv_profile="$(dr_get_dovi_profile "$dv_mp4")"
            if [ "$output_dv_profile" = "8" ]; then
              new_video="$dv_mp4"
            else
              p7_to_81_applied=0
            fi
          else
            new_video="$dv_mp4"
          fi
        fi
      fi
    fi
  fi
fi

if [ "$p7_to_81_requested" -eq 1 ] && [ "$VFO_DV_REQUIRE_P7_TO_81" = "1" ] && [ "$new_video" != "$dv_mp4" ]; then
  echo "Source contains Dolby Vision profile 7 but profile 8.1 conversion failed (VFO_DV_REQUIRE_P7_TO_81=1)"
  exit 1
fi

if [ "$source_is_dv" -eq 1 ] && [ "$VFO_DV_REQUIRE_DOVI" = "1" ] && [ "$new_video" != "$dv_mp4" ]; then
  echo "Source contains Dolby Vision but DV retention failed (VFO_DV_REQUIRE_DOVI=1)"
  exit 1
fi

ACTUAL_OUTPUT="$OUTPUT"
if [ -n "$MAIN_SUB_POS" ]; then
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  ffmpeg -hide_banner -nostdin -y \
    -i "$new_video" -i "$INPUT" \
    -map 0:v:0 -map 1:a? -map "1:s:${MAIN_SUB_POS}" \
    -c:v copy -c:a copy -c:s copy \
    -max_muxing_queue_size 4096 \
    "$ACTUAL_OUTPUT"
else
  finalize_streamable_mp4 "$new_video" "$INPUT" "$ACTUAL_OUTPUT"
fi

dr_collect_output_state "$ACTUAL_OUTPUT"
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "preserve"; then
  exit 1
fi

if [ "$source_is_dv" -eq 1 ] && [ "$VFO_DV_REQUIRE_DOVI" = "1" ] && [ "$DR_OUTPUT_CLASS" != "dv" ]; then
  echo "Dynamic-range validation failed: source is DV but output is ${DR_OUTPUT_CLASS}"
  exit 1
fi

if [ "$p7_to_81_requested" -eq 1 ] && [ "$VFO_DV_REQUIRE_P7_TO_81" = "1" ] && [ "$DR_OUT_DV_PROFILE" != "8" ]; then
  echo "Dynamic-range validation failed: source DV profile 7 was not converted to profile 8.x"
  exit 1
fi

if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "preserve" "$VFO_DYNAMIC_RANGE_STRICT"
fi
