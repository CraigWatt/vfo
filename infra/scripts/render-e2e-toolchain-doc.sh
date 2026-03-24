#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
REPORT_DIR="${REPO_ROOT}/tests/e2e/.reports/latest"
DOC_PATH="${REPO_ROOT}/platform/docs-site/docs/e2e-toolchain-latest.md"
ARTIFACT_DOC_DIR="${REPO_ROOT}/platform/docs-site/docs/e2e-toolchain-artifacts"
SUMMARY_SRC="${REPORT_DIR}/toolchain_versions_summary.md"

SOURCE_WORKFLOW="${E2E_TOOLCHAIN_SOURCE_WORKFLOW:-}"
SOURCE_RUN_URL="${E2E_TOOLCHAIN_SOURCE_RUN_URL:-}"

mkdir -p "$ARTIFACT_DOC_DIR"
rm -f "${ARTIFACT_DOC_DIR}"/*.md

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

if [ ! -f "$SUMMARY_SRC" ]; then
  {
    write_header
    printf '## Status\n\n'
    printf 'No e2e toolchain summary artifact was available for this docs build.\n\n'
    printf 'This is expected for some PR-only docs builds, or before the e2e lanes have completed.\n'
  } > "$DOC_PATH"
  echo "Rendered fallback docs page: ${DOC_PATH#$REPO_ROOT/}"
  exit 0
fi

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
