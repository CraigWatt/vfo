#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP_DIR="${ROOT_DIR}/tests/e2e/.tmp"
FIXTURES_DIR="${TMP_DIR}/fixtures"
OUTPUTS_DIR="${TMP_DIR}/outputs"
# shellcheck source=lib/e2e_toolchain_report.sh
. "${ROOT_DIR}/tests/e2e/lib/e2e_toolchain_report.sh"

ASSET_MODE="${VFO_E2E_ASSET_MODE:-auto}" # auto|local|synthetic
ASSETS_DIR="${VFO_E2E_ASSETS_DIR:-${ROOT_DIR}/tests/e2e/assets/open-source}"
CLIP_DURATION="${VFO_E2E_CLIP_DURATION:-2}"
MAX_SEEDS="${VFO_E2E_MAX_SEEDS:-1}"
KEEP_TMP="${VFO_E2E_KEEP_TMP:-0}"

ACTION_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_profile.sh"
ACTION_1080="${ROOT_DIR}/services/vfo/actions/transcode_hevc_1080_profile.sh"
ACTION_MAIN_SUB_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_main_subtitle_preserve_profile.sh"
ACTION_MAIN_SUB_1080="${ROOT_DIR}/services/vfo/actions/transcode_hevc_1080_main_subtitle_preserve_profile.sh"
ACTION_MAIN_SUB_LEGACY="${ROOT_DIR}/services/vfo/actions/transcode_hevc_legacy_main_subtitle_preserve_profile.sh"
ACTION_SMART_ENG_AUDIO_CONFORM_4K="${ROOT_DIR}/services/vfo/actions/transcode_hevc_4k_smart_eng_sub_audio_conform_profile.sh"
ACTION_SMART_ENG_AUDIO_CONFORM_1080="${ROOT_DIR}/services/vfo/actions/transcode_hevc_1080_smart_eng_sub_audio_conform_profile.sh"
ACTION_SMART_ENG_AUDIO_CONFORM_LEGACY="${ROOT_DIR}/services/vfo/actions/transcode_hevc_legacy_smart_eng_sub_audio_conform_profile.sh"
ACTION_GUARDRAIL_SKIP="${ROOT_DIR}/services/vfo/actions/profile_guardrail_skip.sh"
VALIDATE_AUDIO_CONFORM_POLICY="${ROOT_DIR}/tests/e2e/validate_audio_conform_policy.sh"

log() {
  printf '[e2e] %s\n' "$*"
}

fail() {
  printf '[e2e] ERROR: %s\n' "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing command: $1"
}

cleanup() {
  if [ "$KEEP_TMP" = "1" ]; then
    log "Keeping temporary data at: ${TMP_DIR}"
    return 0
  fi
  rm -rf "$TMP_DIR"
}

assert_equals() {
  local actual="$1"
  local expected="$2"
  local message="$3"
  if [ "$actual" != "$expected" ]; then
    fail "${message}. expected='${expected}' actual='${actual}'"
  fi
}

assert_int_lte() {
  local value="$1"
  local max="$2"
  local message="$3"
  if [ "$value" -gt "$max" ]; then
    fail "${message}. value=${value} max=${max}"
  fi
}

assert_positive_int() {
  local value="$1"
  local name="$2"
  case "$value" in
    ''|*[!0-9]*)
      fail "${name} must be a positive integer (got: ${value})"
      ;;
    0)
      fail "${name} must be greater than zero"
      ;;
  esac
}

is_video_file() {
  local candidate="$1"
  case "$candidate" in
    *.mkv|*.mp4|*.mov|*.m4v|*.m2ts|*.ts|*.avi|*.webm|\
    *.MKV|*.MP4|*.MOV|*.M4V|*.M2TS|*.TS|*.AVI|*.WEBM)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

collect_video_assets() {
  local list_file="$1"
  local candidate
  : > "$list_file"

  [ -d "$ASSETS_DIR" ] || return 0

  while IFS= read -r candidate; do
    if is_video_file "$candidate"; then
      printf '%s\n' "$candidate" >> "$list_file"
    fi
  done < <(find "$ASSETS_DIR" -type f 2>/dev/null | LC_ALL=C sort)
}

create_fixture_from_input() {
  local input="$1"
  local width="$2"
  local height="$3"
  local output="$4"

  ffmpeg -hide_banner -nostdin -y \
    -ss 0 -t "$CLIP_DURATION" \
    -i "$input" \
    -map 0:v:0 -map 0:a? \
    -vf "scale=${width}:${height}:force_original_aspect_ratio=decrease,pad=${width}:${height}:(ow-iw)/2:(oh-ih)/2" \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a aac -b:a 128k \
    "$output" >/dev/null 2>&1
}

create_audio_conform_fixture_from_input() {
  local input="$1"
  local width="$2"
  local height="$3"
  local output="$4"

  ffmpeg -hide_banner -nostdin -y \
    -ss 0 -t "$CLIP_DURATION" \
    -i "$input" \
    -map 0:v:0 -map 0:a? \
    -vf "scale=${width}:${height}:force_original_aspect_ratio=decrease,pad=${width}:${height}:(ow-iw)/2:(oh-ih)/2" \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a copy \
    "$output" >/dev/null 2>&1
}

create_synthetic_fixture() {
  local width="$1"
  local height="$2"
  local output="$3"

  ffmpeg -hide_banner -nostdin -y \
    -f lavfi -i "testsrc2=size=${width}x${height}:rate=24" \
    -f lavfi -i "sine=frequency=1000:sample_rate=48000" \
    -t "$CLIP_DURATION" \
    -map 0:v:0 -map 1:a:0 \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a aac -b:a 128k \
    "$output" >/dev/null 2>&1
}

