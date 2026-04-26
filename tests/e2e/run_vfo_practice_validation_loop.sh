#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

PRACTICE_ROOT="${VFO_PRACTICE_ROOT:-/Volumes/Mitchum/vfo_practice}"
SELECTED_ASSET="${VFO_PRACTICE_ASSET:-}"
PROFILE_PACK="${VFO_PRACTICE_PROFILE_PACK:-craigstreamy_hevc_smart_eng_sub_aggressive_vmaf}"
VFO_BIN="${VFO_PRACTICE_VFO_BIN:-vfo}"
VFO_PATH_PREFIX=""
VFO_COMMAND_NAME="$VFO_BIN"
if [[ "$VFO_BIN" == */* ]]; then
  VFO_PATH_PREFIX="$(cd "$(dirname "$VFO_BIN")" && pwd)"
  VFO_COMMAND_NAME="$(basename "$VFO_BIN")"
fi
VFO_ACTIONS_PATH_PREFIX="${VFO_PRACTICE_ACTIONS_DIR:-${ROOT_DIR}/services/vfo/actions}"
PROFILE_COMMAND="${VFO_PRACTICE_PROFILE_COMMAND:-${VFO_COMMAND_NAME} run}"
REPORT_ROOT="${VFO_PRACTICE_REPORT_ROOT:-${ROOT_DIR}/tests/e2e/.reports/vfo-practice}"
POLL_SECONDS="${VFO_PRACTICE_POLL_SECONDS:-21600}"
CREATE_ISSUES="${VFO_PRACTICE_CREATE_ISSUES:-0}"
ISSUE_WEAK_SIGNALS="${VFO_PRACTICE_ISSUE_WEAK_SIGNALS:-0}"
ISSUE_LABEL="${VFO_PRACTICE_ISSUE_LABEL:-codex-candidate}"
GITHUB_REPO="${VFO_PRACTICE_GITHUB_REPO:-CraigWatt/vfo}"
SOURCE_TEST_ACTIVE_VALUE="${SOURCE_TEST_ACTIVE:-true}"
SOURCE_TEST_TRIM_START_VALUE="${SOURCE_TEST_TRIM_START:-00:00:00}"
SOURCE_TEST_TRIM_DURATION_VALUE="${SOURCE_TEST_TRIM_DURATION:-00:02:00}"
VFO_ASSUME_YES_VALUE="${VFO_ASSUME_YES:-1}"
VFO_LIVE_OUTPUT_VALUE="${VFO_LIVE_OUTPUT:-stderr}"

MODE="once"
LAST_FINGERPRINT=""

usage() {
  cat <<'USAGE'
Usage:
  tests/e2e/run_vfo_practice_validation_loop.sh [--once|--watch] [options]

Options:
  --once                 Run one validation cycle (default).
  --watch                Poll for newest media changes and run when they change.
  --practice-root PATH   Practice root. Default: /Volumes/Mitchum/vfo_practice
  --asset PATH           Selected asset path for issue/report context.
  --profile-command CMD  VFO command to run after doctor/status. Default: vfo run
  --create-issues        Create or update deduped GitHub issues on findings.
  --help                 Show this help.

Useful environment:
  VFO_PRACTICE_CREATE_ISSUES=1
  VFO_PRACTICE_VFO_BIN=/usr/local/bin/vfo
  VFO_PRACTICE_ACTIONS_DIR=/Users/craigwatt/localProjects/vfo/services/vfo/actions
  VFO_PRACTICE_PROFILE_COMMAND="vfo run"
  VFO_PRACTICE_PROFILE_COMMAND="yes y | vfo profiles"
  SOURCE_TEST_ACTIVE=true
  SOURCE_TEST_TRIM_DURATION=00:02:00
  VFO_ASSUME_YES=1
  VFO_LIVE_OUTPUT=stderr

Reports:
  tests/e2e/.reports/vfo-practice/latest
USAGE
}

log() {
  printf '[vfo-practice] %s\n' "$*"
}

warn() {
  printf '[vfo-practice] WARN: %s\n' "$*" >&2
}

fail() {
  printf '[vfo-practice] ERROR: %s\n' "$*" >&2
  exit 1
}

parse_args() {
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --once)
        MODE="once"
        shift
        ;;
      --watch)
        MODE="watch"
        shift
        ;;
      --practice-root)
        [ "$#" -ge 2 ] || fail "--practice-root requires a path"
        PRACTICE_ROOT="$2"
        shift 2
        ;;
      --asset)
        [ "$#" -ge 2 ] || fail "--asset requires a path"
        SELECTED_ASSET="$2"
        shift 2
        ;;
      --profile-command)
        [ "$#" -ge 2 ] || fail "--profile-command requires a command"
        PROFILE_COMMAND="$2"
        shift 2
        ;;
      --create-issues)
        CREATE_ISSUES="1"
        shift
        ;;
      --help|-h)
        usage
        exit 0
        ;;
      *)
        fail "Unknown option: $1"
        ;;
    esac
  done
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing command: $1"
}

stat_mtime() {
  if stat -f '%m' "$1" >/dev/null 2>&1; then
    stat -f '%m' "$1"
  else
    stat -c '%Y' "$1"
  fi
}

stat_size() {
  if stat -f '%z' "$1" >/dev/null 2>&1; then
    stat -f '%z' "$1"
  else
    stat -c '%s' "$1"
  fi
}

find_latest_asset() {
  local search_root="${PRACTICE_ROOT}/mezzanine"

  if [ -n "$SELECTED_ASSET" ]; then
    [ -f "$SELECTED_ASSET" ] || fail "Selected asset does not exist: $SELECTED_ASSET"
    printf '%s\n' "$SELECTED_ASSET"
    return 0
  fi

  [ -d "$search_root" ] || fail "Practice mezzanine root does not exist: $search_root"

  find "$search_root" -type f \( \
    -iname '*.mkv' -o \
    -iname '*.mp4' -o \
    -iname '*.m2ts' -o \
    -iname '*.ts' \
  \) -print0 |
    while IFS= read -r -d '' asset; do
      printf '%s\t%s\t%s\n' "$(stat_mtime "$asset")" "$(stat_size "$asset")" "$asset"
    done |
    sort -nr |
    awk -F '\t' 'NR == 1 {print $3}'
}

asset_fingerprint() {
  local asset="$1"
  if [ -z "$asset" ]; then
    printf 'no-asset\n'
    return 0
  fi
  printf '%s:%s:%s\n' "$asset" "$(stat_mtime "$asset")" "$(stat_size "$asset")"
}

write_asset_metadata() {
  local asset="$1"
  local output_file="$2"

  {
    echo "asset_path=${asset}"
    if [ -n "$asset" ] && [ -f "$asset" ]; then
      echo "asset_mtime=$(stat_mtime "$asset")"
      echo "asset_size_bytes=$(stat_size "$asset")"
      echo
      echo "[ffprobe_video_stream]"
      if command -v ffprobe >/dev/null 2>&1; then
        ffprobe -v error \
          -select_streams v:0 \
          -show_entries stream=codec_name,width,height,bits_per_raw_sample,color_space,color_transfer,color_primaries:stream_side_data=side_data_type,dv_profile,dv_level,rpu_present_flag,el_present_flag,bl_present_flag,dv_bl_signal_compatibility_id \
          -of default=nw=1 \
          "$asset" 2>&1 || true
      else
        echo "ffprobe unavailable"
      fi
    else
      echo "asset_status=missing"
    fi
  } > "$output_file"
}

run_with_log() {
  local stage="$1"
  local command_text="$2"
  local log_file="$3"
  local status_file="${log_file}.status"
  local status

  log "START ${stage}: ${command_text}"
  set +e
  (
    cd "$PRACTICE_ROOT"
    env \
      SOURCE_TEST_ACTIVE="$SOURCE_TEST_ACTIVE_VALUE" \
      SOURCE_TEST_TRIM_START="$SOURCE_TEST_TRIM_START_VALUE" \
      SOURCE_TEST_TRIM_DURATION="$SOURCE_TEST_TRIM_DURATION_VALUE" \
      VFO_ASSUME_YES="$VFO_ASSUME_YES_VALUE" \
      VFO_LIVE_OUTPUT="$VFO_LIVE_OUTPUT_VALUE" \
      PATH="${VFO_ACTIONS_PATH_PREFIX:+${VFO_ACTIONS_PATH_PREFIX}:}${VFO_PATH_PREFIX:+${VFO_PATH_PREFIX}:}$PATH" \
      bash -o pipefail -c "$command_text"
  ) 2>&1 | tee "$log_file"
  status="${PIPESTATUS[0]}"
  printf '%s\n' "$status" > "$status_file"
  log "END ${stage}: exit=${status}"
  return "$status"
}

collect_weak_signals() {
  local output_file="$1"

  {
    echo "[repository_weak_signals]"
    if command -v rg >/dev/null 2>&1; then
      rg -n --no-heading 'TODO|FIXME|HACK|flaky|flake' \
        "$ROOT_DIR/services/vfo/actions" \
        "$ROOT_DIR/services/vfo/src" \
        "$ROOT_DIR/tests/e2e" 2>/dev/null || true
    else
      grep -RInE 'TODO|FIXME|HACK|flaky|flake' \
        "$ROOT_DIR/services/vfo/actions" \
        "$ROOT_DIR/services/vfo/src" \
        "$ROOT_DIR/tests/e2e" 2>/dev/null || true
    fi
    echo
    echo "[runtime_weak_signals]"
    find "$(dirname "$output_file")" -maxdepth 1 -type f -name '*.log' -print0 |
      while IFS= read -r -d '' log_file; do
        grep -Ehi 'WARN:|WARNING:|Abort trap|fallback|deprecated|Last message repeated' "$log_file" || true
      done
  } > "$output_file"
}

first_matching_line() {
  local pattern="$1"
  shift
  local file

  for file in "$@"; do
    [ -f "$file" ] || continue
    grep -Eihm 1 "$pattern" "$file" && return 0
  done
  return 1
}

normalize_for_signature() {
  sed -E \
    -e "s#${PRACTICE_ROOT}/[^[:space:]]+#<vfo_practice_path>#g" \
    -e 's#/var/folders/[^[:space:]]+#<system_tmp_path>#g' \
    -e 's#/tmp/[^[:space:]]+#<tmp_path>#g' \
    -e 's#vfo-[A-Za-z0-9]+#vfo-<tmp>#g' \
    -e 's#[0-9A-Fa-f]{8,}#<hex>#g'
}

signature_for_text() {
  shasum -a 256 | awk '{print $1}'
}

compose_issue_body() {
  local body_file="$1"
  local signature="$2"
  local asset="$3"
  local stage="$4"
  local command_text="$5"
  local metadata_file="$6"
  local failure_file="$7"
  local weak_file="$8"
  local report_dir="$9"

  {
    echo "<!-- vfo-practice-signature:${signature} -->"
    echo
    echo "## Summary"
    echo
    echo "VFO practice validation detected a technical failure or enabled weak signal."
    echo
    echo "No media content summary is included."
    echo
    echo "## Reproduction"
    echo
    echo "- Practice root: \`${PRACTICE_ROOT}\`"
    echo "- Asset path: \`${asset}\`"
    echo "- Profile pack: \`${PROFILE_PACK}\`"
    echo "- Command: \`${command_text}\`"
    echo "- Failing stage: \`${stage}\`"
    echo "- Report directory: \`${report_dir}\`"
    echo
    echo "## Minimal Technical Metadata"
    echo
    echo '```text'
    cat "$metadata_file"
    echo '```'
    echo
    echo "## Exact Error Text"
    echo
    echo '```text'
    cat "$failure_file"
    echo '```'
    echo
    echo "## Weak Signals Captured"
    echo
    echo '```text'
    sed -n '1,120p' "$weak_file"
    echo '```'
  } > "$body_file"
}

ensure_issue_label() {
  if ! command -v gh >/dev/null 2>&1; then
    warn "gh is not available; cannot create GitHub issue"
    return 1
  fi

  if ! gh auth status >/dev/null 2>&1; then
    warn "gh is not authenticated; cannot create GitHub issue"
    return 1
  fi

  if ! gh label list --repo "$GITHUB_REPO" --json name --jq ".[] | select(.name == \"${ISSUE_LABEL}\") | .name" | grep -qx "$ISSUE_LABEL"; then
    gh label create "$ISSUE_LABEL" \
      --repo "$GITHUB_REPO" \
      --color "0E8A16" \
      --description "Candidate issue found by VFO practice validation" >/dev/null 2>&1 || true
  fi
}

upsert_issue() {
  local title="$1"
  local body_file="$2"
  local signature="$3"
  local existing_issue

  ensure_issue_label || return 0

  existing_issue="$(gh issue list \
    --repo "$GITHUB_REPO" \
    --state open \
    --label "$ISSUE_LABEL" \
    --search "vfo-practice-signature:${signature} in:body" \
    --json number \
    --jq '.[0].number // empty' 2>/dev/null || true)"

  if [ -n "$existing_issue" ]; then
    gh issue comment "$existing_issue" --repo "$GITHUB_REPO" --body-file "$body_file" >/dev/null
    log "Updated existing issue #${existing_issue}"
  else
    gh issue create \
      --repo "$GITHUB_REPO" \
      --label "$ISSUE_LABEL" \
      --title "$title" \
      --body-file "$body_file" >/dev/null
    log "Created new ${ISSUE_LABEL} issue"
  fi
}

analyze_run() {
  local run_dir="$1"
  local asset="$2"
  local metadata_file="$3"
  local weak_file="$4"
  local failure_file="${run_dir}/failure.txt"
  local issue_body_file="${run_dir}/issue_body.md"
  local doctor_status profile_status status_status
  local fatal_pattern weak_pattern first_error first_weak stage command_text normalized signature title_error title

  doctor_status="$(cat "${run_dir}/doctor.log.status" 2>/dev/null || printf '0')"
  status_status="$(cat "${run_dir}/status.log.status" 2>/dev/null || printf '0')"
  profile_status="$(cat "${run_dir}/profile.log.status" 2>/dev/null || printf '0')"

  fatal_pattern='MAJOR ERROR|RUN ERROR|PROFILE .*WARNING: .*failed|WARNING: all destination locations were exhausted|Source contains Dolby Vision .*conversion failed|ffmpeg command failed|Conversion failed|Error opening output|Could not write header|No space left on device|Cannot write moov atom|Invalid argument|syntax error|command not found|Segmentation fault|QUALITY ERROR:|quality stage failed|NOT a valid profile|ERROR - detected one or more unknown words'
  weak_pattern='WARN:|WARNING:|Abort trap|fallback|deprecated|Last message repeated'

  first_error="$(first_matching_line "$fatal_pattern" "${run_dir}/doctor.log" "${run_dir}/status.log" "${run_dir}/profile.log" || true)"
  first_weak="$(first_matching_line "$weak_pattern" "${run_dir}/doctor.log" "${run_dir}/status.log" "${run_dir}/profile.log" "$weak_file" || true)"

  stage="none"
  command_text="$PROFILE_COMMAND"
  if [ "$doctor_status" != "0" ]; then
    stage="doctor"
    command_text="${VFO_COMMAND_NAME} doctor"
    first_error="${first_error:-${VFO_COMMAND_NAME} doctor exited ${doctor_status}}"
  elif [ "$status_status" != "0" ]; then
    stage="status"
    command_text="${VFO_COMMAND_NAME} status"
    first_error="${first_error:-${VFO_COMMAND_NAME} status exited ${status_status}}"
  elif [ "$profile_status" != "0" ]; then
    stage="profile"
    command_text="$PROFILE_COMMAND"
    first_error="${first_error:-${PROFILE_COMMAND} exited ${profile_status}}"
  elif [ -n "$first_error" ]; then
    stage="profile"
    command_text="$PROFILE_COMMAND"
  elif [ "$ISSUE_WEAK_SIGNALS" = "1" ] && [ -n "$first_weak" ]; then
    stage="weak-signal"
    command_text="$PROFILE_COMMAND"
    first_error="$first_weak"
  fi

  if [ "$stage" = "none" ]; then
    log "No issue-worthy findings detected"
    return 0
  fi

  {
    echo "$first_error"
    echo
    echo "[exit_status]"
    echo "doctor=${doctor_status}"
    echo "status=${status_status}"
    echo "profile=${profile_status}"
  } > "$failure_file"

  normalized="$(printf '%s\n' "$first_error" | normalize_for_signature)"
  signature="$(printf '%s\n%s\n%s\n' "$stage" "$PROFILE_PACK" "$normalized" | signature_for_text)"
  title_error="$(printf '%s' "$first_error" | tr -d '\r' | cut -c 1-96)"
  title="[vfo-practice] ${stage}: ${title_error}"

  compose_issue_body "$issue_body_file" "$signature" "$asset" "$stage" "$command_text" "$metadata_file" "$failure_file" "$weak_file" "$run_dir"

  log "Finding signature: ${signature}"
  log "Finding title: ${title}"

  if [ "$CREATE_ISSUES" = "1" ]; then
    upsert_issue "$title" "$issue_body_file" "$signature"
  else
    log "Issue creation disabled; set VFO_PRACTICE_CREATE_ISSUES=1 or pass --create-issues to upsert GitHub issues"
  fi

  return 1
}

run_cycle() {
  local run_id run_dir latest_link asset metadata_file weak_file doctor_status status_status profile_status

  [ -d "$PRACTICE_ROOT" ] || fail "Practice root does not exist: $PRACTICE_ROOT"
  require_command "$VFO_BIN"

  run_id="$(date -u +%Y%m%dT%H%M%SZ)"
  run_dir="${REPORT_ROOT}/runs/${run_id}"
  mkdir -p "$run_dir"
  latest_link="${REPORT_ROOT}/latest"
  ln -sfn "$run_dir" "$latest_link"

  asset="$(find_latest_asset)"
  [ -n "$asset" ] || fail "No test media found under ${PRACTICE_ROOT}/mezzanine"

  metadata_file="${run_dir}/asset_metadata.txt"
  weak_file="${run_dir}/weak_signals.txt"
  write_asset_metadata "$asset" "$metadata_file"

  {
    echo "run_id=${run_id}"
    echo "practice_root=${PRACTICE_ROOT}"
    echo "selected_or_newest_asset=${asset}"
    echo "profile_pack=${PROFILE_PACK}"
    echo "vfo_bin=${VFO_BIN}"
    echo "vfo_command_name=${VFO_COMMAND_NAME}"
    echo "vfo_path_prefix=${VFO_PATH_PREFIX}"
    echo "vfo_actions_path_prefix=${VFO_ACTIONS_PATH_PREFIX}"
    echo "profile_command=${PROFILE_COMMAND}"
    echo "source_test_active=${SOURCE_TEST_ACTIVE_VALUE}"
    echo "source_test_trim_start=${SOURCE_TEST_TRIM_START_VALUE}"
    echo "source_test_trim_duration=${SOURCE_TEST_TRIM_DURATION_VALUE}"
  } > "${run_dir}/run_context.txt"

  log "Report directory: ${run_dir}"
  log "Context asset: ${asset}"
  log "Profile command: ${PROFILE_COMMAND}"
  log "Actions path: ${VFO_ACTIONS_PATH_PREFIX}"
  log "Smoke trim: SOURCE_TEST_ACTIVE=${SOURCE_TEST_ACTIVE_VALUE} SOURCE_TEST_TRIM_DURATION=${SOURCE_TEST_TRIM_DURATION_VALUE}"

  set +e
  run_with_log "doctor" "${VFO_COMMAND_NAME} doctor" "${run_dir}/doctor.log"
  doctor_status="$?"
  run_with_log "status" "${VFO_COMMAND_NAME} status" "${run_dir}/status.log"
  status_status="$?"
  if [ "$doctor_status" = "0" ] && [ "$status_status" = "0" ]; then
    run_with_log "profile" "$PROFILE_COMMAND" "${run_dir}/profile.log"
    profile_status="$?"
  else
    printf '99\n' > "${run_dir}/profile.log.status"
    printf 'Skipped profile command because doctor/status failed.\n' > "${run_dir}/profile.log"
    profile_status="99"
  fi
  set -e

  collect_weak_signals "$weak_file"
  analyze_run "$run_dir" "$asset" "$metadata_file" "$weak_file"
}

watch_loop() {
  local asset fingerprint

  while true; do
    asset="$(find_latest_asset)"
    [ -n "$asset" ] || fail "No test media found under ${PRACTICE_ROOT}/mezzanine"
    fingerprint="$(asset_fingerprint "$asset")"
    if [ "$fingerprint" != "$LAST_FINGERPRINT" ]; then
      LAST_FINGERPRINT="$fingerprint"
      set +e
      run_cycle
      set -e
    else
      log "No new asset fingerprint detected; sleeping ${POLL_SECONDS}s"
    fi
    sleep "$POLL_SECONDS"
  done
}

main() {
  parse_args "$@"

  case "$MODE" in
    once)
      run_cycle
      ;;
    watch)
      watch_loop
      ;;
    *)
      fail "Unsupported mode: $MODE"
      ;;
  esac
}

main "$@"
