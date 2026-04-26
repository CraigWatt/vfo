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
  local live_output="${VFO_LIVE_OUTPUT:-auto}"
  live_output="$(live_encode_lower_text "$live_output")"

  case "$live_output" in
    stderr|pipe|log)
      printf '%s' stderr
      return 0
      ;;
    tty)
      if [ -w /dev/tty ] 2>/dev/null; then
        printf '%s' /dev/tty
        return 0
      fi
      printf '%s' stderr
      return 0
      ;;
    auto|"")
      ;;
    *)
      printf '%s' stderr
      return 0
      ;;
  esac

  if tty -s >/dev/null 2>&1 && [ -w /dev/tty ] 2>/dev/null; then
    printf '%s' /dev/tty
    return 0
  fi

  printf '%s' stderr
}

vfo_live_print() {
  if [ "$(vfo_live_output_device)" = "/dev/tty" ]; then
    printf '%s\n' "$*" >/dev/tty
  else
    printf '%s\n' "$*" >&2
  fi
}

vfo_drive_backed_tmpdir() {
  local output_path="$1"
  local temp_root="${VFO_TEMP_ROOT:-}"
  local parent_dir=""

  if [ -n "$temp_root" ]; then
    parent_dir="$temp_root"
  else
    parent_dir="$(dirname "$output_path")"
  fi

  mkdir -p "$parent_dir/.vfo-tmp"
  mktemp -d "${parent_dir%/}/.vfo-tmp/${VFO_TMP_PREFIX:-vfo}-XXXXXX"
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
  local use_script=0

  command_line="$(vfo_live_shell_quote "$@")"
  output_device="$(vfo_live_output_device)"
  if [ "$output_device" = "/dev/tty" ]; then
    printf 'VFO LIVE START: %s\n' "$label" >/dev/tty
    printf 'VFO LIVE COMMAND: %s\n' "$command_line" >/dev/tty
  else
    printf 'VFO LIVE START: %s\n' "$label" >&2
    printf 'VFO LIVE COMMAND: %s\n' "$command_line" >&2
  fi

  if [ "$output_device" = "/dev/tty" ] \
    && command -v script >/dev/null 2>&1 \
    && [ "${VFO_LIVE_USE_SCRIPT:-1}" = "1" ]; then
    use_script=1
  fi

  if [ "$use_script" -eq 1 ]; then
    script -q -e /dev/null "$@" >/dev/tty 2>&1
    exit_code=$?
  else
    command "$@" >&2
    exit_code=$?
  fi

  if [ "$output_device" = "/dev/tty" ]; then
    printf 'VFO LIVE END: %s (exit=%d)\n' "$label" "$exit_code" >/dev/tty
  else
    printf 'VFO LIVE END: %s (exit=%d)\n' "$label" "$exit_code" >&2
  fi
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
