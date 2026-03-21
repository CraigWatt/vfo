#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="${ROOT_DIR}/tests/e2e/.tmp"
FIXTURES_DIR="${TMP_DIR}/fixtures"
OUTPUTS_DIR="${TMP_DIR}/outputs"

ASSET_MODE="${VFO_E2E_ASSET_MODE:-auto}" # auto|local|synthetic
ASSETS_DIR="${VFO_E2E_ASSETS_DIR:-${ROOT_DIR}/tests/e2e/assets/open-source}"
CLIP_DURATION="${VFO_E2E_CLIP_DURATION:-2}"
KEEP_TMP="${VFO_E2E_KEEP_TMP:-0}"

ACTION_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_profile.sh"
ACTION_1080="${ROOT_DIR}/services/vfo/actions/transcode_hevc_1080_profile.sh"

log() {
  printf '[e2e] %s\n' "$*"
}

fail() {
  printf '[e2e] ERROR: %s\n' "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing command: $1"
}

cleanup() {
  if [ "$KEEP_TMP" = "1" ]; then
    log "Keeping temporary data at: ${TMP_DIR}"
    return 0
  fi
  rm -rf "$TMP_DIR"
}

assert_equals() {
  local actual="$1"
  local expected="$2"
  local message="$3"
  if [ "$actual" != "$expected" ]; then
    fail "${message}. expected='${expected}' actual='${actual}'"
  fi
}

assert_int_lte() {
  local value="$1"
  local max="$2"
  local message="$3"
  if [ "$value" -gt "$max" ]; then
    fail "${message}. value=${value} max=${max}"
  fi
}

find_first_video() {
  local candidate
  while IFS= read -r -d '' candidate; do
    case "$candidate" in
      *.mkv|*.mp4|*.mov|*.m4v|*.m2ts|*.ts|*.avi|*.webm|\
      *.MKV|*.MP4|*.MOV|*.M4V|*.M2TS|*.TS|*.AVI|*.WEBM)
        printf '%s\n' "$candidate"
        return 0
        ;;
    esac
  done < <(find "$ASSETS_DIR" -type f -print0 2>/dev/null)
  return 1
}

create_fixture_from_input() {
  local input="$1"
  local width="$2"
  local height="$3"
  local output="$4"

  ffmpeg -hide_banner -nostdin -y \
    -ss 0 -t "$CLIP_DURATION" \
    -i "$input" \
    -map 0:v:0 -map 0:a? \
    -vf "scale=${width}:${height}:force_original_aspect_ratio=decrease,pad=${width}:${height}:(ow-iw)/2:(oh-ih)/2" \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a aac -b:a 128k \
    "$output" >/dev/null 2>&1
}

create_synthetic_fixture() {
  local width="$1"
  local height="$2"
  local output="$3"

  ffmpeg -hide_banner -nostdin -y \
    -f lavfi -i "testsrc2=size=${width}x${height}:rate=24" \
    -f lavfi -i "sine=frequency=1000:sample_rate=48000" \
    -t "$CLIP_DURATION" \
    -map 0:v:0 -map 1:a:0 \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a aac -b:a 128k \
    "$output" >/dev/null 2>&1
}

probe_video_codec() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=codec_name \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

probe_video_height() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=height \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

probe_audio_count() {
  ffprobe -v error -select_streams a \
    -show_entries stream=index \
    -of csv=p=0 "$1" | wc -l | tr -d ' '
}

