#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `all_sub_preserve` on the legacy/sub-HD HEVC lane.
#
# Behavior:
# - delegates to the canonical legacy HEVC subtitle-aware action
# - sets subtitle selection scope to `all_sub_preserve`
# - keeps subtitle mode at `preserve`
# - preserves audio streams with stream copy

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_SUBTITLE_SELECTION_SCOPE="all_sub_preserve"
export VFO_SUBTITLE_MODE="preserve"

exec "$SCRIPT_DIR/transcode_hevc_legacy_main_subtitle_preserve_profile.sh" "$@"
