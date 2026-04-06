#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware legacy/sub-HD HEVC profile action with smart-English-subtitle
# preserve and DTS/PCM-family audio conform handling.
#
# Contract (called from vfo config):
#   transcode_hevc_legacy_smart_eng_sub_audio_conform_profile.sh <input_file> <output_file>
#
# Behavior:
# - Preserves one selected English subtitle when it appears director-intent oriented:
#   priority: forced english -> forced untagged/unknown -> optional default english.
#   non-english forced tracks are intentionally skipped.
# - Preserves AAC and Dolby-family audio streams by default.
# - Conforms DTS-family and PCM-family audio streams:
#   DTS or PCM mono/stereo -> AAC + loudnorm
#   DTS or PCM 3.0/4.0/5.0/5.1 -> E-AC-3 when available, else AC-3, with loudnorm
#   DTS or PCM > 5.1 -> 5.1 E-AC-3/AC-3 downmix, with loudnorm
# - Preserved non-MP4-safe audio (for example TrueHD) forces MKV output.
# - Preserves dynamic-range signaling for HDR workflows by default:
#   applies metadata-repair defaults when source tags are incomplete.
# - Optionally applies deinterlace when input is interlaced (default: auto).
# - Optionally applies stable black-bar auto-crop for persistent bars.
# - If selected subtitle is bitmap-based, crop is disabled for subtitle placement safety.
# - If a smart English subtitle is selected, output container is MKV.
# - If no subtitle is selected and preserved audio is MP4-safe, output container is
#   stream-ready MP4 (fragmented MP4 with init/moov at the start, with faststart
#   fallback when E-AC-3 packaging needs it).
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
#   VFO_AUDIO_CONFORM_TARGET_I=-14
#   VFO_AUDIO_CONFORM_TARGET_TP=-1.5
#   VFO_AUDIO_CONFORM_TARGET_LRA=11
#   VFO_QUALITY_MODE=standard|aggressive_vmaf
#     default: standard
#   VFO_QUALITY_VMAF_MIN=94
#   VFO_QUALITY_VMAF_MAX_PASSES=4

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
# shellcheck source=audio_conform_tools.sh
. "$SCRIPT_DIR/audio_conform_tools.sh"
# shellcheck source=quality_mode_tools.sh
. "$SCRIPT_DIR/quality_mode_tools.sh"

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

is_bitmap_subtitle_codec() {
  case "$(lower_text "$1")" in
    hdmv_pgs_subtitle|dvd_subtitle|dvb_subtitle|xsub|pgs|vobsub) return 0 ;;
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

