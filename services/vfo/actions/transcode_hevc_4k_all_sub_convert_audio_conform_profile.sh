#!/usr/bin/env bash
set -euo pipefail

# Wrapper profile action for `all_sub_preserve + subtitle_convert + audio_conform`
# on the 4K HEVC lane.
#
# Behavior:
# - Inherits the 4K HEVC encode path from the smart-English audio-conform action.
# - Defaults subtitle behavior to `all_sub_preserve + subtitle_convert`.
# - Text subtitles normalize to `mov_text` when MP4 remains viable.
# - Bitmap subtitles fall back to MKV preservation by default.
# - Preserves AAC and Dolby-family audio streams where already acceptable.
# - Conforms DTS-family and PCM-family audio when needed.
# - Supports the same bounded video-only `aggressive_vmaf` mode as the underlying HEVC lane.
#
# Optional env:
#   VFO_ENCODER_MODE=auto|hw|cpu
#   VFO_MP4_STREAM_MODE=fmp4_faststart|fmp4|faststart
#   VFO_SUBTITLE_SELECTION_SCOPE=all_sub_preserve
#   VFO_SUBTITLE_MODE=subtitle_convert
#   VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv|fail
#   VFO_QUALITY_MODE=standard|aggressive_vmaf

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VFO_SUBTITLE_SELECTION_SCOPE="${VFO_SUBTITLE_SELECTION_SCOPE:-all_sub_preserve}"
export VFO_SUBTITLE_MODE="${VFO_SUBTITLE_MODE:-subtitle_convert}"
export VFO_SUBTITLE_CONVERT_BITMAP_POLICY="${VFO_SUBTITLE_CONVERT_BITMAP_POLICY:-preserve_mkv}"

exec "$SCRIPT_DIR/transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh" "$@"
