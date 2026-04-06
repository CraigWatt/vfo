#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `all_sub_preserve + preserve + audio_conform` on the 4K HEVC lane.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_SUBTITLE_SELECTION_SCOPE="all_sub_preserve"
export VFO_SUBTITLE_MODE="preserve"

exec "$SCRIPT_DIR/transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh" "$@"
