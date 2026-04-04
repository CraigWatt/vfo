#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 4K HEVC profile action with smart-English-subtitle preserve,
# DTS/PCM-family audio conform handling, and Dolby Vision retention.
#
# Contract (called from vfo config):
#   transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh <input_file> <output_file>
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
# - Preserves dynamic-range signaling for HDR/DV workflows by default:
#   applies metadata-repair defaults when source tags are incomplete.
# - If source signals Dolby Vision side data, attempts DV RPU retention/injection.
# - If source is DV profile 7.x, attempts profile 8.1 conversion semantics before injection.
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
#   VFO_AUDIO_CONFORM_TARGET_I=-14
#   VFO_AUDIO_CONFORM_TARGET_TP=-1.5
#   VFO_AUDIO_CONFORM_TARGET_LRA=11

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

workdir="$(mktemp -d "${TMPDIR:-/tmp}/vfo-smart-eng-audio-4k-XXXXXX")"
trap 'rm -rf "$workdir"' EXIT

enc_mp4="$workdir/enc_video.mp4"
enc_hevc="$workdir/enc.hevc"
src_hevc="$workdir/src.hevc"
rpu_bin="$workdir/rpu.bin"
dv_hevc="$workdir/dv.hevc"
dv_mp4="$workdir/dv_video.mp4"
src_p81_hevc="$workdir/src_p81.hevc"
audio_work_output="$workdir/audio_work.mka"
fps="$(get_fps "$INPUT")"

MAIN_SUB_POS=""
if MAIN_SUB_POS="$(resolve_main_subtitle_position)"; then
  echo "Smart English subtitle detected (s:${MAIN_SUB_POS})"
else
  echo "No smart English subtitle detected"
fi

audio_conform_render_audio_work "$INPUT" "$audio_work_output"

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
    echo "Failed to encode source to HEVC for smart-eng-sub audio-conform workflow" >&2
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
if [ -n "$MAIN_SUB_POS" ] || [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
  ACTUAL_OUTPUT="${OUTPUT%.*}.mkv"
  if [ -n "$MAIN_SUB_POS" ] && [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
    echo "Final container: MKV (smart English subtitle + preserved non-MP4-safe audio)"
  elif [ -n "$MAIN_SUB_POS" ]; then
    echo "Final container: MKV (smart English subtitle preserved)"
  else
    echo "Final container: MKV (preserved non-MP4-safe audio)"
  fi
  audio_conform_mux_mkv "$new_video" "$AUDIO_CONFORM_WORK_FILE" "$INPUT" "$MAIN_SUB_POS" "$ACTUAL_OUTPUT"
else
  echo "Final container: stream-ready MP4"
  audio_conform_finalize_streamable_mp4 "$new_video" "$AUDIO_CONFORM_WORK_FILE" "$ACTUAL_OUTPUT" "$MP4_STREAM_MODE"
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