create_legacy_letterbox_fixture() {
  local input="$1"
  local output="$2"

  ffmpeg -hide_banner -nostdin -y \
    -i "$input" \
    -map 0:v:0 -map 0:a? \
    -vf "scale=960:300:force_original_aspect_ratio=decrease,pad=960:540:(ow-iw)/2:(oh-ih)/2:black" \
    -c:v libx264 -preset veryfast -crf 21 -pix_fmt yuv420p \
    -c:a copy \
    "$output" >/dev/null 2>&1
}

probe_video_codec() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=codec_name \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

probe_video_height() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=height \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

probe_video_width() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=width \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

probe_video_color_transfer() {
  ffprobe -v error -select_streams v:0 \
    -show_entries stream=color_transfer \
    -of default=nk=1:nw=1 "$1" | tr -d '\r\n'
}

is_hdr_transfer() {
  case "$1" in
    smpte2084|arib-std-b67)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

should_run_sdr_lanes_for_input() {
  local input="$1"
  local transfer

  transfer="$(probe_video_color_transfer "$input")"
  if is_hdr_transfer "$transfer"; then
    return 1
  fi
  return 0
}

probe_audio_count() {
  ffprobe -v error -select_streams a \
    -show_entries stream=index \
    -of csv=p=0 "$1" | wc -l | tr -d ' '
}

probe_audio_codec_at() {
  local input="$1"
  local stream_pos="$2"

  ffprobe -v error -select_streams "a:${stream_pos}" \
    -show_entries stream=codec_name \
    -of default=nk=1:nw=1 "$input" | tr -d '\r\n'
}

probe_audio_channels_at() {
  local input="$1"
  local stream_pos="$2"

  ffprobe -v error -select_streams "a:${stream_pos}" \
    -show_entries stream=channels \
    -of default=nk=1:nw=1 "$input" | tr -d '\r\n'
}

lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

is_dts_family_codec_name() {
  case "$(lower_text "$1")" in
    dca|dts|dts_*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

probe_subtitle_count() {
  ffprobe -v error -select_streams s \
    -show_entries stream=index \
    -of csv=p=0 "$1" | wc -l | tr -d ' '
}

probe_data_count() {
  ffprobe -v error -select_streams d \
    -show_entries stream=index \
    -of csv=p=0 "$1" | wc -l | tr -d ' '
}

detect_crop_at_offset_for_input() {
  local input="$1"
  local offset="$2"
  local sample_seconds="$3"
  local detect_limit="$4"

  ffmpeg -hide_banner -nostdin -ss "$offset" -t "$sample_seconds" \
    -i "$input" \
    -map 0:v:0 \
    -vf "cropdetect=limit=${detect_limit}:round=2:reset=0" \
    -an -sn -dn -f null - 2>&1 \
    | grep -Eo 'crop=[0-9]+:[0-9]+:[0-9]+:[0-9]+' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 1 \
    | awk '{print $2}'
}

detect_stable_crop_height_for_input() {
  local input="$1"
  local sample_seconds="$2"
  local detect_limit="$3"
  local duration
  local offset_0="0"
  local offset_1="0"
  local offset_2="0"
  local candidate_0=""
  local candidate_1=""
  local candidate_2=""
  local winner_line=""
  local winner_crop=""
  local crop_values=""
  local crop_h=""

  duration="$(ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 "$input" 2>/dev/null | head -n 1 | tr -d '\r')"
  if ! printf '%s' "$duration" | awk 'BEGIN{ok=0} /^[0-9]+(\.[0-9]+)?$/ {ok=1} END{exit(ok?0:1)}'; then
    duration="0"
  fi

  offset_1="$(awk -v d="$duration" 'BEGIN{printf "%.3f", (d*0.33)}')"
  offset_2="$(awk -v d="$duration" 'BEGIN{printf "%.3f", (d*0.66)}')"

  candidate_0="$(detect_crop_at_offset_for_input "$input" "$offset_0" "$sample_seconds" "$detect_limit" || true)"
  candidate_1="$(detect_crop_at_offset_for_input "$input" "$offset_1" "$sample_seconds" "$detect_limit" || true)"
  candidate_2="$(detect_crop_at_offset_for_input "$input" "$offset_2" "$sample_seconds" "$detect_limit" || true)"

  winner_line="$(
    printf '%s\n%s\n%s\n' "$candidate_0" "$candidate_1" "$candidate_2" \
      | sed '/^$/d' \
      | sort \
      | uniq -c \
      | sort -nr \
      | head -n 1
  )"
  [ -n "$winner_line" ] || return 1

  winner_crop="$(printf '%s' "$winner_line" | awk '{print $2}')"
  [ -n "$winner_crop" ] || return 1

  crop_values="${winner_crop#crop=}"
  crop_h="$(printf '%s' "$crop_values" | awk -F: '{print $2}')"
  [ -n "$crop_h" ] || return 1

  printf '%s\n' "$crop_h"
}

create_subtitle_fixture() {
  local input="$1"
  local output="$2"
  local mode="$3" # forced|forced_non_english|default
  local subtitle_file="${TMP_DIR}/subtitle_${mode}.srt"
  local subtitle_title=""
  local subtitle_disposition=""
  local subtitle_language="eng"

  case "$mode" in
    forced)
      subtitle_title="English Forced"
      subtitle_disposition="forced"
      ;;
    forced_non_english)
      subtitle_title="Spanish Forced"
      subtitle_disposition="forced"
      subtitle_language="spa"
      ;;
    default)
      subtitle_title="English Main"
      subtitle_disposition="default"
      ;;
    *)
      fail "Unsupported subtitle fixture mode: ${mode}"
      ;;
  esac

  cat > "$subtitle_file" <<EOF
