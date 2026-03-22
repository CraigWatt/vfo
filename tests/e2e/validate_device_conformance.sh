#!/usr/bin/env bash
set -euo pipefail

# Validate encoded media against conservative device capability targets.
#
# Usage:
#   validate_device_conformance.sh <target_id> <media_file>
#
# Targets:
#   roku_express_1080
#   roku_4k
#   fire_tv_stick_lite_1080
#   fire_tv_stick_4k
#   fire_tv_stick_4k_max
#   chromecast_google_tv_hd
#   chromecast_google_tv_4k
#   apple_tv_hd
#   apple_tv_4k
#
# Notes:
# - This is a predictive conformance check (not real playback telemetry).
# - Caps are intentionally conservative for broad compatibility.

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <target_id> <media_file>"
  exit 1
fi

TARGET="$1"
MEDIA="$2"

if [ ! -f "$MEDIA" ]; then
  echo "[conformance] ERROR: media file not found: $MEDIA"
  exit 1
fi

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "[conformance] ERROR: missing command: $1"
    exit 1
  }
}

need ffprobe

contains_token() {
  local needle="$1"
  local haystack="$2"
  case " $haystack " in
    *" $needle "*) return 0 ;;
    *) return 1 ;;
  esac
}

rational_to_float() {
  local value="$1"
  if [ -z "$value" ] || [ "$value" = "0/0" ]; then
    printf '0'
    return 0
  fi

  if printf '%s' "$value" | grep -q '/'; then
    awk -F/ '{
      if ($2 == 0) { printf "0"; }
      else { printf "%.6f", $1 / $2; }
    }' <<EOF
$value
EOF
  else
    printf '%s' "$value"
  fi
}

probe_video_field() {
  local field="$1"
  ffprobe -v error -select_streams v:0 \
    -show_entries "stream=${field}" \
    -of default=nk=1:nw=1 "$MEDIA" | tr -d '\r\n'
}

probe_audio_rows() {
  ffprobe -v error -select_streams a \
    -show_entries stream=index,codec_name,channels \
    -of csv=p=0 "$MEDIA" || true
}

MAX_W=0
MAX_H=0
MAX_FPS=0
MAX_AUDIO_CHANNELS=0
ALLOW_HDR=0
VIDEO_CODECS=""
PIXFMTS=""
AUDIO_CODECS=""
TARGET_DESC=""

case "$TARGET" in
  roku_express_1080)
    TARGET_DESC="Roku Express (1080p baseline)"
    MAX_W=1920
    MAX_H=1080
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=6
    ALLOW_HDR=0
    VIDEO_CODECS="h264"
    PIXFMTS="yuv420p"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  roku_4k)
    TARGET_DESC="Roku 4K (conservative baseline)"
    MAX_W=3840
    MAX_H=2160
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=8
    ALLOW_HDR=1
    VIDEO_CODECS="hevc h264"
    PIXFMTS="yuv420p yuv420p10le p010le"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  fire_tv_stick_lite_1080)
    TARGET_DESC="Fire TV Stick Lite (1080p baseline)"
    MAX_W=1920
    MAX_H=1080
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=6
    ALLOW_HDR=0
    VIDEO_CODECS="h264 hevc"
    PIXFMTS="yuv420p"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  fire_tv_stick_4k)
    TARGET_DESC="Fire TV Stick 4K (conservative baseline)"
    MAX_W=3840
    MAX_H=2160
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=8
    ALLOW_HDR=1
    VIDEO_CODECS="hevc h264"
    PIXFMTS="yuv420p yuv420p10le p010le"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  fire_tv_stick_4k_max)
    TARGET_DESC="Fire TV Stick 4K Max (conservative baseline)"
    MAX_W=3840
    MAX_H=2160
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=8
    ALLOW_HDR=1
    VIDEO_CODECS="hevc h264 av1"
    PIXFMTS="yuv420p yuv420p10le p010le"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  chromecast_google_tv_hd)
    TARGET_DESC="Chromecast with Google TV HD (1080p baseline)"
    MAX_W=1920
    MAX_H=1080
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=6
    ALLOW_HDR=0
    VIDEO_CODECS="h264 hevc vp9"
    PIXFMTS="yuv420p"
    AUDIO_CODECS="aac ac3 eac3 mp3 opus"
    ;;
  chromecast_google_tv_4k)
    TARGET_DESC="Chromecast with Google TV 4K (conservative baseline)"
    MAX_W=3840
    MAX_H=2160
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=8
    ALLOW_HDR=1
    VIDEO_CODECS="hevc h264 vp9 av1"
    PIXFMTS="yuv420p yuv420p10le p010le"
    AUDIO_CODECS="aac ac3 eac3 mp3 opus"
    ;;
  apple_tv_hd)
    TARGET_DESC="Apple TV HD (1080p baseline)"
    MAX_W=1920
    MAX_H=1080
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=6
    ALLOW_HDR=0
    VIDEO_CODECS="h264 hevc"
    PIXFMTS="yuv420p"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  apple_tv_4k)
    TARGET_DESC="Apple TV 4K (conservative baseline)"
    MAX_W=3840
    MAX_H=2160
    MAX_FPS=60
    MAX_AUDIO_CHANNELS=8
    ALLOW_HDR=1
    VIDEO_CODECS="hevc h264"
    PIXFMTS="yuv420p yuv420p10le p010le"
    AUDIO_CODECS="aac ac3 eac3 mp3"
    ;;
  *)
    echo "[conformance] ERROR: unknown target_id: $TARGET"
    exit 1
    ;;
