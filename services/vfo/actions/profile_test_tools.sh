#!/usr/bin/env bash
set -euo pipefail

# Shared smoke-test helpers for truncating long assets during local validation.

profile_test_is_enabled() {
  local profile_test_active="${VFO_PROFILE_TEST_ACTIVE:-${SOURCE_TEST_ACTIVE:-false}}"

  case "$(printf '%s' "$profile_test_active" | tr '[:upper:]' '[:lower:]')" in
    1|true|yes|y|on)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

profile_test_trim_start() {
  local profile_test_start="${VFO_PROFILE_TEST_START:-${SOURCE_TEST_TRIM_START:-00:00:00}}"
  printf '%s' "$profile_test_start"
}

profile_test_trim_duration() {
  local profile_test_duration="${VFO_PROFILE_TEST_DURATION:-${SOURCE_TEST_TRIM_DURATION:-}}"

  if [ -z "$profile_test_duration" ] && profile_test_is_enabled; then
    profile_test_duration="00:02:00"
  fi

  printf '%s' "$profile_test_duration"
}

profile_test_prepare_input() {
  local input_file="$1"
  local workdir="$2"
  local profile_test_duration=""
  local profile_test_start=""
  local profile_test_output=""

  if ! profile_test_is_enabled; then
    printf '%s\n' "$input_file"
    return 0
  fi

  profile_test_duration="$(profile_test_trim_duration)"
  if [ -z "$profile_test_duration" ]; then
    printf '%s\n' "$input_file"
    return 0
  fi

  profile_test_start="$(profile_test_trim_start)"
  profile_test_output="${workdir}/profile_test_trimmed_input.mkv"

  echo "PROFILE TEST: trimming smoke-test input start=${profile_test_start} duration=${profile_test_duration}" >&2
  if ! ffmpeg -hide_banner -nostdin -y \
    -ss "$profile_test_start" -t "$profile_test_duration" \
    -i "$input_file" \
    -map 0 \
    -c copy \
    -avoid_negative_ts make_zero \
    "$profile_test_output" >/dev/null 2>&1; then
    echo "PROFILE TEST ERROR: failed to trim smoke-test input" >&2
    return 1
  fi

  if [ ! -s "$profile_test_output" ]; then
    echo "PROFILE TEST ERROR: trimmed smoke-test input is missing or empty: $profile_test_output" >&2
    return 1
  fi

  printf '%s\n' "$profile_test_output"
}
