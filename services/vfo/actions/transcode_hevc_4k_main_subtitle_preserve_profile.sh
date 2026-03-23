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
# - If a main subtitle is selected, output container is MKV for reliable subtitle preservation.
# - If no main subtitle is selected, output container is stream-ready MP4:
#   fragmented MP4 with init/moov at the start.
#
# Optional env:
#   VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT=1   # include default english subtitle when no forced track exists
#   VFO_ENCODER_MODE=auto|hw|cpu
#   VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart
#     default: fmp4_faststart

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
  local input_mp4="$1"
  local output_mp4="$2"
  local movflags

  movflags="$(resolve_mp4_movflags "$MP4_STREAM_MODE")"
  echo "Finalizing MP4 stream packaging mode=${MP4_STREAM_MODE} movflags=${movflags}"
  ffmpeg -hide_banner -nostdin -y \
    -i "$input_mp4" \
    -map 0 \
    -c copy \
    -movflags "$movflags" \
    -max_muxing_queue_size 4096 \
    "$output_mp4"
}

VIDEO_ARGS=()
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=(
    -c:v hevc_videotoolbox
    -pix_fmt p010le
    -b:v "${AVG_K}k"
    -maxrate "${MAXRATE_K}k"
    -bufsize "${BUFSIZE_K}k"
  )
else
  VIDEO_ARGS=(
    -c:v libx265
    -preset "$CPU_PRESET"
    -crf "$CRF_4K"
    -x265-params "vbv-maxrate=${MAXRATE_K}:vbv-bufsize=${BUFSIZE_K}:aq-mode=3"
    -pix_fmt yuv420p10le
  )
fi

MAIN_SUB_POS=""
if MAIN_SUB_POS="$(resolve_main_subtitle_position)"; then
  OUTPUT_PATH="${OUTPUT%.*}.mkv"
  echo "MAIN subtitle detected (s:${MAIN_SUB_POS}); preserving in MKV output: $OUTPUT_PATH"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? -map "0:s:${MAIN_SUB_POS}" \
    "${VIDEO_ARGS[@]}" \
    -c:a copy -c:s copy \
    -max_muxing_queue_size 4096 \
    "$OUTPUT_PATH"
else
  MP4_WORK_OUTPUT="${OUTPUT%.*}.packaging_work.mp4"
  echo "No main subtitle detected; generating MP4 encode work file: $MP4_WORK_OUTPUT"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? \
    "${VIDEO_ARGS[@]}" \
    -c:a copy \
    -max_muxing_queue_size 4096 \
    "$MP4_WORK_OUTPUT"

  finalize_streamable_mp4 "$MP4_WORK_OUTPUT" "$OUTPUT"
  rm -f "$MP4_WORK_OUTPUT"
fi