1
00:00:00,200 --> 00:00:01,200
foreign dialogue

2
00:00:01,250 --> 00:00:01,900
main subtitle lane
EOF

  ffmpeg -hide_banner -nostdin -y \
    -i "$input" \
    -f srt -i "$subtitle_file" \
    -map 0:v:0 -map 0:a? -map 1:0 \
    -c:v copy -c:a copy -c:s srt \
    -metadata:s:s:0 language="$subtitle_language" \
    -metadata:s:s:0 title="$subtitle_title" \
    -disposition:s:0 "$subtitle_disposition" \
    "$output" >/dev/null 2>&1
}

run_action_assertions() {
  local action_name="$1"
  local action_script="$2"
  local input="$3"
  local output="$4"
  local max_height="$5"

  [ -x "$action_script" ] || fail "Action script is not executable: $action_script"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  local input_audio_count
  local output_audio_count
  local codec
  local height

  input_audio_count="$(probe_audio_count "$input")"

  env \
    VFO_ENCODER_MODE=cpu \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    CRF_1080=30 \
    AVG_K=4000 \
    MAXRATE_K=6000 \
    BUFSIZE_K=12000 \
    AVG_K_1080=2500 \
    MAXRATE_K_1080=3500 \
    BUFSIZE_K_1080=7000 \
    bash "$action_script" "$input" "$output"

  [ -s "$output" ] || fail "Output file missing: $output"
  ffprobe -v error "$output" >/dev/null 2>&1 || fail "ffprobe cannot read output: $output"

  codec="$(probe_video_codec "$output")"
  height="$(probe_video_height "$output")"
  output_audio_count="$(probe_audio_count "$output")"

  assert_equals "$codec" "hevc" "${action_name} should output HEVC"
  assert_int_lte "$height" "$max_height" "${action_name} output exceeds max height"
  assert_equals "$output_audio_count" "$input_audio_count" "${action_name} changed audio stream count"

  log "${action_name} passed (codec=${codec}, height=${height}, audio_streams=${output_audio_count})"
}

run_main_subtitle_action_assertions() {
  local action_name="$1"
  local action_script="$2"
  local input="$3"
  local requested_output="$4"
  local max_height="$5"
  local expected_container="$6" # mp4|mkv
  local expected_subtitle_count="$7"
  local include_default_main_sub="${8:-0}"
  local expected_output="$requested_output"

  [ -x "$action_script" ] || fail "Action script is not executable: $action_script"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  if [ "$expected_container" = "mkv" ]; then
    expected_output="${requested_output%.*}.mkv"
  elif [ "$expected_container" != "mp4" ]; then
    fail "Unsupported expected container '${expected_container}' for ${action_name}"
  fi

  rm -f "$requested_output" "${requested_output%.*}.mkv"

  local input_audio_count
  local output_audio_count
  local output_subtitle_count
  local output_data_count
  local codec
  local height

  input_audio_count="$(probe_audio_count "$input")"

  env \
    VFO_ENCODER_MODE=cpu \
    VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT="$include_default_main_sub" \
    CPU_PRESET=ultrafast \
    CRF_4K=30 \
    CRF_1080=30 \
    AVG_K=4000 \
    MAXRATE_K=6000 \
    BUFSIZE_K=12000 \
    AVG_K_1080=2500 \
    MAXRATE_K_1080=3500 \
    BUFSIZE_K_1080=7000 \
    bash "$action_script" "$input" "$requested_output"

  [ -s "$expected_output" ] || fail "Output file missing: $expected_output"
  ffprobe -v error "$expected_output" >/dev/null 2>&1 || fail "ffprobe cannot read output: $expected_output"

  codec="$(probe_video_codec "$expected_output")"
  height="$(probe_video_height "$expected_output")"
  output_audio_count="$(probe_audio_count "$expected_output")"
  output_subtitle_count="$(probe_subtitle_count "$expected_output")"
  output_data_count="$(probe_data_count "$expected_output")"

  assert_equals "$codec" "hevc" "${action_name} should output HEVC"
  assert_int_lte "$height" "$max_height" "${action_name} output exceeds max height"
  assert_equals "$output_audio_count" "$input_audio_count" "${action_name} changed audio stream count"
  assert_equals "$output_subtitle_count" "$expected_subtitle_count" "${action_name} subtitle stream count mismatch"
  if [ "$expected_container" = "mp4" ]; then
    assert_equals "$output_data_count" "0" "${action_name} MP4 output should not contain data streams"
  fi

  log "${action_name} passed (codec=${codec}, height=${height}, audio_streams=${output_audio_count}, subtitle_streams=${output_subtitle_count}, data_streams=${output_data_count}, container=${expected_container})"
}

