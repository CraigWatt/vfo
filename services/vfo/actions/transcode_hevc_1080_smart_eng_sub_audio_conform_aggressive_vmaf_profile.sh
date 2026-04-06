#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `smart_eng_sub + preserve + audio_conform + aggressive_vmaf`
# on the 1080 HEVC lane.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_QUALITY_MODE="aggressive_vmaf"

exec "$SCRIPT_DIR/transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh" "$@"
