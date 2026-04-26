#!/usr/bin/env bash
set -euo pipefail

# Shared quality-mode helpers for bounded aggressive-VMAF retries.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=live_encode_tools.sh
. "$SCRIPT_DIR/live_encode_tools.sh"

QUALITY_MODE_NAME="${VFO_QUALITY_MODE:-standard}"
QUALITY_MODE_VMAF_MIN="${VFO_QUALITY_VMAF_MIN:-94}"
QUALITY_MODE_VMAF_MAX_PASSES="${VFO_QUALITY_VMAF_MAX_PASSES:-4}"
QUALITY_MODE_VMAF_HW_REDUCTION_PCT="${VFO_QUALITY_VMAF_HW_REDUCTION_PCT:-12}"
QUALITY_MODE_VMAF_CPU_CRF_STEP="${VFO_QUALITY_VMAF_CPU_CRF_STEP:-2}"
QUALITY_MODE_VMAF_POOL="${VFO_QUALITY_VMAF_POOL:-harmonic_mean}"

quality_mode_is_aggressive_vmaf() {
  [ "$(printf '%s' "$QUALITY_MODE_NAME" | tr '[:upper:]' '[:lower:]')" = "aggressive_vmaf" ]
}

quality_mode_has_libvmaf() {
  ffmpeg -hide_banner -filters 2>/dev/null | grep -q " libvmaf "
}

quality_mode_scale_kbits() {
  local base_kbits="$1"
  local pass_index="$2"
  local reduction_pct="$3"

  awk -v base="$base_kbits" -v pass="$pass_index" -v pct="$reduction_pct" '
    function clamp(v) {
      if(v < 1)
        return 1;
      return int(v + 0.5);
    }
    BEGIN {
      value = base;
      factor = (100 - pct) / 100.0;
      for(i = 0; i < pass; i++)
        value *= factor;
      print clamp(value);
    }'
}

quality_mode_step_crf() {
  local base_crf="$1"
  local pass_index="$2"
  local step_size="$3"

  awk -v base="$base_crf" -v pass="$pass_index" -v step="$step_size" 'BEGIN { print base + (pass * step) }'
}

quality_mode_score_meets_floor() {
  local score="$1"
  local floor="$2"

  awk -v score="$score" -v floor="$floor" 'BEGIN { exit(score + 0 >= floor + 0 ? 0 : 1) }'
}

quality_mode_measure_vmaf() {
  local distorted="$1"
  local reference="$2"
  local output=""

  output="$(
    ffmpeg -hide_banner -nostdin \
      -i "$distorted" -i "$reference" \
      -lavfi "[1:v][0:v]scale2ref=flags=bicubic[ref_s][dist_s];[dist_s][ref_s]libvmaf=n_threads=0:pool=${QUALITY_MODE_VMAF_POOL}" \
      -f null - 2>&1 >/dev/null || true
  )"

  printf '%s\n' "$output" \
    | awk -F: '/VMAF score:/ { gsub(/^[[:space:]]+/, "", $2); score=$2 } END { if(score != "") { print score; exit 0 } exit 1 }'
}