run_audio_conform_action_assertions() {
  local action_name="$1"
  local action_script="$2"
  local input="$3"
  local requested_output="$4"
  local max_height="$5"
  local expected_container="$6" # mp4|mkv
  local expected_subtitle_count="$7"
  local include_default_main_sub="${8:-0}"
  local expected_output="$requested_output"
  local input_audio_codec=""
  local input_audio_channels=""
  local output_audio_codec=""
  local expected_transcode_codec=""

  run_main_subtitle_action_assertions \
    "$action_name" \
    "$action_script" \
    "$input" \
    "$requested_output" \
    "$max_height" \
    "$expected_container" \
    "$expected_subtitle_count" \
    "$include_default_main_sub"

  if [ "$expected_container" = "mkv" ]; then
    expected_output="${requested_output%.*}.mkv"
  fi

  input_audio_codec="$(probe_audio_codec_at "$input" 0 || true)"
  output_audio_codec="$(probe_audio_codec_at "$expected_output" 0 || true)"
  input_audio_channels="$(probe_audio_channels_at "$input" 0 || true)"

  [ -n "$input_audio_codec" ] || return 0
  [ -n "$output_audio_codec" ] || fail "${action_name} lost the first audio stream"

  if is_dts_family_codec_name "$input_audio_codec"; then
    case "$input_audio_channels" in
      ''|*[!0-9]*)
        input_audio_channels="2"
        ;;
    esac
    if [ "$input_audio_channels" -le 2 ]; then
      expected_transcode_codec="aac"
    elif ffmpeg -hide_banner -encoders 2>/dev/null | awk 'NF >= 2 { print $2 }' | grep -qx 'eac3'; then
      expected_transcode_codec="eac3"
    else
      expected_transcode_codec="ac3"
    fi
    assert_equals "$output_audio_codec" "$expected_transcode_codec" "${action_name} should conform DTS-family input to the expected delivery codec"
  else
    assert_equals "$output_audio_codec" "$input_audio_codec" "${action_name} should preserve non-DTS first audio codec"
  fi

  log "${action_name} audio conform passed (input_audio=${input_audio_codec}, output_audio=${output_audio_codec})"
}

run_legacy_main_subtitle_autocrop_assertions() {
  local action_name="$1"
  local input="$2"
  local requested_output="$3"
  local expected_container="$4" # mp4|mkv
  local expected_subtitle_count="$5"
  local include_default_main_sub="${6:-0}"
  local input_height="$7"
  local expected_output="$requested_output"

  [ -x "$ACTION_MAIN_SUB_LEGACY" ] || fail "Action script is not executable: $ACTION_MAIN_SUB_LEGACY"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  if [ "$expected_container" = "mkv" ]; then
    expected_output="${requested_output%.*}.mkv"
  elif [ "$expected_container" != "mp4" ]; then
    fail "Unsupported expected container '${expected_container}' for ${action_name}"
  fi

  rm -f "$requested_output" "${requested_output%.*}.mkv"

  local input_audio_count
  local output_audio_count
  local output_subtitle_count
  local output_data_count
  local codec
  local width
  local height
  local expected_crop_height=""
  local vertical_pixels_removed=0
  local min_vertical_removed=0

  input_audio_count="$(probe_audio_count "$input")"

  env \
    VFO_ENCODER_MODE=cpu \
    VFO_MAIN_SUBTITLE_INCLUDE_DEFAULT="$include_default_main_sub" \
    VFO_LEGACY_AUTOCROP=1 \
    VFO_LEGACY_CROP_SAMPLE_SECONDS=1 \
    VFO_LEGACY_CROP_MIN_PIXELS=8 \
    VFO_LEGACY_DEINTERLACE=off \
    CPU_PRESET=ultrafast \
    CRF_LEGACY=30 \
    AVG_K_LEGACY=1800 \
    MAXRATE_K_LEGACY=2600 \
    BUFSIZE_K_LEGACY=5200 \
    bash "$ACTION_MAIN_SUB_LEGACY" "$input" "$requested_output"

  [ -s "$expected_output" ] || fail "Output file missing: $expected_output"
  ffprobe -v error "$expected_output" >/dev/null 2>&1 || fail "ffprobe cannot read output: $expected_output"

  codec="$(probe_video_codec "$expected_output")"
  width="$(probe_video_width "$expected_output")"
  height="$(probe_video_height "$expected_output")"
  output_audio_count="$(probe_audio_count "$expected_output")"
  output_subtitle_count="$(probe_subtitle_count "$expected_output")"
  output_data_count="$(probe_data_count "$expected_output")"
  expected_crop_height="$(detect_stable_crop_height_for_input "$input" 1 24 || true)"
  min_vertical_removed=$((8 * 2))
  if [ -n "$expected_crop_height" ]; then
    vertical_pixels_removed=$((input_height - expected_crop_height))
  fi

  assert_equals "$codec" "hevc" "${action_name} should output HEVC"
  assert_int_lte "$height" "$input_height" "${action_name} output height should not exceed input height"
  assert_equals "$output_audio_count" "$input_audio_count" "${action_name} changed audio stream count"
  assert_equals "$output_subtitle_count" "$expected_subtitle_count" "${action_name} subtitle stream count mismatch"
  if [ "$expected_container" = "mp4" ]; then
    assert_equals "$output_data_count" "0" "${action_name} MP4 output should not contain data streams"
  fi
  if [ -n "$expected_crop_height" ] && [ "$vertical_pixels_removed" -ge "$min_vertical_removed" ]; then
    if [ "$height" -ge "$input_height" ]; then
      fail "${action_name} expected auto-crop to reduce height. input_height=${input_height} output_height=${height} expected_crop_height=${expected_crop_height}"
    fi
  else
    log "${action_name} crop precheck: no stable vertical crop candidate detected (input_height=${input_height}, expected_crop_height=${expected_crop_height:-none})"
  fi

  log "${action_name} passed (codec=${codec}, width=${width}, height=${height}, audio_streams=${output_audio_count}, subtitle_streams=${output_subtitle_count}, data_streams=${output_data_count}, container=${expected_container})"
}

run_guardrail_skip_action_assertions() {
  local action_name="$1"
  local input="$2"
  local output="$3"
  local reason="$4"
  local marker_path="${output%.*}.guardrail_skipped.txt"

  [ -x "$ACTION_GUARDRAIL_SKIP" ] || fail "Action script is not executable: $ACTION_GUARDRAIL_SKIP"
  [ -s "$input" ] || fail "Input fixture missing: $input"

  rm -f "$output" "${output%.*}.mkv" "$marker_path"

  bash "$ACTION_GUARDRAIL_SKIP" "$input" "$output" "$reason"

  [ -f "$marker_path" ] || fail "${action_name} did not create guardrail skip marker: $marker_path"
  [ ! -f "$output" ] || fail "${action_name} unexpectedly created output media: $output"

  grep -q "reason=${reason}" "$marker_path" || fail "${action_name} marker missing reason line"
  log "${action_name} passed (marker=${marker_path})"
}

