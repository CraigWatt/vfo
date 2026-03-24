#!/usr/bin/env bash
set -euo pipefail

# Guardrail skip action.
#
# Contract (called from vfo config):
#   profile_guardrail_skip.sh <input_file> <output_file> [reason]
#
# Behavior:
# - Writes a sidecar skip marker beside the requested output path.
# - Exits success (0) so vfo can continue processing remaining candidates.

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <input_file> <output_file> [reason]"
  exit 1
fi

INPUT="$1"
OUTPUT="$2"
REASON="${3:-profile guardrail did not match this candidate}"

if [ ! -f "$INPUT" ]; then
  echo "Input file not found: $INPUT"
  exit 1
fi

OUTPUT_DIR="$(dirname "$OUTPUT")"
OUTPUT_BASE="$(basename "$OUTPUT")"
MARKER_PATH="${OUTPUT%.*}.guardrail_skipped.txt"

mkdir -p "$OUTPUT_DIR"

cat > "$MARKER_PATH" <<EOF
vfo profile guardrail skip marker
input_file=$INPUT
requested_output=$OUTPUT
requested_output_basename=$OUTPUT_BASE
reason=$REASON
EOF

echo "Guardrail skip marker written: $MARKER_PATH"
