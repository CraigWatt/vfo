#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 4K HEVC profile action with policy-driven subtitle handling.
#
# Contract (called from vfo config):
#   transcode_hevc_4k_main_subtitle_preserve_profile.sh <input_file> <output_file>
#
# Behavior:
# - Always preserves audio streams with stream copy.
# - Default subtitle behavior is `smart_eng_sub + preserve`.
# - Policy can be overridden by wrapper packs via:
#   VFO_SUBTITLE_SELECTION_SCOPE=smart_eng_sub|all_sub_preserve
#   VFO_SUBTITLE_MODE=preserve|subtitle_convert
#   VFO_SUBTITLE_CONVERT_BITMAP_POLICY=fail|preserve_mkv
# - Preserves dynamic-range signaling for HDR/DV workflows by default:
#   applies metadata-repair defaults when source tags are incomplete.
# - If source signals Dolby Vision side data, attempts DV RPU retention/injection.
# - If source is DV profile 7.x, attempts profile 8.1 conversion semantics before injection.
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
#   VFO_DV_REQUIRE_DOVI=1|0
#     default: 1
#   VFO_DV_CONVERT_P7_TO_81=1|0
#     default: 1
#   VFO_DV_P7_TO_81_MODE=2|5
#     default: 2
#   VFO_DV_REQUIRE_P7_TO_81=1|0
#     default: 1
#   VFO_DV_P7_EXTRACT_MODE=auto|mkvextract|ffmpeg
#     default: auto
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
# shellcheck source=subtitle_policy_tools.sh
. "$SCRIPT_DIR/subtitle_policy_tools.sh"
# shellcheck source=quality_mode_tools.sh
. "$SCRIPT_DIR/quality_mode_tools.sh"

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
VFO_DV_P7_EXTRACT_MODE="${VFO_DV_P7_EXTRACT_MODE:-auto}"

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
  cmd=(-hide_banner -nostdin -y -i "$input_video_mp4" -i "$input_source_media")
  maps=(-map 0:v:0 -map 1:a?)

  if [ -n "$subtitle_positions" ] && [ "$subtitle_codec" != "none" ]; then
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

get_fps() {
  local src="$1"
  local fps
  fps="$(ffprobe -v error -select_streams v:0 \
    -show_entries stream=avg_frame_rate -of default=nk=1:nw=1 "$src" 2>/dev/null || true)"
  fps="$(printf '%s' "$fps" | tr -d ' \t\r\n')"
  [ -n "$fps" ] || fps="24000/1001"
  printf '%s' "$fps"
}

has_mkv_input() {
  case "$(lower_text "$INPUT")" in
    *.mkv) return 0 ;;
    *) return 1 ;;
  esac
}

dv_profile_is_7_family() {
  case "$1" in
    7|7.*) return 0 ;;
    *) return 1 ;;
  esac
}

resolve_mkv_video_track_id() {
  mkvmerge -i "$INPUT" 2>/dev/null \
    | awk -F: '/[Vv]ideo/ { gsub(/Track ID /,"",$1); gsub(/^[[:space:]]+|[[:space:]]+$/,"",$1); print $1; exit }'
}

extract_source_hevc_for_dv() {
  local output_hevc="$1"
  local track_id=""

  case "$VFO_DV_P7_EXTRACT_MODE" in
    auto|mkvextract|ffmpeg) ;;
    *)
      echo "Invalid VFO_DV_P7_EXTRACT_MODE='${VFO_DV_P7_EXTRACT_MODE}' (allowed: auto|mkvextract|ffmpeg)"
      return 1
      ;;
  esac

  if [ "$VFO_DV_P7_EXTRACT_MODE" != "ffmpeg" ] \
    && has_mkv_input \
    && command -v mkvextract >/dev/null 2>&1 \
    && command -v mkvmerge >/dev/null 2>&1; then
    track_id="$(resolve_mkv_video_track_id || true)"
    if [ -n "$track_id" ]; then
      echo "Using mkvextract for DV source extraction (track id ${track_id})"
      if mkvextract tracks "$INPUT" "${track_id}:${output_hevc}" >/dev/null 2>&1; then
        [ -s "$output_hevc" ] && return 0
      fi
      echo "WARN: mkvextract DV source extraction failed; falling back to ffmpeg extraction"
    elif [ "$VFO_DV_P7_EXTRACT_MODE" = "mkvextract" ]; then
      echo "WARN: could not determine MKV video track id; falling back to ffmpeg extraction"
    fi
  fi

  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -c:v copy -bsf:v hevc_mp4toannexb -f hevc \
    "$output_hevc" >/dev/null 2>&1 || true
  [ -s "$output_hevc" ]
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
  "${COLOR_ARGS[@]}"
)

CPU_VIDEO_ARGS=(
  -c:v libx265
  -preset "$CPU_PRESET"
  -pix_fmt yuv420p10le
  "${COLOR_ARGS[@]}"
)

using_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  using_hw=1
fi

workdir="$(mktemp -d "${TMPDIR:-/tmp}/vfo-main-sub-4k-XXXXXX")"
trap 'rm -rf "$workdir"' EXIT

enc_mp4="$workdir/enc_video.mp4"
enc_hevc="$workdir/enc.hevc"
src_hevc="$workdir/src.hevc"
rpu_bin="$workdir/rpu.bin"
dv_hevc="$workdir/dv.hevc"
dv_mp4="$workdir/dv_video.mp4"
src_p81_hevc="$workdir/src_p81.hevc"
fps="$(get_fps "$INPUT")"

