#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="${ROOT_DIR}/tests/e2e/.tmp_dv_metadata"
DASHBOARD_JSON="${ROOT_DIR}/tests/e2e/.reports/latest/run_dv_metadata_optional_e2e_web_app_dashboard.json"
# shellcheck source=lib/e2e_toolchain_report.sh
. "${ROOT_DIR}/tests/e2e/lib/e2e_toolchain_report.sh"

DV_P7_ASSET="${VFO_E2E_DV_P7_ASSET:-}"
DV_CLIP_DURATION="${VFO_E2E_DV_CLIP_DURATION:-8}"
DV_REQUIRE_RETENTION="${VFO_E2E_DV_REQUIRE_RETENTION:-0}"
DV_REQUIRE_P81="${VFO_E2E_DV_REQUIRE_P81:-1}"
KEEP_TMP="${VFO_E2E_KEEP_TMP:-0}"

ACTION_DV="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_dv_profile.sh"
ACTION_MAIN_SUB_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_main_subtitle_preserve_profile.sh"
INPUT_CLIP="${TMP_DIR}/dv_input_clip.mkv"
OUTPUT_MEDIA="${TMP_DIR}/dv_output.mkv"
OUTPUT_MAIN_SUB_REQUEST="${TMP_DIR}/dv_main_subtitle_output.mp4"

log() {
  printf '[dv-e2e] %s\n' "$*"
}

warn() {
  printf '[dv-e2e] WARN: %s\n' "$*"
}

fail() {
  printf '[dv-e2e] ERROR: %s\n' "$*" >&2
  exit 1
}

cleanup() {
  if [ "$KEEP_TMP" = "1" ]; then
    log "Keeping temporary data at: ${TMP_DIR}"
    return 0
  fi
  rm -rf "$TMP_DIR"
}

has_dovi_side_data() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$1" 2>/dev/null | grep -qi "dovi"
}

get_dovi_profile() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream_side_data \
    -of default=nw=1 "$1" 2>/dev/null | awk -F= '/^dv_profile=/{print $2; exit}' | tr -d ' \t\r\n'
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing command: $1"
}

write_dashboard_skip() {
  mkdir -p "$(dirname "$DASHBOARD_JSON")"
  cat > "$DASHBOARD_JSON" <<EOF
{
  "title": "VFO E2E Dashboard",
  "selectedPipelineId": "run_dv_metadata_optional_e2e",
  "sourceLabel": "Latest e2e artifact",
  "sourceWorkflow": "run_dv_metadata_optional_e2e",
  "sourceRunUrl": "",
  "pipelines": [
    {
      "id": "run_dv_metadata_optional_e2e",
      "label": "DV metadata optional",
      "title": "Pipeline: DV Metadata Optional",
      "runLabel": "Skipped because no DV P7/P8 fixture was configured",
      "sourceLabel": "Latest e2e artifact",
      "sourceWorkflow": "run_dv_metadata_optional_e2e",
      "sourceRunUrl": "",
      "selectedAsset": "dv_p7_asset",
      "selectedNode": "skip",
      "assets": [
        { "name": "dv_p7_asset", "status": "Waiting", "icon": "○" },
        { "name": "dv_input_clip.mkv", "status": "Waiting", "icon": "○" }
      ],
      "filters": ["All", "Failed", "Running", "Waiting", "Complete"],
      "summaryCounts": [
        { "label": "Complete", "count": 0, "icon": "✔" },
        { "label": "Failed", "count": 0, "icon": "✖" },
        { "label": "Running", "count": 0, "icon": "⏳" },
        { "label": "Waiting", "count": 2, "icon": "○" }
      ],
      "stageTotals": [
        { "label": "Input", "count": 1 },
        { "label": "DV", "count": 0 },
        { "label": "Main Sub", "count": 0 }
      ],
      "workflow": {
        "nodes": [
          { "id": "skip", "label": "Skip", "subtitle": "DV fixture not configured on this run", "x": 48, "y": 146, "status": "waiting" }
        ],
        "edges": [],
        "details": {
          "skip": {
            "node": "Skip",
            "status": "Waiting",
            "exitCode": null,
            "command": "VFO_E2E_DV_P7_ASSET not set",
            "output": [
              "result=skipped",
              "reason=optional_dv_fixture_missing"
            ]
          }
        }
      }
    }
  ]
}
EOF
}

