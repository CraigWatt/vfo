#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `smart_eng_sub + subtitle_convert + audio_conform` on the 1080 HEVC lane.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_SUBTITLE_SELECTION_SCOPE="smart_eng_sub"
export VFO_SUBTITLE_MODE="subtitle_convert"

exec "$SCRIPT_DIR/transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh" "$@"
