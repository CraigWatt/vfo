#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 1080p H.264 device-family action with explicit HDR->SDR
# conversion, all-sub subtitle selection, delivery-oriented subtitle conversion,
# and DTS/PCM-family audio conform handling.
#
# Contract (called from vfo config):
#   transcode_h264_1080_all_sub_convert_audio_conform_profile.sh <input_file> <output_file>
#
# Behavior:
# - Default subtitle behavior is `all_sub_preserve + subtitle_convert`.
# - Text subtitles are normalized into `mov_text` when MP4 remains viable.
# - Bitmap subtitles fall back to MKV preservation by default.
# - Preserves AAC and Dolby-family audio streams by default.
# - Conforms DTS-family and PCM-family audio streams when needed.
# - Preserved non-MP4-safe audio forces MKV output.
# - Uses proper zscale+tonemap when available for HDR->SDR conversion.
# - Falls back to an SDR-signaled compatibility transcode when tonemap filters are unavailable.
# - `aggressive_vmaf` is supported for SDR reference inputs only; HDR/HLG inputs fall back to standard encode because this lane intentionally changes dynamic range.
#
# Optional env:
#   VFO_ENCODER_MODE=auto|hw|cpu
#   VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart
#   VFO_SUBTITLE_SELECTION_SCOPE=all_sub_preserve
#   VFO_SUBTITLE_MODE=subtitle_convert
#   VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv|fail
#   VFO_QUALITY_MODE=standard|aggressive_vmaf
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

export VFO_SUBTITLE_SELECTION_SCOPE="${VFO_SUBTITLE_SELECTION_SCOPE:-all_sub_preserve}"
export VFO_SUBTITLE_MODE="${VFO_SUBTITLE_MODE:-subtitle_convert}"
export VFO_SUBTITLE_CONVERT_BITMAP_POLICY="${VFO_SUBTITLE_CONVERT_BITMAP_POLICY:-preserve_mkv}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=dynamic_range_tools.sh
. "$SCRIPT_DIR/dynamic_range_tools.sh"
# shellcheck source=audio_conform_tools.sh
. "$SCRIPT_DIR/audio_conform_tools.sh"
# shellcheck source=subtitle_policy_tools.sh
. "$SCRIPT_DIR/subtitle_policy_tools.sh"
# shellcheck source=quality_mode_tools.sh
. "$SCRIPT_DIR/quality_mode_tools.sh"

ENCODER_MODE="${VFO_ENCODER_MODE:-auto}" # auto|hw|cpu
MP4_STREAM_MODE="${VFO_MP4_STREAM_MODE:-fmp4_faststart}"
PROBE_SIZE="${PROBE_SIZE:-200M}"
ANALYZE_DUR="${ANALYZE_DUR:-200M}"
AVG_K_1080_H264="${AVG_K_1080_H264:-6500}"
MAXRATE_K_1080_H264="${MAXRATE_K_1080_H264:-8500}"
BUFSIZE_K_1080_H264="${BUFSIZE_K_1080_H264:-17000}"
CRF_1080_H264="${CRF_1080_H264:-20}"
CPU_PRESET="${CPU_PRESET:-slow}"
VFO_DYNAMIC_METADATA_REPAIR="${VFO_DYNAMIC_METADATA_REPAIR:-1}"
VFO_DYNAMIC_RANGE_STRICT="${VFO_DYNAMIC_RANGE_STRICT:-1}"
VFO_DYNAMIC_RANGE_REPORT="${VFO_DYNAMIC_RANGE_REPORT:-1}"

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "h264_videotoolbox"
}

has_filter() {
  local name="$1"
  ffmpeg -hide_banner -filters 2>/dev/null | grep -qE "[[:space:]]${name}[[:space:]]"
}

probe_video_transfer() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=color_transfer \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

is_hdr_transfer() {
  case "$1" in
    smpte2084|arib-std-b67) return 0 ;;
    *) return 1 ;;
  esac
}

build_filter_chain() {
  local transfer="$1"

  if is_hdr_transfer "$transfer"; then
    if has_filter zscale && has_filter tonemap; then
      printf '%s' "zscale=t=linear:npl=100,format=gbrpf32le,tonemap=mobius:desat=0,zscale=t=bt709:m=bt709:p=bt709:r=tv,format=yuv420p,scale=1920:1080:force_original_aspect_ratio=decrease"
      return 0
    fi
    echo "WARN: HDR source detected (${transfer}), but zscale+tonemap is unavailable; using SDR-signaled compatibility fallback" >&2
  fi

  printf '%s' "scale=1920:1080:force_original_aspect_ratio=decrease,format=yuv420p"
}

dr_collect_source_state "$INPUT"
dr_compute_target_tags "sdr1080"

if ! subtitle_policy_resolve_plan "$INPUT" 0; then
  echo "Subtitle policy resolution failed: ${SUBTITLE_POLICY_ERROR:-unknown subtitle policy error}"
  exit 1