run_seed_from_input() {
  local seed_index="$1"
  local seed_asset="$2"
  local fixture_1080="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080.mkv"
  local fixture_2160="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160.mkv"
  local fixture_1080_audio_conform="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform.mkv"
  local fixture_2160_audio_conform="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_audio_conform.mkv"
  local fixture_1080_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_forced_sub.mkv"
  local fixture_2160_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_forced_sub.mkv"
  local fixture_1080_default_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_default_sub.mkv"
  local fixture_1080_forced_non_english_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_forced_non_english_sub.mkv"
  local fixture_legacy_letterbox="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_letterbox.mkv"
  local fixture_legacy_letterbox_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_letterbox_forced_sub.mkv"
  local fixture_1080_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform_forced_sub.mkv"
  local fixture_1080_audio_conform_default_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform_default_sub.mkv"
  local fixture_2160_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_audio_conform_forced_sub.mkv"
  local fixture_legacy_audio_conform_letterbox="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_audio_conform_letterbox.mkv"
  local fixture_legacy_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_audio_conform_forced_sub.mkv"
  local output_1080="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_output.mkv"
  local output_4k="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_output.mkv"
  local output_1080_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_main_sub.mp4"
  local output_4k_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_main_sub.mp4"
  local output_1080_audio_conform_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_audio_conform_main_sub.mp4"
  local output_1080_audio_conform_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_audio_conform_default_sub_off.mp4"
  local output_4k_audio_conform_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_audio_conform_main_sub.mp4"
  local output_4k_audio_conform_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_audio_conform_default_sub_off.mp4"
  local output_1080_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_default_sub_off.mp4"
  local output_1080_default_sub_on="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_default_sub_on.mp4"
  local output_1080_forced_non_english_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_forced_non_english_sub.mp4"
  local output_legacy_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_default_sub_off.mp4"
  local output_legacy_forced_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_forced_sub.mp4"
  local output_legacy_audio_conform_forced_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_audio_conform_forced_sub.mp4"
  local output_guardrail_skip="${OUTPUTS_DIR}/seed_${seed_index}_profile_guardrail_skip.mp4"
  local run_sdr_lanes=1

  WEB_APP_SELECTED_ASSET="$(basename "$seed_asset")"
  WEB_APP_ASSET_STATUS="Complete"

  log "Using local open-source asset seed #${seed_index}: ${seed_asset}"
  create_fixture_from_input "$seed_asset" 1920 1080 "$fixture_1080"
  create_fixture_from_input "$seed_asset" 3840 2160 "$fixture_2160"
  create_audio_conform_fixture_from_input "$seed_asset" 1920 1080 "$fixture_1080_audio_conform"
  create_audio_conform_fixture_from_input "$seed_asset" 3840 2160 "$fixture_2160_audio_conform"

  if ! should_run_sdr_lanes_for_input "$fixture_1080"; then
    run_sdr_lanes=0
    log "Seed ${seed_index}: skipping SDR-only 1080/legacy lanes because fixture transfer is HDR ($(probe_video_color_transfer "$fixture_1080"))"
  fi

  create_subtitle_fixture "$fixture_2160" "$fixture_2160_forced_sub" forced
  create_subtitle_fixture "$fixture_2160_audio_conform" "$fixture_2160_audio_conform_forced_sub" forced

  if [ "$run_sdr_lanes" = "1" ]; then
    create_subtitle_fixture "$fixture_1080" "$fixture_1080_forced_sub" forced
    create_subtitle_fixture "$fixture_1080" "$fixture_1080_default_sub" default
    create_subtitle_fixture "$fixture_1080" "$fixture_1080_forced_non_english_sub" forced_non_english
    create_legacy_letterbox_fixture "$fixture_1080" "$fixture_legacy_letterbox"
    create_subtitle_fixture "$fixture_legacy_letterbox" "$fixture_legacy_letterbox_forced_sub" forced

    create_subtitle_fixture "$fixture_1080_audio_conform" "$fixture_1080_audio_conform_forced_sub" forced
    create_subtitle_fixture "$fixture_1080_audio_conform" "$fixture_1080_audio_conform_default_sub" default
    create_legacy_letterbox_fixture "$fixture_1080_audio_conform" "$fixture_legacy_audio_conform_letterbox"
    create_subtitle_fixture "$fixture_legacy_audio_conform_letterbox" "$fixture_legacy_audio_conform_forced_sub" forced
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_action_assertions \
      "hevc_1080_profile_action(seed_${seed_index})" \
      "$ACTION_1080" \
      "$fixture_1080" \
      "$output_1080" \
      1080
  fi

  run_action_assertions \
    "hevc_4k_profile_action(seed_${seed_index})" \
    "$ACTION_4K" \
    "$fixture_2160" \
    "$output_4k" \
    2160

  if [ "$run_sdr_lanes" = "1" ]; then
    run_main_subtitle_action_assertions \
      "hevc_1080_main_subtitle_forced(seed_${seed_index})" \
      "$ACTION_MAIN_SUB_1080" \
      "$fixture_1080_forced_sub" \
      "$output_1080_main_sub" \
      1080 \
      mkv \
      1 \
      0
  fi

  run_main_subtitle_action_assertions \
    "hevc_4k_main_subtitle_forced(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_4K" \
    "$fixture_2160_forced_sub" \
    "$output_4k_main_sub" \
    2160 \
    mkv \
    1 \
    0

  if [ "$run_sdr_lanes" = "1" ]; then
    run_audio_conform_action_assertions \
      "hevc_1080_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
      "$ACTION_SMART_ENG_AUDIO_CONFORM_1080" \
      "$fixture_1080_audio_conform_forced_sub" \
      "$output_1080_audio_conform_main_sub" \
      1080 \
      mkv \
      1 \
      0
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_main_subtitle_action_assertions \
      "hevc_1080_main_subtitle_default_off(seed_${seed_index})" \
      "$ACTION_MAIN_SUB_1080" \
      "$fixture_1080_default_sub" \
      "$output_1080_default_sub_off" \
      1080 \
      mp4 \
      0 \
      0
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_audio_conform_action_assertions \
      "hevc_1080_smart_eng_sub_audio_conform_default_off(seed_${seed_index})" \
      "$ACTION_SMART_ENG_AUDIO_CONFORM_1080" \
      "$fixture_1080_audio_conform_default_sub" \
      "$output_1080_audio_conform_default_sub_off" \
      1080 \
      mp4 \
      0 \
      0
  fi

  run_audio_conform_action_assertions \
    "hevc_4k_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_4K" \
    "$fixture_2160_audio_conform_forced_sub" \
    "$output_4k_audio_conform_main_sub" \
    2160 \
    mkv \
    1 \
    0

  run_audio_conform_action_assertions \
    "hevc_4k_smart_eng_sub_audio_conform_default_off(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_4K" \
    "$fixture_2160_audio_conform" \
    "$output_4k_audio_conform_default_sub_off" \
    2160 \
    mp4 \
    0 \
    0

  if [ "$run_sdr_lanes" = "1" ]; then
    run_main_subtitle_action_assertions \
      "hevc_1080_main_subtitle_default_on(seed_${seed_index})" \
      "$ACTION_MAIN_SUB_1080" \
      "$fixture_1080_default_sub" \
      "$output_1080_default_sub_on" \
      1080 \
      mkv \
      1 \
      1
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_main_subtitle_action_assertions \
      "hevc_1080_main_subtitle_forced_non_english_skip(seed_${seed_index})" \
      "$ACTION_MAIN_SUB_1080" \
      "$fixture_1080_forced_non_english_sub" \
      "$output_1080_forced_non_english_sub" \
      1080 \
      mp4 \
      0 \
      0
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_legacy_main_subtitle_autocrop_assertions \
      "hevc_legacy_main_subtitle_default_off(seed_${seed_index})" \
      "$fixture_legacy_letterbox" \
      "$output_legacy_default_sub_off" \
      mp4 \
      0 \
      0 \
      540
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_legacy_main_subtitle_autocrop_assertions \
      "hevc_legacy_main_subtitle_forced(seed_${seed_index})" \
      "$fixture_legacy_letterbox_forced_sub" \
      "$output_legacy_forced_sub" \
      mkv \
      1 \
      0 \
      540
  fi

  if [ "$run_sdr_lanes" = "1" ]; then
    run_audio_conform_action_assertions \
      "hevc_legacy_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
      "$ACTION_SMART_ENG_AUDIO_CONFORM_LEGACY" \
      "$fixture_legacy_audio_conform_forced_sub" \
      "$output_legacy_audio_conform_forced_sub" \
      540 \
      mkv \
      1 \
      0
  fi

  run_guardrail_skip_action_assertions \
    "profile_guardrail_skip_action(seed_${seed_index})" \
    "$fixture_1080" \
    "$output_guardrail_skip" \
    "e2e_guardrail_skip_seed_${seed_index}"
}