run_action_assertions() {
  local action_name="$1"
  local action_script="$2"
  local input="$3"
  local output="$4"
  local max_height="$5"

  [ -x "$action_script" ] || fail "Action script is not executable: $action_script"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  local input_audio_count
  local output_audio_count
  local codec
  local height

  input_audio_count="$(probe_audio_count "$input")"

  env \
    VFO_ENCODER_MODE=cpu \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    CRF_1080=30 \
    AVG_K=4000 \
    MAXRATE_K=6000 \
    BUFSIZE_K=12000 \
    AVG_K_1080=2500 \
    MAXRATE_K_1080=3500 \
    BUFSIZE_K_1080=7000 \
    bash "$action_script" "$input" "$output"

  [ -s "$output" ] || fail "Output file missing: $output"
  ffprobe -v error "$output" >/dev/null 2>&1 || fail "ffprobe cannot read output: $output"

  codec="$(probe_video_codec "$output")"
  height="$(probe_video_height "$output")"
  output_audio_count="$(probe_audio_count "$output")"

  assert_equals "$codec" "hevc" "${action_name} should output HEVC"
  assert_int_lte "$height" "$max_height" "${action_name} output exceeds max height"
  assert_equals "$output_audio_count" "$input_audio_count" "${action_name} changed audio stream count"

  log "${action_name} passed (codec=${codec}, height=${height}, audio_streams=${output_audio_count})"
}

build_fixtures() {
  local seed_asset=""

  mkdir -p "$FIXTURES_DIR" "$OUTPUTS_DIR"

  case "$ASSET_MODE" in
    auto)
      if seed_asset="$(find_first_video)"; then
        log "Using local open-source asset seed: ${seed_asset}"
        create_fixture_from_input "$seed_asset" 1920 1080 "${FIXTURES_DIR}/mezzanine_1080.mkv"
        create_fixture_from_input "$seed_asset" 3840 2160 "${FIXTURES_DIR}/mezzanine_2160.mkv"
      else
        log "No local asset found in ${ASSETS_DIR}; generating synthetic CI fixtures"
        create_synthetic_fixture 1920 1080 "${FIXTURES_DIR}/mezzanine_1080.mkv"
        create_synthetic_fixture 3840 2160 "${FIXTURES_DIR}/mezzanine_2160.mkv"
      fi
      ;;
    local)
      seed_asset="$(find_first_video || true)"
      [ -n "$seed_asset" ] || fail "ASSET_MODE=local but no video assets found in ${ASSETS_DIR}"
      log "Using local open-source asset seed: ${seed_asset}"
      create_fixture_from_input "$seed_asset" 1920 1080 "${FIXTURES_DIR}/mezzanine_1080.mkv"
      create_fixture_from_input "$seed_asset" 3840 2160 "${FIXTURES_DIR}/mezzanine_2160.mkv"
      ;;
    synthetic)
      log "Using synthetic fixtures (no local assets required)"
      create_synthetic_fixture 1920 1080 "${FIXTURES_DIR}/mezzanine_1080.mkv"
      create_synthetic_fixture 3840 2160 "${FIXTURES_DIR}/mezzanine_2160.mkv"
      ;;
    *)
      fail "Unsupported VFO_E2E_ASSET_MODE='${ASSET_MODE}' (expected: auto|local|synthetic)"
      ;;
  esac
}

main() {
  require_command ffmpeg
  require_command ffprobe
  [ -f "$ACTION_4K" ] || fail "Missing action script: $ACTION_4K"
  [ -f "$ACTION_1080" ] || fail "Missing action script: $ACTION_1080"

  rm -rf "$TMP_DIR"
  mkdir -p "$FIXTURES_DIR" "$OUTPUTS_DIR"
  trap cleanup EXIT

  log "asset_mode=${ASSET_MODE} assets_dir=${ASSETS_DIR} clip_duration=${CLIP_DURATION}s"
  build_fixtures

  run_action_assertions \
    "hevc_1080_profile_action" \
    "$ACTION_1080" \
    "${FIXTURES_DIR}/mezzanine_1080.mkv" \
    "${OUTPUTS_DIR}/profile_1080_output.mkv" \
    1080

  run_action_assertions \
    "hevc_4k_profile_action" \
    "$ACTION_4K" \
    "${FIXTURES_DIR}/mezzanine_2160.mkv" \
    "${OUTPUTS_DIR}/profile_4k_output.mkv" \
    2160

  log "All e2e profile action checks passed"
}

main "$@"
