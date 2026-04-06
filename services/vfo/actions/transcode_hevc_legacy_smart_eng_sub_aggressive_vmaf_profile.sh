#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `smart_eng_sub + preserve + aggressive_vmaf` on the legacy/sub-HD HEVC lane.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_QUALITY_MODE="aggressive_vmaf"

exec "$SCRIPT_DIR/transcode_hevc_legacy_main_subtitle_preserve_profile.sh" "$@"