run_seed_synthetic() {
  local seed_index="$1"
  local fixture_1080="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080.mkv"
  local fixture_2160="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160.mkv"
  local fixture_1080_audio_conform="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform.mkv"
  local fixture_2160_audio_conform="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_audio_conform.mkv"
  local fixture_1080_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_forced_sub.mkv"
  local fixture_2160_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_forced_sub.mkv"
  local fixture_1080_default_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_default_sub.mkv"
  local fixture_1080_forced_non_english_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_forced_non_english_sub.mkv"
  local fixture_legacy_letterbox="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_letterbox.mkv"
  local fixture_legacy_letterbox_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_letterbox_forced_sub.mkv"
  local fixture_1080_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform_forced_sub.mkv"
  local fixture_1080_audio_conform_default_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_1080_audio_conform_default_sub.mkv"
  local fixture_2160_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_2160_audio_conform_forced_sub.mkv"
  local fixture_legacy_audio_conform_letterbox="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_audio_conform_letterbox.mkv"
  local fixture_legacy_audio_conform_forced_sub="${FIXTURES_DIR}/seed_${seed_index}_mezzanine_legacy_audio_conform_forced_sub.mkv"
  local output_1080="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_output.mkv"
  local output_4k="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_output.mkv"
  local output_1080_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_main_sub.mp4"
  local output_4k_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_main_sub.mp4"
  local output_1080_audio_conform_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_audio_conform_main_sub.mp4"
  local output_1080_audio_conform_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_audio_conform_default_sub_off.mp4"
  local output_4k_audio_conform_main_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_audio_conform_main_sub.mp4"
  local output_4k_audio_conform_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_4k_audio_conform_default_sub_off.mp4"
  local output_1080_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_default_sub_off.mp4"
  local output_1080_default_sub_on="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_default_sub_on.mp4"
  local output_1080_forced_non_english_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_1080_forced_non_english_sub.mp4"
  local output_legacy_default_sub_off="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_default_sub_off.mp4"
  local output_legacy_forced_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_forced_sub.mp4"
  local output_legacy_audio_conform_forced_sub="${OUTPUTS_DIR}/seed_${seed_index}_profile_legacy_audio_conform_forced_sub.mp4"
  local output_guardrail_skip="${OUTPUTS_DIR}/seed_${seed_index}_profile_guardrail_skip.mp4"

  WEB_APP_SELECTED_ASSET="synthetic_seed_${seed_index}.mkv"
  WEB_APP_ASSET_STATUS="Complete"

  log "Using synthetic fixtures for seed #${seed_index} (no local assets required)"
  create_synthetic_fixture 1920 1080 "$fixture_1080"
  create_synthetic_fixture 3840 2160 "$fixture_2160"
  create_synthetic_fixture 1920 1080 "$fixture_1080_audio_conform"
  create_synthetic_fixture 3840 2160 "$fixture_2160_audio_conform"

  create_subtitle_fixture "$fixture_1080" "$fixture_1080_forced_sub" forced
  create_subtitle_fixture "$fixture_2160" "$fixture_2160_forced_sub" forced
  create_subtitle_fixture "$fixture_1080" "$fixture_1080_default_sub" default
  create_subtitle_fixture "$fixture_1080" "$fixture_1080_forced_non_english_sub" forced_non_english
  create_legacy_letterbox_fixture "$fixture_1080" "$fixture_legacy_letterbox"
  create_subtitle_fixture "$fixture_legacy_letterbox" "$fixture_legacy_letterbox_forced_sub" forced

  create_subtitle_fixture "$fixture_1080_audio_conform" "$fixture_1080_audio_conform_forced_sub" forced
  create_subtitle_fixture "$fixture_1080_audio_conform" "$fixture_1080_audio_conform_default_sub" default
  create_subtitle_fixture "$fixture_2160_audio_conform" "$fixture_2160_audio_conform_forced_sub" forced
  create_legacy_letterbox_fixture "$fixture_1080_audio_conform" "$fixture_legacy_audio_conform_letterbox"
  create_subtitle_fixture "$fixture_legacy_audio_conform_letterbox" "$fixture_legacy_audio_conform_forced_sub" forced

  run_action_assertions \
    "hevc_1080_profile_action(seed_${seed_index})" \
    "$ACTION_1080" \
    "$fixture_1080" \
    "$output_1080" \
    1080

  run_action_assertions \
    "hevc_4k_profile_action(seed_${seed_index})" \
    "$ACTION_4K" \
    "$fixture_2160" \
    "$output_4k" \
    2160

  run_main_subtitle_action_assertions \
    "hevc_1080_main_subtitle_forced(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_1080" \
    "$fixture_1080_forced_sub" \
    "$output_1080_main_sub" \
    1080 \
    mkv \
    1 \
    0

  run_main_subtitle_action_assertions \
    "hevc_4k_main_subtitle_forced(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_4K" \
    "$fixture_2160_forced_sub" \
    "$output_4k_main_sub" \
    2160 \
    mkv \
    1 \
    0

  run_audio_conform_action_assertions \
    "hevc_1080_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_1080" \
    "$fixture_1080_audio_conform_forced_sub" \
    "$output_1080_audio_conform_main_sub" \
    1080 \
    mkv \
    1 \
    0

  run_audio_conform_action_assertions \
    "hevc_1080_smart_eng_sub_audio_conform_default_off(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_1080" \
    "$fixture_1080_audio_conform_default_sub" \
    "$output_1080_audio_conform_default_sub_off" \
    1080 \
    mp4 \
    0 \
    0

  run_audio_conform_action_assertions \
    "hevc_4k_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_4K" \
    "$fixture_2160_audio_conform_forced_sub" \
    "$output_4k_audio_conform_main_sub" \
    2160 \
    mkv \
    1 \
    0

  run_audio_conform_action_assertions \
    "hevc_4k_smart_eng_sub_audio_conform_default_off(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_4K" \
    "$fixture_2160_audio_conform" \
    "$output_4k_audio_conform_default_sub_off" \
    2160 \
    mp4 \
    0 \
    0

  run_main_subtitle_action_assertions \
    "hevc_1080_main_subtitle_default_off(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_1080" \
    "$fixture_1080_default_sub" \
    "$output_1080_default_sub_off" \
    1080 \
    mp4 \
    0 \
    0

  run_main_subtitle_action_assertions \
    "hevc_1080_main_subtitle_default_on(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_1080" \
    "$fixture_1080_default_sub" \
    "$output_1080_default_sub_on" \
    1080 \
    mkv \
    1 \
    1

  run_main_subtitle_action_assertions \
    "hevc_1080_main_subtitle_forced_non_english_skip(seed_${seed_index})" \
    "$ACTION_MAIN_SUB_1080" \
    "$fixture_1080_forced_non_english_sub" \
    "$output_1080_forced_non_english_sub" \
    1080 \
    mp4 \
    0 \
    0

  run_legacy_main_subtitle_autocrop_assertions \
    "hevc_legacy_main_subtitle_default_off(seed_${seed_index})" \
    "$fixture_legacy_letterbox" \
    "$output_legacy_default_sub_off" \
    mp4 \
    0 \
    0 \
    540

  run_legacy_main_subtitle_autocrop_assertions \
    "hevc_legacy_main_subtitle_forced(seed_${seed_index})" \
    "$fixture_legacy_letterbox_forced_sub" \
    "$output_legacy_forced_sub" \
    mkv \
    1 \
    0 \
    540

  run_audio_conform_action_assertions \
    "hevc_legacy_smart_eng_sub_audio_conform_forced(seed_${seed_index})" \
    "$ACTION_SMART_ENG_AUDIO_CONFORM_LEGACY" \
    "$fixture_legacy_audio_conform_forced_sub" \
    "$output_legacy_audio_conform_forced_sub" \
    540 \
    mkv \
    1 \
    0

  run_guardrail_skip_action_assertions \
    "profile_guardrail_skip_action(seed_${seed_index})" \
    "$fixture_1080" \
    "$output_guardrail_skip" \
    "e2e_guardrail_skip_seed_${seed_index}"
}