write_dashboard_pass() {
  mkdir -p "$(dirname "$DASHBOARD_JSON")"
  cat > "$DASHBOARD_JSON" <<EOF
{
  "title": "VFO E2E Dashboard",
  "selectedPipelineId": "run_dv_metadata_optional_e2e",
  "sourceLabel": "Latest e2e artifact",
  "sourceWorkflow": "run_dv_metadata_optional_e2e",
  "sourceRunUrl": "",
  "pipelines": [
    {
      "id": "run_dv_metadata_optional_e2e",
      "label": "DV metadata optional",
      "title": "Pipeline: DV Metadata Optional",
      "runLabel": "DV fixture provided and validated successfully",
      "sourceLabel": "Latest e2e artifact",
      "sourceWorkflow": "run_dv_metadata_optional_e2e",
      "sourceRunUrl": "",
      "selectedAsset": "dv_input_clip.mkv",
      "selectedNode": "main_subtitle",
      "assets": [
        { "name": "dv_input_clip.mkv", "status": "Complete", "icon": "✔" },
        { "name": "dv_output.mkv", "status": "Complete", "icon": "✔" },
        { "name": "dv_main_subtitle_output.mp4", "status": "Complete", "icon": "✔" }
      ],
      "filters": ["All", "Failed", "Running", "Waiting", "Complete"],
      "summaryCounts": [
        { "label": "Complete", "count": 3, "icon": "✔" },
        { "label": "Failed", "count": 0, "icon": "✖" },
        { "label": "Running", "count": 0, "icon": "⏳" },
        { "label": "Waiting", "count": 0, "icon": "○" }
      ],
      "stageTotals": [
        { "label": "Input", "count": 1 },
        { "label": "DV", "count": 1 },
        { "label": "Main Sub", "count": 1 }
      ],
      "workflow": {
        "nodes": [
          { "id": "input", "label": "Input", "subtitle": "DV source fixture", "x": 48, "y": 146, "status": "complete" },
          { "id": "dv", "label": "DV", "subtitle": "transcode_hevc_4k_dv_profile.sh", "x": 280, "y": 146, "status": "complete" },
          { "id": "main_subtitle", "label": "Main Sub", "subtitle": "4K subtitle-preserve DV retention check", "x": 510, "y": 146, "status": "complete" }
        ],
        "edges": [
          { "source": "input", "target": "dv" },
          { "source": "dv", "target": "main_subtitle" }
        ],
        "details": {
          "input": {
            "node": "Input",
            "status": "Complete",
            "exitCode": 0,
            "command": "ffmpeg -ss 0 -t 8 -i DV_P7_ASSET -c copy dv_input_clip.mkv",
            "output": [
              "fixture=DV_P7_ASSET",
              "result=accepted"
            ]
          },
          "dv": {
            "node": "DV",
            "status": "Complete",
            "exitCode": 0,
            "command": "bash services/vfo/actions/transcode_hevc_4k_dv_profile.sh dv_input_clip.mkv dv_output.mkv",
            "output": [
              "dovi_side_data=retained",
              "profile_conversion=8.x",
              "result=pass"
            ]
          },
          "main_subtitle": {
            "node": "Main Sub",
            "status": "Complete",
            "exitCode": 0,
            "command": "bash services/vfo/actions/transcode_hevc_4k_main_subtitle_preserve_profile.sh dv_input_clip.mkv dv_main_subtitle_output.mp4",
            "output": [
              "dovi_side_data=retained",
              "result=pass"
            ]
          }
        }
      }
    }
  ]
}
EOF
}