esac

echo "[conformance] target=${TARGET} (${TARGET_DESC}) media=${MEDIA}"

fail_count=0

conformance_fail() {
  fail_count=$((fail_count + 1))
  printf '[conformance] FAIL: %s\n' "$*"
}

conformance_ok() {
  printf '[conformance] OK: %s\n' "$*"
}

video_codec="$(probe_video_field codec_name)"
video_width="$(probe_video_field width)"
video_height="$(probe_video_field height)"
video_pix_fmt="$(probe_video_field pix_fmt)"
video_transfer="$(probe_video_field color_transfer)"
video_fps_raw="$(probe_video_field avg_frame_rate)"
video_fps="$(rational_to_float "$video_fps_raw")"

[ -n "$video_codec" ] || conformance_fail "missing primary video stream"

if [ -n "$video_codec" ] && contains_token "$video_codec" "$VIDEO_CODECS"; then
  conformance_ok "video codec '${video_codec}' within allowed set (${VIDEO_CODECS})"
else
  conformance_fail "video codec '${video_codec}' not allowed (${VIDEO_CODECS})"
fi

if [ -n "$video_width" ] && [ -n "$video_height" ] && [ "$video_width" -le "$MAX_W" ] && [ "$video_height" -le "$MAX_H" ]; then
  conformance_ok "resolution ${video_width}x${video_height} <= ${MAX_W}x${MAX_H}"
else
  conformance_fail "resolution ${video_width}x${video_height} exceeds ${MAX_W}x${MAX_H}"
fi

if awk -v f="$video_fps" -v m="$MAX_FPS" 'BEGIN { exit !(f <= m + 0.0001) }'; then
  conformance_ok "frame rate ${video_fps}fps <= ${MAX_FPS}fps"
else
  conformance_fail "frame rate ${video_fps}fps exceeds ${MAX_FPS}fps"
fi

if contains_token "$video_pix_fmt" "$PIXFMTS"; then
  conformance_ok "pixel format '${video_pix_fmt}' allowed (${PIXFMTS})"
else
  conformance_fail "pixel format '${video_pix_fmt}' not in allowed set (${PIXFMTS})"
fi

if [ "$ALLOW_HDR" -eq 0 ]; then
  case "$video_transfer" in
    smpte2084|arib-std-b67)
      conformance_fail "HDR transfer '${video_transfer}' not allowed for this target"
      ;;
    *)
      conformance_ok "transfer '${video_transfer:-unknown}' accepted for SDR target"
      ;;
  esac
else
  conformance_ok "HDR/SDR transfer accepted for target (${video_transfer:-unknown})"
fi

audio_rows="$(probe_audio_rows)"
if [ -z "$audio_rows" ]; then
  conformance_ok "no audio streams detected"
else
  while IFS=, read -r audio_index audio_codec audio_channels; do
    [ -n "$audio_codec" ] || continue

    if contains_token "$audio_codec" "$AUDIO_CODECS"; then
      conformance_ok "audio stream #${audio_index} codec '${audio_codec}' allowed"
    else
      conformance_fail "audio stream #${audio_index} codec '${audio_codec}' not allowed (${AUDIO_CODECS})"
    fi

    if [ -n "$audio_channels" ] && [ "$audio_channels" -le "$MAX_AUDIO_CHANNELS" ]; then
      conformance_ok "audio stream #${audio_index} channels ${audio_channels} <= ${MAX_AUDIO_CHANNELS}"
    else
      conformance_fail "audio stream #${audio_index} channels ${audio_channels} exceeds ${MAX_AUDIO_CHANNELS}"
    fi
  done <<EOF
$audio_rows
EOF
fi

if [ "$fail_count" -gt 0 ]; then
  echo "[conformance] RESULT: FAIL (${fail_count} issue(s))"
  exit 1
fi

echo "[conformance] RESULT: PASS"
