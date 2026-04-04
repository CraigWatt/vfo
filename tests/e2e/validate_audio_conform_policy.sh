#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AUDIO_HELPER="${ROOT_DIR}/services/vfo/actions/audio_conform_tools.sh"

# shellcheck source=/dev/null
. "$AUDIO_HELPER"

fail() {
  printf '[validate-audio-conform] ERROR: %s\n' "$*" >&2
  exit 1
}

assert_equals() {
  local actual="$1"
  local expected="$2"
  local message="$3"

  if [ "$actual" != "$expected" ]; then
    fail "${message}. expected='${expected}' actual='${actual}'"
  fi
}

assert_true() {
  local message="$1"
  shift
  if ! "$@"; then
    fail "$message"
  fi
}

assert_false() {
  local message="$1"
  shift
  if "$@"; then
    fail "$message"
  fi
}

main() {
  audio_conform_prepare_encoder_cache

  assert_true "dts should be recognized as DTS-family" audio_conform_is_dts_family_codec dts
  assert_true "dca should be recognized as DTS-family" audio_conform_is_dts_family_codec dca
  assert_false "aac should not be recognized as DTS-family" audio_conform_is_dts_family_codec aac
  assert_true "pcm_s24le should be recognized as PCM-family" audio_conform_is_pcm_family_codec pcm_s24le
  assert_false "aac should not be recognized as PCM-family" audio_conform_is_pcm_family_codec aac

  assert_true "dts should require delivery conform" audio_conform_requires_delivery_conform dts
  assert_true "pcm_s24le should require delivery conform" audio_conform_requires_delivery_conform pcm_s24le
  assert_false "aac should not require delivery conform" audio_conform_requires_delivery_conform aac

  assert_true "aac should be treated as MP4-safe when preserved" audio_conform_is_mp4_safe_preserve_codec aac
  assert_true "eac3 should be treated as MP4-safe when preserved" audio_conform_is_mp4_safe_preserve_codec eac3
  assert_false "truehd should force MKV when preserved" audio_conform_is_mp4_safe_preserve_codec truehd

  assert_equals "$(audio_conform_stream_target_channels 1)" "1" "mono DTS should remain mono"
  assert_equals "$(audio_conform_stream_target_channels 2)" "2" "stereo DTS should remain stereo"
  assert_equals "$(audio_conform_stream_target_channels 4)" "4" "4-channel DTS should remain 4-channel"
  assert_equals "$(audio_conform_stream_target_channels 6)" "6" "5.1 DTS should remain 5.1"
  assert_equals "$(audio_conform_stream_target_channels 8)" "6" "8-channel DTS should downmix to 5.1"

  assert_equals "$(audio_conform_stream_target_codec 1)" "aac" "mono DTS should conform to AAC"
  assert_equals "$(audio_conform_stream_target_codec 2)" "aac" "stereo DTS should conform to AAC"
  if [ "$AUDIO_CONFORM_HAVE_EAC3" = "1" ]; then
    assert_equals "$(audio_conform_stream_target_codec 3)" "eac3" "multichannel DTS should prefer E-AC-3"
    assert_equals "$(audio_conform_stream_target_codec 8)" "eac3" "8-channel DTS should still prefer E-AC-3 after downmix"
  else
    assert_equals "$(audio_conform_stream_target_codec 3)" "ac3" "multichannel DTS should fall back to AC-3"
    assert_equals "$(audio_conform_stream_target_codec 8)" "ac3" "8-channel DTS should fall back to AC-3 after downmix"
  fi

  assert_equals "$(audio_conform_stream_bitrate aac 2)" "$AUDIO_CONFORM_AAC_STEREO_BITRATE" "stereo AAC bitrate should use the configured stereo target"
  assert_equals "$(audio_conform_stream_bitrate ac3 6)" "$AUDIO_CONFORM_AC3_6CH_BITRATE" "AC-3 5.1 bitrate should use the configured multichannel target"
  if [ "$AUDIO_CONFORM_HAVE_EAC3" = "1" ]; then
    assert_equals "$(audio_conform_stream_bitrate eac3 6)" "$AUDIO_CONFORM_EAC3_6CH_BITRATE" "E-AC-3 5.1 bitrate should use the configured multichannel target"
  fi

  printf '[validate-audio-conform] delivery-conform policy checks passed\n'
}

main "$@"
