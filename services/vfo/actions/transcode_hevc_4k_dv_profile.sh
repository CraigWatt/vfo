#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 4K HEVC profile action with best-effort Dolby Vision metadata retention.
#
# Contract (called from vfo config):
#   transcode_hevc_4k_dv_profile.sh <input_file> <output_file>
#
# Behavior:
# - If source signals Dolby Vision side data and dovi_tool is available:
#   attempt RPU extraction and injection into encoded HEVC output.
# - If source DV profile is 7, convert metadata to profile 8.1 before injection.
# - If any DV step fails, gracefully falls back to HDR10-compatible HEVC output.
# - Preserves audio/subtitle streams with stream copy.
#
# Optional env:
# - VFO_DV_REQUIRE_DOVI=1 : fail if source is DV but DV retention cannot be completed.
# - VFO_DV_CONVERT_P7_TO_81=1 : convert DV profile 7 metadata to 8.1 (default: 1).
# - VFO_DV_P7_TO_81_MODE=2 : dovi_tool conversion mode for profile 7 -> 8.1 (2 or 5).
# - VFO_DV_REQUIRE_P7_TO_81=1 : fail if source is profile 7 but conversion to 8.1 fails.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=live_encode_tools.sh
. "$SCRIPT_DIR/live_encode_tools.sh"
# shellcheck source=dv_p7_to_p81_tools.sh
. "$SCRIPT_DIR/dv_p7_to_p81_tools.sh"

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
PROBE_SIZE="${PROBE_SIZE:-200M}"
ANALYZE_DUR="${ANALYZE_DUR:-200M}"
AVG_K="${AVG_K:-15000}"
MAXRATE_K="${MAXRATE_K:-20000}"
BUFSIZE_K="${BUFSIZE_K:-40000}"
CRF_4K="${CRF_4K:-18}"
CPU_PRESET="${CPU_PRESET:-slow}"
VFO_DV_REQUIRE_DOVI="${VFO_DV_REQUIRE_DOVI:-0}"
VFO_DV_CONVERT_P7_TO_81="${VFO_DV_CONVERT_P7_TO_81:-1}"
VFO_DV_P7_TO_81_MODE="${VFO_DV_P7_TO_81_MODE:-2}"
VFO_DV_REQUIRE_P7_TO_81="${VFO_DV_REQUIRE_P7_TO_81:-1}"
MP4_TIMESCALE="${MP4_TIMESCALE:-24000}"

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "hevc_videotoolbox"
}

has_dovi_side_data() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$1" 2>/dev/null | grep -qi "dovi"
}

get_dovi_profile() {
  local src="$1"
  local profile=""
  profile="$(ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$src" 2>/dev/null | awk -F= '/^dv_profile=/{print $2; exit}' | tr -d ' \t\r\n' || true)"
  printf '%s' "$profile"
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

HW_VIDEO_ARGS=(
  -c:v hevc_videotoolbox
  -pix_fmt p010le
  -b:v "${AVG_K}k"
  -maxrate "${MAXRATE_K}k"
  -bufsize "${BUFSIZE_K}k"
)

CPU_VIDEO_ARGS=(
  -c:v libx265
  -preset "$CPU_PRESET"
  -crf "$CRF_4K"
  -x265-params "vbv-maxrate=${MAXRATE_K}:vbv-bufsize=${BUFSIZE_K}:aq-mode=3"
  -pix_fmt yuv420p10le
)

VIDEO_ARGS=()
using_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=("${HW_VIDEO_ARGS[@]}")
  using_hw=1
else
  VIDEO_ARGS=("${CPU_VIDEO_ARGS[@]}")
fi

SOURCE_INPUT="$INPUT"
SOURCE_DV_PROFILE="$(dv_p7_to_p81_detect_dv_profile "$SOURCE_INPUT")"
workdir="$(vfo_drive_backed_tmpdir "$OUTPUT")"
trap 'rm -rf "$workdir"' EXIT

INPUT="$(dv_p7_to_p81_prepare_input "$SOURCE_INPUT" "$workdir")" || {
  echo "Failed to prepare a stable DV source for profile processing" >&2
  exit 1
}
if [ "$INPUT" != "$SOURCE_INPUT" ]; then
  echo "DV PREP: using preconverted P8.1 input for subsequent 4K profile stages"
fi

enc_mp4="$workdir/enc_video.mp4"
enc_hevc="$workdir/enc.hevc"
src_hevc="$workdir/src.hevc"
rpu_bin="$workdir/rpu.bin"
dv_hevc="$workdir/dv.hevc"
dv_mp4="$workdir/dv_video.mp4"
fps="$(get_fps "$INPUT")"

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
    echo "Failed to encode source to HEVC for DV workflow" >&2
    exit 1
  fi
fi

new_video="$enc_mp4"
source_is_dv=0
source_dv_profile=""
p7_to_81_requested=0
p7_to_81_applied=0

if [ -n "$SOURCE_DV_PROFILE" ]; then
  source_is_dv=1
  source_dv_profile="$SOURCE_DV_PROFILE"
fi

if [ "$source_is_dv" -eq 1 ] && command -v dovi_tool >/dev/null 2>&1; then
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
          output_dv_profile="$(get_dovi_profile "$dv_mp4")"
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

if [ "$p7_to_81_requested" -eq 1 ] && [ "$VFO_DV_REQUIRE_P7_TO_81" = "1" ] && [ "$new_video" != "$dv_mp4" ]; then
  echo "Source contains Dolby Vision profile 7 but profile 8.1 conversion failed (VFO_DV_REQUIRE_P7_TO_81=1)"
  exit 1
fi

if [ "$source_is_dv" -eq 1 ] && [ "$VFO_DV_REQUIRE_DOVI" = "1" ] && [ "$new_video" != "$dv_mp4" ]; then
  echo "Source contains Dolby Vision but DV retention failed (VFO_DV_REQUIRE_DOVI=1)"
  exit 1
fi

ffmpeg -hide_banner -nostdin -y \
  -i "$new_video" -i "$INPUT" \
  -map 0:v:0 -map 1:a? -map 1:s? \
  -c:v copy -c:a copy -c:s copy \
  -max_muxing_queue_size 4096 \
  "$OUTPUT"
