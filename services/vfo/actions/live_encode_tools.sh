#!/usr/bin/env bash
set -euo pipefail

# Shared live-logging and ffmpeg wrapping helpers for action scripts.
#
# Goals:
# - keep child ffmpeg jobs chatty enough to feel live in the terminal
# - print the exact command before execution
# - prefer a pseudo-tty when available so child output stays attached live
# - line-buffer ffmpeg output when stdbuf is available
# - expose a compatibility lower_text helper for older action scripts

vfo_live_shell_quote() {
  local arg=""
  local quoted=""

  for arg in "$@"; do
    quoted="${quoted} $(printf '%q' "$arg")"
  done

  printf '%s' "${quoted# }"
}

vfo_live_output_device() {
  if tty -s >/dev/null 2>&1 && [ -w /dev/tty ] 2>/dev/null; then
    printf '%s' /dev/tty
    return 0
  fi

  printf '%s' /dev/stderr
}

vfo_live_print() {
  local output_device=""

  output_device="$(vfo_live_output_device)"
  printf '%s\n' "$*" >"$output_device"
}

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

vfo_live_run_command() {
  local label="$1"
  shift
  local command_line=""
  local exit_code=0
  local output_device=""

  command_line="$(vfo_live_shell_quote "$@")"
  output_device="$(vfo_live_output_device)"
  printf 'VFO LIVE START: %s\n' "$label" >"$output_device"
  printf 'VFO LIVE COMMAND: %s\n' "$command_line" >"$output_device"

  if command -v script >/dev/null 2>&1 && [ "${VFO_LIVE_USE_SCRIPT:-1}" = "1" ]; then
    script -q -e /dev/null "$@" >"$output_device" 2>&1
    exit_code=$?
  elif command -v stdbuf >/dev/null 2>&1; then
    env stdbuf -oL -eL "$@" >"$output_device" 2>&1
    exit_code=$?
  else
    "$@" >"$output_device" 2>&1
    exit_code=$?
  fi

  printf 'VFO LIVE END: %s (exit=%d)\n' "$label" "$exit_code" >"$output_device"
  return "$exit_code"
}

ffmpeg() {
  local stats_period="${VFO_FFMPEG_STATS_PERIOD:-2}"
  local -a ffmpeg_args=()

  if live_encode_has_passthrough_flag "$@"; then
    command ffmpeg "$@"
    return $?
  fi

  ffmpeg_args=(-stats -stats_period "$stats_period" "$@")
  vfo_live_run_command "ffmpeg" ffmpeg "${ffmpeg_args[@]}"
}

vfo_encode_step() {
  vfo_live_print "VFO STEP: $*"
}

vfo_encode_step_start() {
  vfo_live_print "VFO STEP START: $*"
}

vfo_encode_step_end() {
  vfo_live_print "VFO STEP END: $*"
}