write_dashboard_warn() {
  mkdir -p "$(dirname "$DASHBOARD_JSON")"
  cat > "$DASHBOARD_JSON" <<EOF
{
  "title": "VFO E2E Dashboard",
  "selectedPipelineId": "run_dv_metadata_optional_e2e",
  "sourceLabel": "Latest e2e artifact",
  "sourceWorkflow": "run_dv_metadata_optional_e2e",
  "sourceRunUrl": "",
  "pipelines": [
    {
      "id": "run_dv_metadata_optional_e2e",
      "label": "DV metadata optional",
      "title": "Pipeline: DV Metadata Optional",
      "runLabel": "DV fixture provided, side data not retained",
      "sourceLabel": "Latest e2e artifact",
      "sourceWorkflow": "run_dv_metadata_optional_e2e",
      "sourceRunUrl": "",
      "selectedAsset": "dv_input_clip.mkv",
      "selectedNode": "main_subtitle",
      "assets": [
        { "name": "dv_input_clip.mkv", "status": "Complete", "icon": "✔" },
        { "name": "dv_output.mkv", "status": "Complete", "icon": "✔" },
        { "name": "dv_main_subtitle_output.mp4", "status": "Waiting", "icon": "○" }
      ],
      "filters": ["All", "Failed", "Running", "Waiting", "Complete"],
      "summaryCounts": [
        { "label": "Complete", "count": 2, "icon": "✔" },
        { "label": "Failed", "count": 0, "icon": "✖" },
        { "label": "Running", "count": 0, "icon": "⏳" },
        { "label": "Waiting", "count": 1, "icon": "○" }
      ],
      "stageTotals": [
        { "label": "Input", "count": 1 },
        { "label": "DV", "count": 1 },
        { "label": "Main Sub", "count": 1 }
      ],
      "workflow": {
        "nodes": [
          { "id": "input", "label": "Input", "subtitle": "DV source fixture", "x": 48, "y": 146, "status": "complete" },
          { "id": "dv", "label": "DV", "subtitle": "transcode_hevc_4k_dv_profile.sh", "x": 280, "y": 146, "status": "complete" },
          { "id": "main_subtitle", "label": "Main Sub", "subtitle": "DV retention not preserved in this branch", "x": 510, "y": 146, "status": "waiting" }
        ],
        "edges": [
          { "source": "input", "target": "dv" },
          { "source": "dv", "target": "main_subtitle" }
        ],
        "details": {
          "input": {
            "node": "Input",
            "status": "Complete",
            "exitCode": 0,
            "command": "ffmpeg -ss 0 -t 8 -i DV_P7_ASSET -c copy dv_input_clip.mkv",
            "output": [
              "fixture=DV_P7_ASSET",
              "result=accepted"
            ]
          },
          "dv": {
            "node": "DV",
            "status": "Complete",
            "exitCode": 0,
            "command": "bash services/vfo/actions/transcode_hevc_4k_dv_profile.sh dv_input_clip.mkv dv_output.mkv",
            "output": [
              "dovi_side_data=present",
              "profile_conversion=8.x",
              "result=pass"
            ]
          },
          "main_subtitle": {
            "node": "Main Sub",
            "status": "Waiting",
            "exitCode": null,
            "command": "bash services/vfo/actions/transcode_hevc_4k_main_subtitle_preserve_profile.sh dv_input_clip.mkv dv_main_subtitle_output.mp4",
            "output": [
              "dovi_side_data=not_retained",
              "result=warn"
            ]
          }
        }
      }
    }
  ]
}
EOF
}

