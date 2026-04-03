#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="${ROOT_DIR}/tests/e2e/.tmp_device_conformance"
FIXTURES_DIR="${TMP_DIR}/fixtures"
OUTPUTS_DIR="${TMP_DIR}/outputs"
# shellcheck source=lib/e2e_toolchain_report.sh
. "${ROOT_DIR}/tests/e2e/lib/e2e_toolchain_report.sh"

ASSET_MODE="${VFO_E2E_ASSET_MODE:-auto}" # auto|local|synthetic
ASSETS_DIR="${VFO_E2E_ASSETS_DIR:-${ROOT_DIR}/tests/e2e/assets/open-source}"
CLIP_DURATION="${VFO_E2E_CLIP_DURATION:-2}"
MAX_SEEDS="${VFO_E2E_MAX_SEEDS:-1}"
KEEP_TMP="${VFO_E2E_KEEP_TMP:-0}"

ACTION_HEVC_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_profile.sh"
ACTION_HEVC_1080="${ROOT_DIR}/services/vfo/actions/transcode_hevc_1080_profile.sh"
# Explicit conversion lane for SDR-target checks.
ACTION_H264_1080_HDR_TO_SDR="${ROOT_DIR}/services/vfo/actions/transcode_h264_1080_hdr_to_sdr_profile.sh"
VALIDATOR="${ROOT_DIR}/tests/e2e/validate_device_conformance.sh"

log() {
  printf '[device-e2e] %s\n' "$*"
}

fail() {
  printf '[device-e2e] ERROR: %s\n' "$*" >&2
  exit 1
}

need() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing command: $1"
}

cleanup() {
  if [ "$KEEP_TMP" = "1" ]; then
    log "Keeping temporary data at: ${TMP_DIR}"
    return 0
  fi
  rm -rf "$TMP_DIR"
}

assert_positive_int() {
  local value="$1"
  local name="$2"
  case "$value" in
    ''|*[!0-9]*)
      fail "${name} must be a positive integer (got: ${value})"
      ;;
    0)
      fail "${name} must be greater than zero"
      ;;
  esac
}

is_video_file() {
  local candidate="$1"
  case "$candidate" in
    *.mkv|*.mp4|*.mov|*.m4v|*.m2ts|*.ts|*.avi|*.webm|\
    *.MKV|*.MP4|*.MOV|*.M4V|*.M2TS|*.TS|*.AVI|*.WEBM)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

