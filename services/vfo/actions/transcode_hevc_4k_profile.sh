#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 4K HEVC profile action for vfo scenario commands.
#
# Contract (called from vfo config):
#   transcode_hevc_4k_profile.sh <input_file> <output_file>
#
# Notes:
# - Preserves all audio/subtitle streams with stream copy.
# - Video path auto-selects Apple VideoToolbox when available unless overridden.
# - Intended for profile-action workflows where vfo handles scenario routing.

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
AVG_K="${AVG_K:-15000}"
MAXRATE_K="${MAXRATE_K:-20000}"
BUFSIZE_K="${BUFSIZE_K:-40000}"
CRF_4K="${CRF_4K:-18}"
CPU_PRESET="${CPU_PRESET:-slow}"

has_videotoolbox_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "hevc_videotoolbox"
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

ffmpeg -hide_banner -nostdin -y \
  -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
  -i "$INPUT" \
  -map 0:v:0 -map 0:a? -map 0:s? \
  "${VIDEO_ARGS[@]}" \
  -c:a copy -c:s copy \
  -max_muxing_queue_size 4096 \
  "$OUTPUT"
