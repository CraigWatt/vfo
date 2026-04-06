#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `smart_eng_sub + subtitle_convert` on the 4K HEVC lane.
#
# Behavior:
# - delegates to the canonical 4K HEVC subtitle-aware action
# - keeps selection scope at `smart_eng_sub`
# - changes subtitle mode to `subtitle_convert`
# - preserves audio streams with stream copy
# - fails on bitmap subtitle conversion unless `VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv`

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_SUBTITLE_SELECTION_SCOPE="smart_eng_sub"
export VFO_SUBTITLE_MODE="subtitle_convert"

exec "$SCRIPT_DIR/transcode_hevc_4k_main_subtitle_preserve_profile.sh" "$@"