collect_video_assets() {
  local list_file="$1"
  local candidate
  : > "$list_file"

  [ -d "$ASSETS_DIR" ] || return 0

  while IFS= read -r candidate; do
    if is_video_file "$candidate"; then
      printf '%s\n' "$candidate" >> "$list_file"
    fi
  done < <(find "$ASSETS_DIR" -type f 2>/dev/null | LC_ALL=C sort)
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

run_action() {
  local script="$1"
  local input="$2"
  local output="$3"

  [ -x "$script" ] || fail "Action script not executable: $script"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  env \
    VFO_ENCODER_MODE=cpu \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    CRF_1080=30 \
    CRF_1080_H264=24 \
    AVG_K=4000 \
    MAXRATE_K=6000 \
    BUFSIZE_K=12000 \
    AVG_K_1080=2500 \
    MAXRATE_K_1080=3500 \
    BUFSIZE_K_1080=7000 \
    AVG_K_1080_H264=2200 \
    MAXRATE_K_1080_H264=3200 \
    BUFSIZE_K_1080_H264=6400 \
    bash "$script" "$input" "$output"

  [ -s "$output" ] || fail "Action output missing: $output"
  ffprobe -v error "$output" >/dev/null 2>&1 || fail "ffprobe cannot read action output: $output"
}

run_seed_suite() {
  local seed_index="$1"
  local seed_input="$2"
  local in_2160="${FIXTURES_DIR}/seed_${seed_index}_input_2160.mkv"
  local out_hevc_4k="${OUTPUTS_DIR}/seed_${seed_index}_hevc_4k.mkv"
  local out_hevc_1080="${OUTPUTS_DIR}/seed_${seed_index}_hevc_1080.mkv"
  local out_h264_1080_sdr="${OUTPUTS_DIR}/seed_${seed_index}_h264_1080_sdr.mkv"

  WEB_APP_SELECTED_ASSET="$(basename "$seed_input")"
  WEB_APP_ASSET_STATUS="Complete"

  log "Building fixture for seed #${seed_index}: ${seed_input}"
  create_fixture_from_input "$seed_input" 3840 2160 "$in_2160"

  log "Running profile actions for seed #${seed_index}"
  run_action "$ACTION_HEVC_4K" "$in_2160" "$out_hevc_4k"
  run_action "$ACTION_HEVC_1080" "$in_2160" "$out_hevc_1080"
  run_action "$ACTION_H264_1080_HDR_TO_SDR" "$in_2160" "$out_h264_1080_sdr"

  log "Validating device conformance for seed #${seed_index}"
  bash "$VALIDATOR" roku_express_1080 "$out_h264_1080_sdr"
  bash "$VALIDATOR" fire_tv_stick_lite_1080 "$out_h264_1080_sdr"
  bash "$VALIDATOR" chromecast_google_tv_hd "$out_h264_1080_sdr"
  bash "$VALIDATOR" apple_tv_hd "$out_h264_1080_sdr"

  bash "$VALIDATOR" roku_4k "$out_hevc_4k"
  bash "$VALIDATOR" fire_tv_stick_4k "$out_hevc_4k"
  bash "$VALIDATOR" fire_tv_stick_4k_max "$out_hevc_4k"
  bash "$VALIDATOR" chromecast_google_tv_4k "$out_hevc_4k"
  bash "$VALIDATOR" apple_tv_4k "$out_hevc_4k"

  log "Seed #${seed_index} conformance checks passed"
}

run_synthetic_seed() {
  local seed_index="$1"
  local synthetic_input="${FIXTURES_DIR}/seed_${seed_index}_synthetic_input_2160.mkv"
  WEB_APP_SELECTED_ASSET="synthetic_seed_${seed_index}.mkv"
  WEB_APP_ASSET_STATUS="Complete"
  create_synthetic_fixture 3840 2160 "$synthetic_input"
  run_seed_suite "$seed_index" "$synthetic_input"
}

run_local_suite() {
  local seed_list="${TMP_DIR}/seed_assets.txt"
  local processed=0
  local seed_asset=""

  collect_video_assets "$seed_list"

  while IFS= read -r seed_asset; do
    [ -n "$seed_asset" ] || continue
    if [ "$processed" -ge "$MAX_SEEDS" ]; then
      break
    fi
    processed=$((processed + 1))
    run_seed_suite "$processed" "$seed_asset"
  done < "$seed_list"

  if [ "$processed" -eq 0 ]; then
    if [ "$ASSET_MODE" = "auto" ]; then
      log "No local asset found in ${ASSETS_DIR}; using synthetic device conformance seed"
      run_synthetic_seed 1
      printf '%s\n' "$WEB_APP_SELECTED_ASSET" > "$seed_list"
      processed=1
    else
      fail "ASSET_MODE=local but no video assets found in ${ASSETS_DIR}"
    fi
  fi

  log "Processed ${processed} seed asset(s)"
}

main() {
  need ffmpeg
  need ffprobe
  e2e_write_toolchain_report \
    "$ROOT_DIR" \
    "run_device_conformance_e2e" \
    ffmpeg ffprobe
  [ -x "$ACTION_HEVC_4K" ] || fail "Missing action script: $ACTION_HEVC_4K"
  [ -x "$ACTION_HEVC_1080" ] || fail "Missing action script: $ACTION_HEVC_1080"
  [ -x "$ACTION_H264_1080_HDR_TO_SDR" ] || fail "Missing action script: $ACTION_H264_1080_HDR_TO_SDR"
  [ -x "$VALIDATOR" ] || fail "Missing conformance validator: $VALIDATOR"

  assert_positive_int "$MAX_SEEDS" "VFO_E2E_MAX_SEEDS"

  rm -rf "$TMP_DIR"
  mkdir -p "$FIXTURES_DIR" "$OUTPUTS_DIR"
  trap cleanup EXIT

  WEB_APP_DASHBOARD_JSON="$(e2e_reports_dir "$ROOT_DIR")/run_device_conformance_e2e_web_app.json"
  WEB_APP_SELECTED_ASSET=""
  WEB_APP_ASSET_STATUS="Complete"
  WEB_APP_ASSET_MANIFEST_FILE="${ROOT_DIR}/tests/e2e/assets/open-source/mezzanine-source-set.txt"
  WEB_APP_ASSET_LIST_FILE="${TMP_DIR}/seed_assets.txt"

  log "asset_mode=${ASSET_MODE} assets_dir=${ASSETS_DIR} clip_duration=${CLIP_DURATION}s max_seeds=${MAX_SEEDS}"

  case "$ASSET_MODE" in
    auto|local)
      run_local_suite
      WEB_APP_ASSET_LIST_FILE="${TMP_DIR}/seed_assets.txt"
      ;;
    synthetic)
      run_synthetic_seed 1
      printf '%s\n' "$WEB_APP_SELECTED_ASSET" > "$WEB_APP_ASSET_LIST_FILE"
      log "Processed 1 seed asset(s)"
      ;;
    *)
      fail "Unsupported VFO_E2E_ASSET_MODE='${ASSET_MODE}' (expected: auto|local|synthetic)"
      ;;
  esac

  e2e_write_web_app_dashboard \
    "$WEB_APP_DASHBOARD_JSON" \
    "device-conformance" \
    "Device conformance" \
    "Pipeline: Device Conformance" \
    "Run: device conformance replay" \
    "Device conformance e2e" \
    "run_device_conformance_e2e" \
    "" \
    "${WEB_APP_SELECTED_ASSET:-seed_1_input_2160.mkv}" \
    "device_conformance" \
    "$WEB_APP_ASSET_STATUS" \
    "$WEB_APP_ASSET_MANIFEST_FILE" \
    "$WEB_APP_ASSET_LIST_FILE"

  log "All device conformance e2e checks passed"
}

main "$@"