if ! subtitle_policy_resolve_plan "$INPUT" "$INCLUDE_DEFAULT_MAIN_SUB"; then
  echo "Subtitle policy resolution failed: ${SUBTITLE_POLICY_ERROR:-unknown subtitle policy error}"
  exit 1
fi

if [ "$SUBTITLE_POLICY_SELECTED_COUNT" -gt 0 ]; then
  echo "Subtitle policy selected ${SUBTITLE_POLICY_SELECTED_COUNT} stream(s); scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE} codec=${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}"
else
  echo "Subtitle policy selected no streams; scope=${SUBTITLE_POLICY_SELECTION_SCOPE} mode=${SUBTITLE_POLICY_MODE}"
fi

encode_4k_video_candidate() {
  local output_path="$1"
  local pass_index="$2"
  local current_avg=""
  local current_max=""
  local current_buf=""
  local current_crf=""
  local -a candidate_args=()

  current_avg="$(quality_mode_scale_kbits "$AVG_K" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_max="$(quality_mode_scale_kbits "$MAXRATE_K" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_buf="$(quality_mode_scale_kbits "$BUFSIZE_K" "$pass_index" "$QUALITY_MODE_VMAF_HW_REDUCTION_PCT")"
  current_crf="$(quality_mode_step_crf "$CRF_4K" "$pass_index" "$QUALITY_MODE_VMAF_CPU_CRF_STEP")"

  if [ "$using_hw" -eq 1 ]; then
    candidate_args=(
      "${HW_VIDEO_ARGS[@]}"
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
      -video_track_timescale "$MP4_TIMESCALE" \
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
    "${CPU_VIDEO_ARGS[@]}"
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
    -video_track_timescale "$MP4_TIMESCALE" \
    "$output_path" >/dev/null 2>&1
}

run_4k_quality_mode_encode() {
  local output_path="$1"
  local pass_index=0
  local candidate_path=""
  local score=""
  local best_candidate=""
  local best_score=""

  if ! quality_mode_is_aggressive_vmaf; then
    encode_4k_video_candidate "$output_path" 0
    return 0
  fi

  if [ "$DR_SOURCE_CLASS" != "sdr" ]; then
    echo "Aggressive VMAF requested, but 4K retries are currently limited to SDR sources; falling back to standard encode"
    encode_4k_video_candidate "$output_path" 0
    return 0
  fi

  if ! quality_mode_has_libvmaf; then
    echo "Aggressive VMAF requested, but ffmpeg libvmaf is unavailable; falling back to standard encode"
    encode_4k_video_candidate "$output_path" 0
    return 0
  fi

  while [ "$pass_index" -lt "$QUALITY_MODE_VMAF_MAX_PASSES" ]; do
    candidate_path="${workdir}/enc_video_pass_${pass_index}.mp4"
    encode_4k_video_candidate "$candidate_path" "$pass_index"
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

if ! run_4k_quality_mode_encode "$enc_mp4"; then
  echo "Failed to encode source to HEVC for main-subtitle workflow" >&2
  exit 1
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

    extract_source_hevc_for_dv "$src_hevc" || true

    if dv_profile_is_7_family "$source_dv_profile" && [ "$VFO_DV_CONVERT_P7_TO_81" = "1" ]; then
      p7_to_81_requested=1
      if [ -s "$src_hevc" ] && dovi_tool -m "$VFO_DV_P7_TO_81_MODE" extract-rpu -i "$src_hevc" -o "$rpu_bin" >/dev/null 2>&1; then
        p7_to_81_applied=1
      elif [ -s "$src_hevc" ] \
        && dovi_tool -m "$VFO_DV_P7_TO_81_MODE" convert "$src_hevc" -o "$src_p81_hevc" >/dev/null 2>&1 \
        && [ -s "$src_p81_hevc" ] \
        && dovi_tool extract-rpu -i "$src_p81_hevc" -o "$rpu_bin" >/dev/null 2>&1; then
        p7_to_81_applied=1
        echo "DV P7 fallback path used: convert source bitstream to P8.1 then extract RPU"
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
if [ "$SUBTITLE_POLICY_OUTPUT_CONTAINER" = "mkv" ]; then
  subtitle_map_args=()
  subtitle_pos=""
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  while IFS= read -r subtitle_pos; do
    [ -n "$subtitle_pos" ] || continue
    subtitle_map_args+=(-map "1:s:${subtitle_pos}")
  done <<< "$SUBTITLE_POLICY_SELECTED_POSITIONS"
  ffmpeg -hide_banner -nostdin -y \
    -i "$new_video" -i "$INPUT" \
    -map 0:v:0 -map 1:a? \
    "${subtitle_map_args[@]}" \
    -c:v copy -c:a copy -c:s "${SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC}" \
    -max_muxing_queue_size 4096 \
    "$ACTUAL_OUTPUT"
else
  finalize_streamable_mp4 \
    "$new_video" \
    "$INPUT" \
    "$SUBTITLE_POLICY_SELECTED_POSITIONS" \
    "$SUBTITLE_POLICY_OUTPUT_SUBTITLE_CODEC" \
    "$ACTUAL_OUTPUT"
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
