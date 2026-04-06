#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 1080p HEVC profile action with policy-driven subtitle handling.
#
# Contract (called from vfo config):
#   transcode_hevc_1080_main_subtitle_preserve_profile.sh <input_file> <output_file>
#
# Behavior:
# - Always preserves audio streams with stream copy.
# - Default subtitle behavior is `smart_eng_sub + preserve`.
# - Policy can be overridden by wrapper packs via:
#   VFO_SUBTITLE_SELECTION_SCOPE=smart_eng_sub|all_sub_preserve
#   VFO_SUBTITLE_MODE=preserve|subtitle_convert
#   VFO_SUBTITLE_CONVERT_BITMAP_POLICY=fail|preserve_mkv
# - Enforces SDR-oriented 1080 policy metadata on output (`bt709` signaling).
# - `preserve` emits MKV whenever the resolved subtitle policy selects streams.
# - `subtitle_convert` keeps MP4 when selected subtitles are text-convertible and
#   converts them to `mov_text`; bitmap subtitles fail by default unless
#   `VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv`.
# - If no subtitle is selected, output container is stream-ready MP4.
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
# shellcheck source=subtitle_policy_tools.sh
. "$SCRIPT_DIR/subtitle_policy_tools.sh"

ENCODER_MODE="${VFO_ENCODER_MODE:-auto}" # auto|hw|cpu
INCLUDE_DEFAULT_MAIN_SUB="${VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT:-0}"
MP4_STREAM_MODE="${VFO_MP4_STREAM_MODE:-fmp4_faststart}"
PROBE_SIZE="${PROBE_SIZE:-200M}"
ANALYZE_DUR="${ANALYZE_DUR:-200M}"
AVG_K_1080="${AVG_K_1080:-8000}"
MAXRATE_K_1080="${MAXRATE_K_1080:-12000}"
BUFSIZE_K_1080="${BUFSIZE_K_1080:-24000}"
CRF_1080="${CRF_1080:-19}"
CPU_PRESET="${CPU_PRESET:-slow}"
VFO_DYNAMIC_METADATA_REPAIR="${VFO_DYNAMIC_METADATA_REPAIR:-1}"
VFO_DYNAMIC_RANGE_STRICT="${VFO_DYNAMIC_RANGE_STRICT:-1}"
VFO_DYNAMIC_RANGE_REPORT="${VFO_DYNAMIC_RANGE_REPORT:-1}"

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
  local subtitle_source_media="$2"
  local subtitle_positions="$3"
  local subtitle_codec="$4"
  local output_mp4="$5"
  local movflags
  local next_input_index=1
  local pos=""
  local -a cmd=()
  local -a maps=()
  local -a subtitle_args=()

  movflags="$(resolve_mp4_movflags "$MP4_STREAM_MODE")"
  echo "Finalizing MP4 stream packaging mode=${MP4_STREAM_MODE} movflags=${movflags}"
  cmd=(-hide_banner -nostdin -y -i "$input_mp4")
  maps=(-map 0:v -map 0:a?)

  if [ -n "$subtitle_positions" ] && [ "$subtitle_codec" != "none" ]; then
    cmd+=(-i "$subtitle_source_media")
    while IFS= read -r pos; do
      [ -n "$pos" ] || continue
      maps+=(-map "${next_input_index}:s:${pos}")
    done <<< "$subtitle_positions"
    subtitle_args=(-c:s "$subtitle_codec")
    next_input_index=$((next_input_index + 1))
  else
    subtitle_args=(-sn)
  fi

  ffmpeg "${cmd[@]}" \
    "${maps[@]}" \
    -dn \
    -c copy \
    "${subtitle_args[@]}" \
    -write_tmcd 0 \
    -movflags "$movflags" \
    -max_muxing_queue_size 4096 \
    "$output_mp4"
}

dr_collect_source_state "$INPUT"
dr_compute_target_tags "sdr1080"
COLOR_ARGS=()
if [ "$VFO_DYNAMIC_METADATA_REPAIR" = "1" ]; then
  COLOR_ARGS=(
    -colorspace "$DR_TARGET_COLOR_SPACE"
    -color_trc "$DR_TARGET_COLOR_TRC"
    -color_primaries "$DR_TARGET_COLOR_PRIMARIES"
  )
fi
if [ "$DR_SOURCE_CLASS" != "sdr" ]; then
  echo "WARN: 1080 SDR lane received ${DR_SOURCE_CLASS} input; profile policy enforces SDR signaling"
fi

VIDEO_ARGS=()
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=(
    -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
    -c:v hevc_videotoolbox
    -pix_fmt p010le
    -b:v "${AVG_K_1080}k"
    -maxrate "${MAXRATE_K_1080}k"
    -bufsize "${BUFSIZE_K_1080}k"
    "${COLOR_ARGS[@]}"
  )
else
  VIDEO_ARGS=(
    -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
    -c:v libx265
    -preset "$CPU_PRESET"
    -crf "$CRF_1080"
    -x265-params "vbv-maxrate=${MAXRATE_K_1080}:vbv-bufsize=${BUFSIZE_K_1080}:aq-mode=3"
    -pix_fmt yuv420p10le
    "${COLOR_ARGS[@]}"
  )
fi

if ! subtitle_policy_resolve_plan "$INPUT" "$INCLUDE_DEFAULT_MAIN_SUB"; then
  echo "Subtitle policy resolution failed: ${SUBTITLE_POLICY_ERROR:-unknown subtitle policy error}"
  exit 1
fi

if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
  echo "Subtitle policy selected ${SUBTITLE_POLICY_SELECTED_COUNT} stream(s); scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE} codec=${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}"
else
  echo "Subtitle policy selected no streams; scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE}"
fi

ACTUAL_OUTPUT="$OUTPUT"
if [ "$SUBTITLE_POLICY_OUTPUT_CONTAINER" = "mkv" ]; then
  local_subtitle_pos=""
  mkv_subtitle_maps=()
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  echo "Subtitle policy requires MKV output: $ACTUAL_OUTPUT"
  while IFS= read -r local_subtitle_pos; do
    [ -n "$local_subtitle_pos" ] || continue
    mkv_subtitle_maps+=(-map "0:s:${local_subtitle_pos}")
  done <<< "$SUBTITLE_POLICY_SELECTED_POSITIONS"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? \
    "${mkv_subtitle_maps[@]}" \
    "${VIDEO_ARGS[@]}" \
    -c:a copy \
    -c:s "${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}" \
    -max_muxing_queue_size 4096 \
    "$ACTUAL_OUTPUT"
else
  MP4_WORK_OUTPUT="${OUTPUT%.*}.packaging_work.mp4"
  echo "Generating MP4 encode work file: $MP4_WORK_OUTPUT"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? \
    "${VIDEO_ARGS[@]}" \
    -c:a copy \
    -max_muxing_queue_size 4096 \
    "$MP4_WORK_OUTPUT"

  finalize_streamable_mp4 \
    "$MP4_WORK_OUTPUT" \
    "$INPUT" \
    "$SUBTITLE_POLICY_SELECTED_POSITIONS" \
    "$SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC" \
    "$OUTPUT"
  rm -f "$MP4_WORK_OUTPUT"
fi

dr_collect_output_state "$ACTUAL_OUTPUT"
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "sdr1080"; then
  exit 1
fi
if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "sdr1080" "$VFO_DYNAMIC_RANGE_STRICT"
fi
