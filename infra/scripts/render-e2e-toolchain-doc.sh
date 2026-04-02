#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
REPORT_DIR="${REPO_ROOT}/tests/e2e/.reports/latest"
DOC_PATH="${REPO_ROOT}/platform/docs-site/docs/e2e-toolchain-latest.md"
ARTIFACT_DOC_DIR="${REPO_ROOT}/platform/docs-site/docs/e2e-toolchain-artifacts"
SUMMARY_SRC="${REPORT_DIR}/toolchain_versions_summary.md"
WEB_APP_JSON="${ARTIFACT_DOC_DIR}/vfo-web-app.json"

SOURCE_WORKFLOW="${E2E_TOOLCHAIN_SOURCE_WORKFLOW:-}"
SOURCE_RUN_URL="${E2E_TOOLCHAIN_SOURCE_RUN_URL:-}"

mkdir -p "$ARTIFACT_DOC_DIR"
rm -f "${ARTIFACT_DOC_DIR}"/*.md
rm -f "$WEB_APP_JSON"

write_header() {
  printf '# Latest E2E Toolchain Report\n\n'
  printf 'This page is generated during docs CI from the latest available e2e toolchain artifact.\n\n'
  if [ -n "$SOURCE_WORKFLOW" ]; then
    printf -- '- Source workflow: `%s`\n' "$SOURCE_WORKFLOW"
  fi
  if [ -n "$SOURCE_RUN_URL" ]; then
    printf -- '- Source run: %s\n' "$SOURCE_RUN_URL"
  fi
  printf -- '- Artifact source directory in build workspace: `tests/e2e/.reports/latest/`\n\n'
}

write_web_app_json() {
  local source_label="$1"

  cat > "$WEB_APP_JSON" <<EOF
{
  "title": "Pipeline: UHD SDR Ladder",
  "selectedPipelineId": "demo",
  "sourceLabel": "$(printf '%s' "$source_label" | sed 's/"/\\"/g')",
  "sourceWorkflow": "$(printf '%s' "$SOURCE_WORKFLOW" | sed 's/"/\\"/g')",
  "sourceRunUrl": "$(printf '%s' "$SOURCE_RUN_URL" | sed 's/"/\\"/g')",
  "pipelines": [
    {
      "id": "demo",
      "label": "Demo payload",
      "title": "Pipeline: UHD SDR Ladder",
      "runLabel": "Run: 2026-04-02 18:42",
      "sourceLabel": "$(printf '%s' "$source_label" | sed 's/"/\\"/g')",
      "sourceWorkflow": "$(printf '%s' "$SOURCE_WORKFLOW" | sed 's/"/\\"/g')",
      "sourceRunUrl": "$(printf '%s' "$SOURCE_RUN_URL" | sed 's/"/\\"/g')",
      "selectedAsset": "movie_01.mxf",
      "selectedNode": "encode",
      "assets": [
        { "name": "movie_01.mxf", "status": "Failed", "icon": "✖" },
        { "name": "movie_02.mxf", "status": "Complete", "icon": "✔" },
        { "name": "movie_03.mxf", "status": "Running", "icon": "⏳" },
        { "name": "movie_04.mxf", "status": "Waiting", "icon": "○" },
        { "name": "movie_05.mxf", "status": "Complete", "icon": "✔" },
        { "name": "movie_06.mxf", "status": "Failed", "icon": "✖" },
        { "name": "movie_07.mxf", "status": "Complete", "icon": "✔" },
        { "name": "movie_08.mxf", "status": "Waiting", "icon": "○" },
        { "name": "movie_09.mxf", "status": "Waiting", "icon": "○" }
      ],
      "filters": ["All", "Failed", "Running", "Waiting", "Complete"],
      "summaryCounts": [
        { "label": "Complete", "count": 91, "icon": "✔" },
        { "label": "Failed", "count": 7, "icon": "✖" },
        { "label": "Running", "count": 12, "icon": "⏳" },
        { "label": "Waiting", "count": 18, "icon": "○" }
      ],
      "stageTotals": [
        { "label": "Input", "count": 128 },
        { "label": "Probe", "count": 128 },
        { "label": "Deint", "count": 97 },
        { "label": "Encode", "count": 91 },
        { "label": "HLS", "count": 88 },
        { "label": "QC", "count": 73 },
        { "label": "Metadata", "count": 128 }
      ]
    }
  ]
}
EOF
}

if [ ! -f "$SUMMARY_SRC" ]; then
  write_web_app_json "Demo payload"
  {
    write_header
    printf '## Status\n\n'
    printf 'No e2e toolchain summary artifact was available for this docs build.\n\n'
    printf 'This is expected for some PR-only docs builds, or before the e2e lanes have completed.\n'
  } > "$DOC_PATH"
  echo "Rendered fallback docs page: ${DOC_PATH#$REPO_ROOT/}"
  exit 0
fi

write_web_app_json "Latest e2e artifact"

for suite_doc in "$REPORT_DIR"/*_toolchain_versions.md; do
  [ -f "$suite_doc" ] || continue
  cp "$suite_doc" "$ARTIFACT_DOC_DIR/$(basename "$suite_doc")"
done

cp "$SUMMARY_SRC" "$ARTIFACT_DOC_DIR/toolchain_versions_summary.md"

{
  write_header
  printf '## Suite Reports\n\n'
  found_suite="0"
  for suite_doc in "$ARTIFACT_DOC_DIR"/*_toolchain_versions.md; do
    [ -f "$suite_doc" ] || continue
    found_suite="1"
    suite_name="$(basename "$suite_doc" .md)"
    printf -- '- [%s](e2e-toolchain-artifacts/%s)\n' "$suite_name" "$(basename "$suite_doc")"
  done
  if [ "$found_suite" = "0" ]; then
    printf -- '- no per-suite markdown reports were found in this artifact\n'
  fi

  printf '\n## Combined Summary\n\n'
  sed '1d' "$SUMMARY_SRC"
} > "$DOC_PATH"

echo "Rendered docs page: ${DOC_PATH#$REPO_ROOT/}"
echo "Copied artifact docs under: ${ARTIFACT_DOC_DIR#$REPO_ROOT/}"
