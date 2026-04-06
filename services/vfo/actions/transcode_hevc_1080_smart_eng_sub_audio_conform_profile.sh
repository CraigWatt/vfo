#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 1080p HEVC profile action with smart-English-subtitle preserve
# and DTS/PCM-family audio conform handling.
#
# Contract (called from vfo config):
#   transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh <input_file> <output_file>
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
# - Enforces SDR-oriented 1080 policy metadata on output (`bt709` signaling).
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
AVG_K_1080="${AVG_K_1080:-8000}"
MAXRATE_K_1080="${MAXRATE_K_1080:-12000}"
BUFSIZE_K_1080="${BUFSIZE_K_1080:-24000}"
CRF_1080="${CRF_1080:-19}"
CPU_PRESET="${CPU_PRESET:-slow}"
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

HW_VIDEO_BASE_ARGS=(
  -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
  -c:v hevc_videotoolbox
  -pix_fmt p010le
  "${COLOR_ARGS[@]}"
)

CPU_VIDEO_BASE_ARGS=(
  -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
  -c:v libx265
  -preset "$CPU_PRESET"
  -pix_fmt yuv420p10le
  "${COLOR_ARGS[@]}"
)

using_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  using_hw=1
fi

workdir="$(mktemp -d "${TMPDIR:-/tmp}/vfo-smart-eng-audio-1080-XXXXXX")"
trap 'rm -rf "$workdir"' EXIT

video_work_output="${workdir}/enc_video.mp4"
audio_work_output="${workdir}/audio_work.mka"

MAIN_SUB_POS=""
if MAIN_SUB_POS="$(resolve_main_subtitle_position)"; then
  echo "Smart English subtitle detected (s:${MAIN_SUB_POS})"
else
  echo "No smart English subtitle detected"
fi

audio_conform_render_audio_work "$INPUT" "$audio_work_output"

encode_1080_video_candidate() {
  local output_path="$1"
  local pass_index="$2"
  local current_avg=""
  local current_max=""
  local current_buf=""
  local current_crf=""
  local -a candidate_args=()

  current_avg="$(quality_mode_scale_kbits "$AVG_K_1080" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_max="$(quality_mode_scale_kbits "$MAXRATE_K_1080" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_buf="$(quality_mode_scale_kbits "$BUFSIZE_K_1080" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_crf="$(quality_mode_step_crf "$CRF_1080" "$pass_index" "$QUALITY_MODE_VMAF_CPU_CRF_STEP")"

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

run_1080_quality_mode_encode() {
  local output_path="$1"
  local pass_index=0
  local candidate_path=""
  local score=""
  local best_candidate=""
  local best_score=""

  if ! quality_mode_is_aggressive_vmaf; then
    encode_1080_video_candidate "$output_path" 0
    return 0
  fi

  if [ "$DR_SOURCE_CLASS" != "sdr" ]; then
    echo "Aggressive VMAF requested, but this lane only enables bounded retries for SDR inputs; falling back to standard encode"
    encode_1080_video_candidate "$output_path" 0
    return 0
  fi

  if ! quality_mode_has_libvmaf; then
    echo "Aggressive VMAF requested, but ffmpeg libvmaf is unavailable; falling back to standard encode"
    encode_1080_video_candidate "$output_path" 0
    return 0
  fi

  while [ "$pass_index" -lt "$QUALITY_MODE_VMAF_MAX_PASSES" ]; do
    candidate_path="${workdir}/enc_video_pass_${pass_index}.mp4"
    encode_1080_video_candidate "$candidate_path" "$pass_index"
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

run_1080_quality_mode_encode "$video_work_output"

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
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "sdr1080"; then
  exit 1
fi
if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "sdr1080" "$VFO_DYNAMIC_RANGE_STRICT"
fi