run_local_asset_suite() {
  local seed_list="${TMP_DIR}/seed_assets.txt"
  local seed_asset=""
  local processed=0

  collect_video_assets "$seed_list"

  while IFS= read -r seed_asset; do
    [ -n "$seed_asset" ] || continue
    if [ "$processed" -ge "$MAX_SEEDS" ]; then
      break
    fi
    processed=$((processed + 1))
    run_seed_from_input "$processed" "$seed_asset"
  done < "$seed_list"

  if [ "$processed" -eq 0 ]; then
    if [ "$ASSET_MODE" = "auto" ]; then
        log "No local asset found in ${ASSETS_DIR}; generating synthetic CI fixtures"
      run_seed_synthetic 1
      printf '%s\n' "$WEB_APP_SELECTED_ASSET" > "$seed_list"
      processed=1
    else
      fail "ASSET_MODE=local but no video assets found in ${ASSETS_DIR}"
    fi
  fi

  log "Processed ${processed} seed asset(s)"
}

main() {
  require_command ffmpeg
  require_command ffprobe
  e2e_reset_toolchain_reports "$ROOT_DIR"
  e2e_write_toolchain_report \
    "$ROOT_DIR" \
    "run_profile_actions_e2e" \
    ffmpeg ffprobe mkvmerge mkvextract dovi_tool
  [ -f "$ACTION_4K" ] || fail "Missing action script: $ACTION_4K"
  [ -f "$ACTION_1080" ] || fail "Missing action script: $ACTION_1080"
  [ -f "$ACTION_MAIN_SUB_4K" ] || fail "Missing action script: $ACTION_MAIN_SUB_4K"
  [ -f "$ACTION_MAIN_SUB_1080" ] || fail "Missing action script: $ACTION_MAIN_SUB_1080"
  [ -f "$ACTION_MAIN_SUB_LEGACY" ] || fail "Missing action script: $ACTION_MAIN_SUB_LEGACY"
  [ -f "$ACTION_SMART_ENG_AUDIO_CONFORM_4K" ] || fail "Missing action script: $ACTION_SMART_ENG_AUDIO_CONFORM_4K"
  [ -f "$ACTION_SMART_ENG_AUDIO_CONFORM_1080" ] || fail "Missing action script: $ACTION_SMART_ENG_AUDIO_CONFORM_1080"
  [ -f "$ACTION_SMART_ENG_AUDIO_CONFORM_LEGACY" ] || fail "Missing action script: $ACTION_SMART_ENG_AUDIO_CONFORM_LEGACY"
  [ -f "$ACTION_GUARDRAIL_SKIP" ] || fail "Missing action script: $ACTION_GUARDRAIL_SKIP"
  [ -f "$VALIDATE_AUDIO_CONFORM_POLICY" ] || fail "Missing validation script: $VALIDATE_AUDIO_CONFORM_POLICY"

  rm -rf "$TMP_DIR"
  mkdir -p "$FIXTURES_DIR" "$OUTPUTS_DIR"
  trap cleanup EXIT

  WEB_APP_DASHBOARD_JSON="$(e2e_reports_dir "$ROOT_DIR")/run_profile_actions_e2e_web_app.json"
  WEB_APP_SELECTED_ASSET=""
  WEB_APP_ASSET_STATUS="Complete"
  WEB_APP_ASSET_MANIFEST_FILE="${ROOT_DIR}/tests/e2e/assets/open-source/mezzanine-source-set.txt"
  WEB_APP_ASSET_LIST_FILE="${TMP_DIR}/seed_assets.txt"

  assert_positive_int "$MAX_SEEDS" "VFO_E2E_MAX_SEEDS"

  log "asset_mode=${ASSET_MODE} assets_dir=${ASSETS_DIR} clip_duration=${CLIP_DURATION}s max_seeds=${MAX_SEEDS}"
  bash "$VALIDATE_AUDIO_CONFORM_POLICY"

  case "$ASSET_MODE" in
    auto|local)
      run_local_asset_suite
      WEB_APP_ASSET_LIST_FILE="${TMP_DIR}/seed_assets.txt"
      ;;
    synthetic)
      run_seed_synthetic 1
      printf '%s\n' "$WEB_APP_SELECTED_ASSET" > "$WEB_APP_ASSET_LIST_FILE"
      log "Processed 1 seed asset(s)"
      ;;
    *)
      fail "Unsupported VFO_E2E_ASSET_MODE='${ASSET_MODE}' (expected: auto|local|synthetic)"
      ;;
  esac

  e2e_write_web_app_dashboard \
    "$WEB_APP_DASHBOARD_JSON" \
    "profile-actions" \
    "Profile actions" \
    "Pipeline: Mezzanine to Profile" \
    "Run: profile actions replay" \
    "Profile actions e2e" \
    "run_profile_actions_e2e" \
    "" \
    "${WEB_APP_SELECTED_ASSET:-mezzanine_asset.mkv}" \
    "profile_actions" \
    "$WEB_APP_ASSET_STATUS" \
    "$WEB_APP_ASSET_MANIFEST_FILE" \
    "$WEB_APP_ASSET_LIST_FILE"

  log "All e2e profile action checks passed"
}

main "$@"
