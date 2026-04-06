#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware legacy/sub-HD HEVC profile action with policy-driven subtitle handling.
#
# Contract (called from vfo config):
#   transcode_hevc_legacy_main_subtitle_preserve_profile.sh <input_file> <output_file>
#
# Behavior:
# - Always preserves audio streams with stream copy.
# - Default subtitle behavior is `smart_eng_sub + preserve`.
# - Policy can be overridden by wrapper packs via:
#   VFO_SUBTITLE_SELECTION_SCOPE=smart_eng_sub|all_sub_preserve
#   VFO_SUBTITLE_MODE=preserve|subtitle_convert
#   VFO_SUBTITLE_CONVERT_BITMAP_POLICY=fail|preserve_mkv
# - Preserves dynamic-range signaling for HDR workflows by default:
#   applies metadata-repair defaults when source tags are incomplete.
# - Optionally applies deinterlace when input is interlaced (default: auto).
# - Optionally applies stable black-bar auto-crop for persistent bars.
# - If selected subtitle is bitmap-based, crop is disabled for subtitle placement safety.
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
#   VFO_LEGACY_DEINTERLACE=auto|always|off
#     default: auto
#   VFO_LEGACY_AUTOCROP=1|0
#     default: 1
#   VFO_LEGACY_CROP_SAMPLE_SECONDS=3
#   VFO_LEGACY_CROP_DETECT_LIMIT=24
#   VFO_LEGACY_CROP_MIN_PIXELS=8
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
AVG_K_LEGACY="${AVG_K_LEGACY:-2500}"
MAXRATE_K_LEGACY="${MAXRATE_K_LEGACY:-4000}"
BUFSIZE_K_LEGACY="${BUFSIZE_K_LEGACY:-8000}"
CRF_LEGACY="${CRF_LEGACY:-22}"
CPU_PRESET="${CPU_PRESET:-slow}"
LEGACY_DEINTERLACE_MODE="${VFO_LEGACY_DEINTERLACE:-auto}"
LEGACY_AUTOCROP="${VFO_LEGACY_AUTOCROP:-1}"
LEGACY_CROP_SAMPLE_SECONDS="${VFO_LEGACY_CROP_SAMPLE_SECONDS:-3}"
LEGACY_CROP_DETECT_LIMIT="${VFO_LEGACY_CROP_DETECT_LIMIT:-24}"
LEGACY_CROP_MIN_PIXELS="${VFO_LEGACY_CROP_MIN_PIXELS:-8}"
VFO_DYNAMIC_METADATA_REPAIR="${VFO_DYNAMIC_METADATA_REPAIR:-1}"
VFO_DYNAMIC_RANGE_STRICT="${VFO_DYNAMIC_RANGE_STRICT:-1}"
VFO_DYNAMIC_RANGE_REPORT="${VFO_DYNAMIC_RANGE_REPORT:-1}"

lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "hevc_videotoolbox"
}

resolve_mp4_movflags() {
  case "$1" in
    fmp4_faststart)
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
      maps+=(-map "1:s:${pos}")
    done <<< "$subtitle_positions"
    subtitle_args=(-c:s "$subtitle_codec")
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

probe_video_stream_value() {
  local key="$1"
  ffprobe -v error \
    -select_streams v:0 \
    -show_entries "stream=${key}" \
    -of default=nw=1:nk=1 \
    "$INPUT" 2>/dev/null | head -n 1 | tr -d '\r'
}

detect_crop_at_offset() {
  local offset="$1"
  ffmpeg -hide_banner -nostdin -ss "$offset" -t "$LEGACY_CROP_SAMPLE_SECONDS" \
    -i "$INPUT" \
    -map 0:v:0 \
    -vf "cropdetect=limit=${LEGACY_CROP_DETECT_LIMIT}:round=2:reset=0" \
    -an -sn -dn -f null - 2>&1 \
    | grep -Eo 'crop=[0-9]+:[0-9]+:[0-9]+:[0-9]+' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 1 \
    | awk '{print $2}'
}