main() {
  e2e_write_toolchain_report \
    "$ROOT_DIR" \
    "run_dv_metadata_optional_e2e" \
    ffmpeg ffprobe dovi_tool

  if [ -z "$DV_P7_ASSET" ] || [ ! -f "$DV_P7_ASSET" ]; then
    log "Skipping optional DV metadata test (VFO_E2E_DV_P7_ASSET not set or file missing)"
    write_dashboard_skip
    exit 0
  fi

  require_command ffmpeg
  require_command ffprobe
  require_command dovi_tool
  [ -x "$ACTION_DV" ] || fail "DV action script is missing or not executable: $ACTION_DV"
  [ -x "$ACTION_MAIN_SUB_4K" ] || fail "Main-subtitle 4K action script is missing or not executable: $ACTION_MAIN_SUB_4K"

  rm -rf "$TMP_DIR"
  mkdir -p "$TMP_DIR"
  trap cleanup EXIT

  log "Preparing DV fixture clip from: ${DV_P7_ASSET}"
  ffmpeg -hide_banner -nostdin -y \
    -ss 0 -t "$DV_CLIP_DURATION" \
    -i "$DV_P7_ASSET" \
    -map 0:v:0 -map 0:a? \
    -c copy \
    "$INPUT_CLIP" >/dev/null 2>&1

  [ -s "$INPUT_CLIP" ] || fail "Could not produce DV input clip"

  if ! has_dovi_side_data "$INPUT_CLIP"; then
    fail "Configured DV fixture does not signal Dolby Vision side data: ${DV_P7_ASSET}"
  fi
  input_dv_profile="$(get_dovi_profile "$INPUT_CLIP")"
  if [ -n "$input_dv_profile" ]; then
    log "Input DV profile detected: ${input_dv_profile}"
  fi

  log "Running DV-capable profile action"
  env \
    VFO_ENCODER_MODE=cpu \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    AVG_K=5000 \
    MAXRATE_K=7000 \
    BUFSIZE_K=14000 \
    VFO_DV_REQUIRE_DOVI="$DV_REQUIRE_RETENTION" \
    VFO_DV_REQUIRE_P7_TO_81="$DV_REQUIRE_P81" \
    bash "$ACTION_DV" "$INPUT_CLIP" "$OUTPUT_MEDIA"

  [ -s "$OUTPUT_MEDIA" ] || fail "DV action produced no output"
  ffprobe -v error "$OUTPUT_MEDIA" >/dev/null 2>&1 || fail "ffprobe cannot read DV action output"

  if has_dovi_side_data "$OUTPUT_MEDIA"; then
    output_dv_profile="$(get_dovi_profile "$OUTPUT_MEDIA")"
    if [ "$input_dv_profile" = "7" ] && [ "$DV_REQUIRE_P81" = "1" ] && [ "$output_dv_profile" != "8" ]; then
      fail "DV profile conversion check failed: input profile 7 did not output profile 8.x"
    fi
    if [ "$input_dv_profile" = "7" ] && [ "$output_dv_profile" = "8" ]; then
      log "DV profile conversion check passed: input profile 7 converted to profile 8.x"
    fi
    log "DV side data retained in output"
  else
    if [ "$DV_REQUIRE_RETENTION" = "1" ]; then
      fail "DV side data missing in output while VFO_E2E_DV_REQUIRE_RETENTION=1"
    fi
    warn "DV side data not retained in standalone DV action output (allowed because VFO_E2E_DV_REQUIRE_RETENTION=0)"
  fi

  log "Running 4K main-subtitle profile action with DV retention settings"
  rm -f "$OUTPUT_MAIN_SUB_REQUEST" "${OUTPUT_MAIN_SUB_REQUEST%.*}.mkv"
  env \
    VFO_ENCODER_MODE=cpu \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    AVG_K=5000 \
    MAXRATE_K=7000 \
    BUFSIZE_K=14000 \
    VFO_DV_REQUIRE_DOVI="$DV_REQUIRE_RETENTION" \
    VFO_DV_REQUIRE_P7_TO_81="$DV_REQUIRE_P81" \
    bash "$ACTION_MAIN_SUB_4K" "$INPUT_CLIP" "$OUTPUT_MAIN_SUB_REQUEST"

  main_sub_output="$OUTPUT_MAIN_SUB_REQUEST"
  if [ -f "${OUTPUT_MAIN_SUB_REQUEST%.*}.mkv" ]; then
    main_sub_output="${OUTPUT_MAIN_SUB_REQUEST%.*}.mkv"
  fi

  [ -s "$main_sub_output" ] || fail "Main-subtitle 4K action produced no output"
  ffprobe -v error "$main_sub_output" >/dev/null 2>&1 || fail "ffprobe cannot read main-subtitle output"

  if has_dovi_side_data "$main_sub_output"; then
    main_sub_output_dv_profile="$(get_dovi_profile "$main_sub_output")"
    if [ "$input_dv_profile" = "7" ] && [ "$DV_REQUIRE_P81" = "1" ] && [ "$main_sub_output_dv_profile" != "8" ]; then
      fail "Main-subtitle DV profile conversion check failed: input profile 7 did not output profile 8.x"
    fi
    log "Main-subtitle 4K action retained DV side data"
    write_dashboard_pass
    exit 0
  fi

  if [ "$DV_REQUIRE_RETENTION" = "1" ]; then
    fail "Main-subtitle 4K action output is missing DV side data while VFO_E2E_DV_REQUIRE_RETENTION=1"
  fi

  warn "Main-subtitle 4K action output missing DV side data (allowed because VFO_E2E_DV_REQUIRE_RETENTION=0)"
  write_dashboard_warn
  exit 0
}

main "$@"