fi

if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
  echo "Subtitle policy selected ${SUBTITLE_POLICY_SELECTED_COUNT} stream(s); scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE} codec=${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}"
else
  echo "Subtitle policy selected no streams; scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE}"
fi

workdir="$(vfo_drive_backed_tmpdir "$OUTPUT")"
trap 'rm -rf "$workdir"' EXIT

video_work_output="${workdir}/enc_video.mp4"
audio_work_output="${workdir}/audio_work.mka"
video_transfer="$(probe_video_transfer "$INPUT" || true)"
filter_chain="$(build_filter_chain "$video_transfer")"

audio_conform_render_audio_work "$INPUT" "$audio_work_output"

encode_1080_video_candidate() {
  local output_path="$1"
  local pass_index="$2"
  local current_avg=""
  local current_max=""
  local current_buf=""
  local current_crf=""

  current_avg="$(quality_mode_scale_kbits "$AVG_K_1080_H264" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_max="$(quality_mode_scale_kbits "$MAXRATE_K_1080_H264" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_buf="$(quality_mode_scale_kbits "$BUFSIZE_K_1080_H264" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_crf="$(quality_mode_step_crf "$CRF_1080_H264" "$pass_index" "$QUALITY_MODE_VMAF_CPU_CRF_STEP")"

  if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
    if ffmpeg -hide_banner -nostdin -y \
      -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
      -i "$INPUT" \
      -map 0:v:0 \
      -vf "$filter_chain" \
      -c:v h264_videotoolbox \
      -pix_fmt yuv420p \
      -b:v "${current_avg}k" \
      -maxrate "${current_max}k" \
      -bufsize "${current_buf}k" \
      -profile:v high -level:v 4.1 \
      -colorspace bt709 -color_trc bt709 -color_primaries bt709 \
      -movflags +faststart \
      -max_muxing_queue_size 4096 \
      "$output_path" >/dev/null 2>&1; then
      echo "Encode pass $((pass_index + 1)): hardware H.264 avg=${current_avg}k maxrate=${current_max}k bufsize=${current_buf}k"
      return 0
    fi

    if [ "$ENCODER_MODE" = "auto" ]; then
      echo "VideoToolbox H.264 encode failed during pass $((pass_index + 1)); falling back to CPU for remaining passes" >&2
      ENCODER_MODE="cpu"
    else
      return 1
    fi
  fi

  echo "Encode pass $((pass_index + 1)): CPU H.264 crf=${current_crf} maxrate=${current_max}k bufsize=${current_buf}k"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 \
    -vf "$filter_chain" \
    -c:v libx264 \
    -preset "$CPU_PRESET" \
    -crf "$current_crf" \
    -profile:v high -level:v 4.1 \
    -pix_fmt yuv420p \
    -x264-params "vbv-maxrate=${current_max}:vbv-bufsize=${current_buf}:keyint=240:min-keyint=24" \
    -colorspace bt709 -color_trc bt709 -color_primaries bt709 \
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
    echo "Aggressive VMAF requested, but this HDR->SDR lane only enables bounded retries for SDR inputs; falling back to standard encode"
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
FINAL_SUBTITLE_CODEC="$SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC"
if [ "$SUBTITLE_POLICY_OUTPUT_CONTAINER" = "mkv" ] || [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ] && [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
    echo "Final container: MKV (subtitle policy + preserved non-MP4-safe audio)"
  elif [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
    echo "Final container: MKV (subtitle policy)"
  else
    echo "Final container: MKV (preserved non-MP4-safe audio)"
  fi
  if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
    FINAL_SUBTITLE_CODEC="copy"
  else
    FINAL_SUBTITLE_CODEC="none"
  fi
  audio_conform_mux_mkv "$video_work_output" "$AUDIO_CONFORM_WORK_FILE" "$INPUT" "$SUBTITLE_POLICY_SELECTED_POSITIONS" "$FINAL_SUBTITLE_CODEC" "$ACTUAL_OUTPUT"
else
  echo "Final container: stream-ready MP4"
  audio_conform_finalize_streamable_mp4 "$video_work_output" "$AUDIO_CONFORM_WORK_FILE" "$INPUT" "$SUBTITLE_POLICY_SELECTED_POSITIONS" "$FINAL_SUBTITLE_CODEC" "$ACTUAL_OUTPUT" "$MP4_STREAM_MODE"
fi

dr_collect_output_state "$ACTUAL_OUTPUT"
if ! dr_validate_output_against_source "$VFO_DYNAMIC_RANGE_STRICT" "sdr1080"; then
  exit 1
fi
if [ "$VFO_DYNAMIC_RANGE_REPORT" = "1" ]; then
  dr_write_report "$ACTUAL_OUTPUT" "$(basename "$0")" "sdr1080" "$VFO_DYNAMIC_RANGE_STRICT"
fi