resolve_stable_crop_filter() {
  local autocrop_enabled
  local video_w
  local video_h
  local duration
  local offset_0="0"
  local offset_1="0"
  local offset_2="0"
  local candidate_0=""
  local candidate_1=""
  local candidate_2=""
  local winner_line=""
  local winner_count="0"
  local winner_crop=""
  local crop_values
  local crop_w
  local crop_h
  local crop_x
  local crop_y
  local removed_w
  local removed_h
  local min_removed

  autocrop_enabled="$(lower_text "$LEGACY_AUTOCROP")"
  case "$autocrop_enabled" in
    0|false|no|off) return 1 ;;
  esac

  video_w="$(probe_video_stream_value width)"
  video_h="$(probe_video_stream_value height)"
  if [ -z "$video_w" ] || [ -z "$video_h" ]; then
    return 1
  fi

  duration="$(ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 "$INPUT" 2>/dev/null | head -n 1 | tr -d '\r')"
  if ! printf '%s' "$duration" | awk 'BEGIN{ok=0} /^[0-9]+(\.[0-9]+)?$/ {ok=1} END{exit(ok?0:1)}'; then
    duration="0"
  fi

  offset_1="$(awk -v d="$duration" 'BEGIN{printf "%.3f", (d*0.33)}')"
  offset_2="$(awk -v d="$duration" 'BEGIN{printf "%.3f", (d*0.66)}')"

  candidate_0="$(detect_crop_at_offset "$offset_0" || true)"
  candidate_1="$(detect_crop_at_offset "$offset_1" || true)"
  candidate_2="$(detect_crop_at_offset "$offset_2" || true)"

  winner_line="$(
    printf '%s\n%s\n%s\n' "$candidate_0" "$candidate_1" "$candidate_2" \
      | sed '/^$/d' \
      | sort \
      | uniq -c \
      | sort -nr \
      | head -n 1
  )"
  if [ -z "$winner_line" ]; then
    return 1
  fi

  winner_count="$(printf '%s' "$winner_line" | awk '{print $1}')"
  winner_crop="$(printf '%s' "$winner_line" | awk '{print $2}')"
  if [ "$winner_count" -lt 1 ]; then
    return 1
  fi

  crop_values="${winner_crop#crop=}"
  IFS=':' read -r crop_w crop_h crop_x crop_y <<EOF
$crop_values
EOF
  [ -n "$crop_w" ] || return 1
  [ -n "$crop_h" ] || return 1
  [ -n "$crop_x" ] || return 1
  [ -n "$crop_y" ] || return 1

  removed_w=$((video_w - crop_w))
  removed_h=$((video_h - crop_h))
  min_removed=$((LEGACY_CROP_MIN_PIXELS * 2))
  if [ "$removed_w" -lt 0 ] || [ "$removed_h" -lt 0 ]; then
    return 1
  fi
  if [ "$removed_w" -lt "$min_removed" ] && [ "$removed_h" -lt "$min_removed" ]; then
    return 1
  fi

  printf '%s\n' "$winner_crop"
}

resolve_deinterlace_filter() {
  local mode
  local field_order
  mode="$(lower_text "$LEGACY_DEINTERLACE_MODE")"

  case "$mode" in
    off|0|false|no)
      return 1
      ;;
    always|1|true|yes)
      printf '%s\n' "bwdif=mode=send_frame:parity=auto:deint=all"
      return 0
      ;;
  esac

  field_order="$(lower_text "$(probe_video_stream_value field_order)")"
  case "$field_order" in
    tt|bb|tb|bt|interlaced)
      printf '%s\n' "bwdif=mode=send_frame:parity=auto:deint=interlaced"
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

