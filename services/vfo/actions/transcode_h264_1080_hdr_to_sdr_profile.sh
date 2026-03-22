#!/usr/bin/env bash
set -euo pipefail

# Hardware-aware 1080p H.264 action with explicit HDR->SDR conversion intent.
#
# Contract (called from vfo config):
#   transcode_h264_1080_hdr_to_sdr_profile.sh <input_file> <output_file>
#
# Notes:
# - Downscales to <=1080p while preserving aspect ratio.
# - Preserves all audio/subtitle streams with stream copy.
# - Converts HDR transfer to SDR (BT.709) for SDR device compatibility profiles.

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
AVG_K_1080_H264="${AVG_K_1080_H264:-6500}"
MAXRATE_K_1080_H264="${MAXRATE_K_1080_H264:-8500}"
BUFSIZE_K_1080_H264="${BUFSIZE_K_1080_H264:-17000}"
CRF_1080_H264="${CRF_1080_H264:-20}"
CPU_PRESET="${CPU_PRESET:-slow}"

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

    if has_filter colorspace; then
      printf '%s' "scale=1920:1080:force_original_aspect_ratio=decrease,colorspace=all=bt709:fast=1,format=yuv420p"
      return 0
    fi
  fi

  printf '%s' "scale=1920:1080:force_original_aspect_ratio=decrease,format=yuv420p"
}

run_hw_encode() {
  local filter_chain="$1"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? -map 0:s? \
    -vf "$filter_chain" \
    -c:v h264_videotoolbox \
    -pix_fmt yuv420p \
    -b:v "${AVG_K_1080_H264}k" \
    -maxrate "${MAXRATE_K_1080_H264}k" \
    -bufsize "${BUFSIZE_K_1080_H264}k" \
    -profile:v high -level:v 4.1 \
    -colorspace bt709 -color_trc bt709 -color_primaries bt709 \
    -c:a copy -c:s copy \
    -max_muxing_queue_size 4096 \
    "$OUTPUT"
}

run_cpu_encode() {
  local filter_chain="$1"
  ffmpeg -hide_banner -nostdin -y \
    -probesize "$PROBE_SIZE" -analyzeduration "$ANALYZE_DUR" \
    -i "$INPUT" \
    -map 0:v:0 -map 0:a? -map 0:s? \
    -vf "$filter_chain" \
    -c:v libx264 \
    -preset "$CPU_PRESET" \
    -crf "$CRF_1080_H264" \
    -profile:v high -level:v 4.1 \
    -pix_fmt yuv420p \
    -x264-params "vbv-maxrate=${MAXRATE_K_1080_H264}:vbv-bufsize=${BUFSIZE_K_1080_H264}:keyint=240:min-keyint=24" \
    -colorspace bt709 -color_trc bt709 -color_primaries bt709 \
    -c:a copy -c:s copy \
    -max_muxing_queue_size 4096 \
    "$OUTPUT"
}

video_transfer="$(probe_video_transfer "$INPUT" || true)"
filter_chain="$(build_filter_chain "$video_transfer")"

use_hw=0
if [ "$ENCODER_MODE" = "hw" ] || { [ "$ENCODER_MODE" = "auto" ] && has_videotoolbox_encoder; }; then
  use_hw=1
fi

if [ "$use_hw" -eq 1 ]; then
  if ! run_hw_encode "$filter_chain"; then
    if [ "$ENCODER_MODE" = "auto" ]; then
      echo "VideoToolbox encode failed; retrying with CPU fallback" >&2
      run_cpu_encode "$filter_chain"
    else
      echo "Failed to encode with h264_videotoolbox" >&2
      exit 1
    fi
  fi
else
  run_cpu_encode "$filter_chain"
fi
