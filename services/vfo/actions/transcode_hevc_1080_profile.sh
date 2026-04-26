#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 1080p HEVC profile action for vfo scenario commands.
#
# Contract (called from vfo config):
#   transcode_hevc_1080_profile.sh <input_file> <output_file>
#
# Notes:
# - Downscales to <=1080p while preserving aspect ratio.
# - Preserves all audio/subtitle streams with stream copy.
# - Video path auto-selects Apple VideoToolbox when available unless overridden.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=live_encode_tools.sh
. "$SCRIPT_DIR/live_encode_tools.sh"

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
AVG_K_1080="${AVG_K_1080:-8000}"
MAXRATE_K_1080="${MAXRATE_K_1080:-12000}"
BUFSIZE_K_1080="${BUFSIZE_K_1080:-24000}"
CRF_1080="${CRF_1080:-19}"
CPU_PRESET="${CPU_PRESET:-slow}"

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "hevc_videotoolbox"
}

VIDEO_ARGS=()
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  VIDEO_ARGS=(
    -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
    -c:v hevc_videotoolbox
    -pix_fmt p010le
    -b:v "${AVG_K_1080}k"
    -maxrate "${MAXRATE_K_1080}k"
    -bufsize "${BUFSIZE_K_1080}k"
  )
else
  VIDEO_ARGS=(
    -vf "scale=1920:1080:force_original_aspect_ratio=decrease"
    -c:v libx265
    -preset "$CPU_PRESET"
    -crf "$CRF_1080"
    -x265-params "vbv-maxrate=${MAXRATE_K_1080}:vbv-bufsize=${BUFSIZE_K_1080}:aq-mode=3"
    -pix_fmt yuv420p10le
  )
fi

ffmpeg -hide_banner -nostdin -y \
  -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
  -i "$INPUT" \
  -map 0:v:0 -map 0:a? -map 0:s? \
  "${VIDEO_ARGS[@]}" \
  -c:a copy -c:s copy \
  -max_muxing_queue_size 4096 \
  "$OUTPUT"