join_filters() {
  local left="$1"
  local right="$2"
  if [ -z "$left" ]; then
    printf '%s\n' "$right"
  elif [ -z "$right" ]; then
    printf '%s\n' "$left"
  else
    printf '%s,%s\n' "$left" "$right"
  fi
}

if ! subtitle_policy_resolve_plan "$INPUT" "$INCLUDE_DEFAULT_MAIN_SUB"; then
  echo "Subtitle policy resolution failed: ${SUBTITLE_POLICY_ERROR:-unknown subtitle policy error}"
  exit 1
fi

if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
  echo "Subtitle policy selected ${SUBTITLE_POLICY_SELECTED_COUNT} stream(s); scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE} codec=${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}"
else
  echo "Subtitle policy selected no streams; scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE}"
fi

DEINTERLACE_FILTER="$(resolve_deinterlace_filter || true)"
CROP_FILTER="$(resolve_stable_crop_filter || true)"
if [ -n "$CROP_FILTER" ] && [ "$SUBTITLE_POLICY_HAS_BITMAP" = "1" ]; then
  echo "Disabling auto-crop because selected subtitle stream is bitmap-based (${SUBTITLE_POLICY_PRIMARY_CODEC:-unknown})"
  CROP_FILTER=""
fi

VIDEO_FILTER_CHAIN=""
VIDEO_FILTER_CHAIN="$(join_filters "$VIDEO_FILTER_CHAIN" "$DEINTERLACE_FILTER")"
VIDEO_FILTER_CHAIN="$(join_filters "$VIDEO_FILTER_CHAIN" "$CROP_FILTER")"

if [ -n "$DEINTERLACE_FILTER" ]; then
  echo "Applying deinterlace filter: ${DEINTERLACE_FILTER}"
fi
if [ -n "$CROP_FILTER" ]; then
  echo "Applying auto-crop filter: ${CROP_FILTER}"
fi

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

VIDEO_ARGS=()
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=(
    -c:v hevc_videotoolbox
    -pix_fmt p010le
    -b:v "${AVG_K_LEGACY}k"
    -maxrate "${MAXRATE_K_LEGACY}k"
    -bufsize "${BUFSIZE_K_LEGACY}k"
    "${COLOR_ARGS[@]}"
  )
else
  VIDEO_ARGS=(
    -c:v libx265
    -preset "$CPU_PRESET"
    -crf "$CRF_LEGACY"
    -x265-params "vbv-maxrate=${MAXRATE_K_LEGACY}:vbv-bufsize=${BUFSIZE_K_LEGACY}:aq-mode=3"
    -pix_fmt yuv420p10le
    "${COLOR_ARGS[@]}"
  )
fi
if [ -n "$VIDEO_FILTER_CHAIN" ]; then
  VIDEO_ARGS=(-vf "$VIDEO_FILTER_CHAIN" "${VIDEO_ARGS[@]}")
fi

ACTUAL_OUTPUT="$OUTPUT"
if [ "$SUBTITLE_POLICY_OUTPUT_CONTAINER" = "mkv" ]; then
  subtitle_pos=""
  subtitle_map_args=()
  OUTPUT_PATH="${OUTPUT%.*}.mkv"
  ACTUAL_OUTPUT="$OUTPUT_PATH"
  echo "Subtitle policy requires MKV output: $OUTPUT_PATH"
  while IFS= read -r subtitle_pos; do
    [ -n "$subtitle_pos" ] || continue
    subtitle_map_args+=(-map "0:s:${subtitle_pos}")
  done <<< "$SUBTITLE_POLICY_SELECTED_POSITIONS"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? \
    "${subtitle_map_args[@]}" \
    "${VIDEO_ARGS[@]}" \
    -c:a copy -c:s "${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}" \
    -max_muxing_queue_size 4096 \
    "$OUTPUT_PATH"
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
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "preserve"; then
  exit 1
fi
if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "preserve" "$VFO_DYNAMIC_RANGE_STRICT"
fi
