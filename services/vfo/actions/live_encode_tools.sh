#!/usr/bin/env bash
set -euo pipefail

# Shared live-logging and ffmpeg wrapping helpers for action scripts.
#
# Goals:
# - keep child ffmpeg jobs chatty enough to feel live in the terminal
# - line-buffer ffmpeg output when stdbuf is available
# - expose a compatibility lower_text helper for older action scripts

live_encode_has_passthrough_flag() {
  local arg=""

  for arg in "$@"; do
    case "$arg" in
      -encoders|-filters|-formats|-codecs|-devices|-buildconf|-version|-help|-h|-pix_fmts|-layouts|-protocols)
        return 0
        ;;
    esac
  done

  return 1
}

live_encode_lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

ffmpeg() {
  local stats_period="${VFO_FFMPEG_STATS_PERIOD:-2}"

  if live_encode_has_passthrough_flag "$@"; then
    command ffmpeg "$@"
    return $?
  fi

  if command -v stdbuf >/dev/null 2>&1; then
    env stdbuf -oL -eL ffmpeg -stats -stats_period "$stats_period" "$@"
    return $?
  fi

  command ffmpeg -stats -stats_period "$stats_period" "$@"
}

vfo_encode_step() {
  printf '%s\n' "$*"
}