get_sub_stream_codec() {
  local sub_pos="$1"
  ffprobe -v error \
    -select_streams "s:${sub_pos}" \
    -show_entries stream=codec_name \
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

MAIN_SUB_POS=""
MAIN_SUB_CODEC=""
if MAIN_SUB_POS="$(resolve_main_subtitle_position)"; then
  MAIN_SUB_CODEC="$(get_sub_stream_codec "$MAIN_SUB_POS")"
  [ -n "$MAIN_SUB_CODEC" ] || MAIN_SUB_CODEC="unknown"
  echo "Smart English subtitle detected (s:${MAIN_SUB_POS}, codec=${MAIN_SUB_CODEC})"
else
  echo "No smart English subtitle detected"
fi

DEINTERLACE_FILTER="$(resolve_deinterlace_filter || true)"
CROP_FILTER="$(resolve_stable_crop_filter || true)"
if [ -n "$CROP_FILTER" ] && [ -n "$MAIN_SUB_POS" ] && is_bitmap_subtitle_codec "$MAIN_SUB_CODEC"; then
  echo "Disabling auto-crop because selected subtitle stream is bitmap-based (${MAIN_SUB_CODEC})"
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

HW_VIDEO_BASE_ARGS=(
  -c:v hevc_videotoolbox
  -pix_fmt p010le
  "${COLOR_ARGS[@]}"
)

CPU_VIDEO_BASE_ARGS=(
  -c:v libx265
  -preset "$CPU_PRESET"
  -pix_fmt yuv420p10le
  "${COLOR_ARGS[@]}"
)

if [ -n "$VIDEO_FILTER_CHAIN" ]; then
  HW_VIDEO_BASE_ARGS=(-vf "$VIDEO_FILTER_CHAIN" "${HW_VIDEO_BASE_ARGS[@]}")
  CPU_VIDEO_BASE_ARGS=(-vf "$VIDEO_FILTER_CHAIN" "${CPU_VIDEO_BASE_ARGS[@]}")
fi

using_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  using_hw=1
fi

workdir="$(mktemp -d "${TMPDIR:-/tmp}/vfo-smart-eng-audio-legacy-XXXXXX")"
trap 'rm -rf "$workdir"' EXIT

video_work_output="${workdir}/enc_video.mp4"
audio_work_output="${workdir}/audio_work.mka"

audio_conform_render_audio_work "$INPUT" "$audio_work_output"

encode_legacy_video_candidate() {
  local output_path="$1"
  local pass_index="$2"
  local current_avg=""
  local current_max=""
  local current_buf=""
  local current_crf=""
  local -a candidate_args=()

  current_avg="$(quality_mode_scale_kbits "$AVG_K_LEGACY" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_max="$(quality_mode_scale_kbits "$MAXRATE_K_LEGACY" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_buf="$(quality_mode_scale_kbits "$BUFSIZE_K_LEGACY" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_crf="$(quality_mode_step_crf "$CRF_LEGACY" "$pass_index" "$QUALITY_MODE_VMAF_CPU_CRF_STEP")"

  if [ "$using_hw" -eq 1 ]; then
    candidate_args=(
      "${HW_VIDEO_BASE_ARGS[@]}"
      -b:v "${current_avg}k"
      -maxrate "${current_max}k"
      -bufsize "${current_buf}k"
    )
    if ffmpeg -hide_banner -nostdin -y \
      -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
      -i "$INPUT" \
      -map 0:v:0 \
      "${candidate_args[@]}" \
      -movflags +faststart \
      -max_muxing_queue_size 4096 \
      "$output_path" >/dev/null 2>&1; then
      echo "Encode pass $((pass_index + 1)): hardware HEVC avg=${current_avg}k maxrate=${current_max}k bufsize=${current_buf}k"
      return 0
    fi

    if [ "$ENCODER_MODE" = "auto" ]; then
      echo "VideoToolbox encode failed during pass $((pass_index + 1)); falling back to CPU for remaining passes" >&2
      using_hw=0
    else
      return 1
    fi
  fi

  candidate_args=(
    "${CPU_VIDEO_BASE_ARGS[@]}"
    -crf "$current_crf"
    -x265-params "vbv-maxrate=${current_max}:vbv-bufsize=${current_buf}:aq-mode=3"
  )
  echo "Encode pass $((pass_index + 1)): CPU HEVC crf=${current_crf} maxrate=${current_max}k bufsize=${current_buf}k"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 \
    "${candidate_args[@]}" \
    -movflags +faststart \
    -max_muxing_queue_size 4096 \
    "$output_path" >/dev/null 2>&1
}

run_legacy_quality_mode_encode() {
  local output_path="$1"
  local pass_index=0
  local candidate_path=""
  local score=""
  local best_candidate=""
  local best_score=""

  if ! quality_mode_is_aggressive_vmaf; then
    encode_legacy_video_candidate "$output_path" 0
    return 0
  fi

  if [ "$DR_SOURCE_CLASS" != "sdr" ]; then
    echo "Aggressive VMAF requested, but this lane only enables bounded retries for SDR inputs; falling back to standard encode"
    encode_legacy_video_candidate "$output_path" 0
    return 0
  fi

  if ! quality_mode_has_libvmaf; then
    echo "Aggressive VMAF requested, but ffmpeg libvmaf is unavailable; falling back to standard encode"
    encode_legacy_video_candidate "$output_path" 0
    return 0
  fi

  while [ "$pass_index" -lt "$QUALITY_MODE_VMAF_MAX_PASSES" ]; do
    candidate_path="${workdir}/enc_video_pass_${pass_index}.mp4"
    encode_legacy_video_candidate "$candidate_path" "$pass_index"
    score="$(quality_mode_measure_vmaf "$candidate_path" "$INPUT" || true)"
    if [ -z "$score" ]; then
      echo "Aggressive VMAF pass $((pass_index + 1)) could not parse a score; keeping the latest candidate"
      [ -n "$best_candidate" ] || best_candidate="$candidate_path"
      break
    fi

    echo "Aggressive VMAF pass $((pass_index + 1))/${QUALITY_MODE_VMAF_MAX_PASSES}: score=${score} floor=${QUALITY_MODE_VMAF_MIN}"

    if [ -z "$best_candidate" ] || quality_mode_score_meets_floor "$score" "$QUALITY_MODE_VMAF_MIN"; then
      best_candidate="$candidate_path"
      best_score="$score"
    fi

    if ! quality_mode_score_meets_floor "$score" "$QUALITY_MODE_VMAF_MIN"; then
      if [ "$pass_index" -eq 0 ]; then
        echo "Baseline encode is already below the VMAF floor; preserving baseline output"
        best_candidate="$candidate_path"
      fi
      break
    fi

    pass_index=$((pass_index + 1))
  done

  [ -n "$best_candidate" ] || {
    echo "Aggressive VMAF could not produce any candidate output" >&2
    return 1
  }

  mv "$best_candidate" "$output_path"
  if [ -n "$best_score" ]; then
    echo "Aggressive VMAF selected candidate score=${best_score}"
  fi
}

run_legacy_quality_mode_encode "$video_work_output"

ACTUAL_OUTPUT="$OUTPUT"
if [ -n "$MAIN_SUB_POS" ] || [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  if [ -n "$MAIN_SUB_POS" ] && [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
    echo "Final container: MKV (smart English subtitle + preserved non-MP4-safe audio)"
  elif [ -n "$MAIN_SUB_POS" ]; then
    echo "Final container: MKV (smart English subtitle preserved)"
  else
    echo "Final container: MKV (preserved non-MP4-safe audio)"
  fi
  audio_conform_mux_mkv "$video_work_output" "$AUDIO_CONFORM_WORK_FILE" "$INPUT" "$MAIN_SUB_POS" "$ACTUAL_OUTPUT"
else
  echo "Final container: stream-ready MP4"
  audio_conform_finalize_streamable_mp4 "$video_work_output" "$AUDIO_CONFORM_WORK_FILE" "$ACTUAL_OUTPUT" "$MP4_STREAM_MODE"
fi

dr_collect_output_state "$ACTUAL_OUTPUT"
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "preserve"; then
  exit 1
fi
if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "preserve" "$VFO_DYNAMIC_RANGE_STRICT"
fi
