#!/usr/bin/env bash
set -euo pipefail

# Shared Dolby Vision profile 7 -> 8.1 preparation helpers.
#
# Goals:
# - detect DV profile from the source as robustly as possible
# - convert MKV profile 7 sources into a fresh P8.1 MKV before later profile stages
# - validate that the prepared output really reports as DV profile 8.x
# - keep the original mezzanine file untouched

dv_p7_to_p81_detect_dv_profile() {
  local file_path="$1"
  local profile=""

  profile="$(
    ffprobe -v error -select_streams v:0 \
      -show_entries stream_side_data \
      -of default=nw=1 "$file_path" 2>/dev/null \
      | awk -F= '/^dv_profile=/{print $2; exit}' \
      | tr -d ' \t\r\n' || true
  )"

  if [ -z "$profile" ] && command -v dovi_tool >/dev/null 2>&1; then
    profile="$(
      dovi_tool info -i "$file_path" 2>/dev/null \
        | awk '
          /[Pp]rofile/ {
            for (i = 1; i <= NF; i++) {
              if ($i ~ /^[0-9]+(\.[0-9]+)?$/) { print $i; exit }
            }
          }
        ' | head -n 1 || true
    )"
  fi

  printf '%s' "$profile" | tr -d ' \t\r\n'
}

dv_p7_to_p81_profile_is_7_family() {
  case "$1" in
    7|7.*) return 0 ;;
    *) return 1 ;;
  esac
}

dv_p7_to_p81_profile_is_8_family() {
  case "$1" in
    8|8.*) return 0 ;;
    *) return 1 ;;
  esac
}

dv_p7_to_p81_resolve_mkv_video_track_id() {
  local input_file="$1"

  mkvmerge -i "$input_file" 2>/dev/null \
    | awk -F: '/[Vv]ideo/ { gsub(/Track ID /,"",$1); gsub(/^[[:space:]]+|[[:space:]]+$/,"",$1); print $1; exit }'
}

dv_p7_to_p81_validate_output() {
  local output_file="$1"
  local detected_profile=""

  if [ ! -s "$output_file" ]; then
    echo "DV PREP ERROR: prepared output is missing or empty: $output_file" >&2
    return 1
  fi

  detected_profile="$(dv_p7_to_p81_detect_dv_profile "$output_file")"
  if dv_p7_to_p81_profile_is_8_family "$detected_profile"; then
    ffprobe -v error "$output_file" >/dev/null 2>&1
    return 0
  fi

  echo "DV PREP ERROR: expected Dolby Vision profile 8.x output, detected '${detected_profile:-unknown}' in $output_file" >&2
  return 1
}

dv_p7_to_p81_convert() {
  local input_file="$1"
  local output_file="$2"
  local conversion_mode="${VFO_DV_P7_TO_81_MODE:-2}"
  local extract_mode="${VFO_DV_P7_EXTRACT_MODE:-auto}"
  local input_profile=""
  local file_dir=""
  local base_name=""
  local tmpdir=""
  local track_id=""
  local hevc_p7=""
  local hevc_p81=""
  local extracted=0

  if [ ! -f "$input_file" ]; then
    echo "DV PREP ERROR: source file not found: $input_file" >&2
    return 1
  fi

  if ! command -v dovi_tool >/dev/null 2>&1; then
    echo "DV PREP ERROR: dovi_tool is required for P7 -> P8.1 conversion" >&2
    return 1
  fi
  if ! command -v mkvmerge >/dev/null 2>&1; then
    echo "DV PREP ERROR: mkvmerge is required for MKV DV source conversion" >&2
    return 1
  fi
  if [ "$conversion_mode" != "2" ] && [ "$conversion_mode" != "5" ]; then
    echo "DV PREP ERROR: invalid VFO_DV_P7_TO_81_MODE='${conversion_mode}' (allowed: 2 or 5)" >&2
    return 1
  fi

  input_profile="$(dv_p7_to_p81_detect_dv_profile "$input_file")"
  if ! dv_p7_to_p81_profile_is_7_family "$input_profile"; then
    echo "DV PREP ERROR: source is not Dolby Vision profile 7.x: ${input_profile:-unknown}" >&2
    return 1
  fi

  file_dir="$(dirname "$output_file")"
  base_name="$(basename "${output_file%.*}")"
  tmpdir="${file_dir}/.${base_name}.dvp81.tmp"

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  track_id="$(dv_p7_to_p81_resolve_mkv_video_track_id "$input_file" || true)"
  if [ -z "$track_id" ]; then
    echo "DV PREP ERROR: could not resolve an MKV video track id for $input_file" >&2
    rm -rf "$tmpdir"
    return 1
  fi

  hevc_p7="$tmpdir/bl_p7.hevc"
  hevc_p81="$tmpdir/video_p81.hevc"

  if [ "$extract_mode" != "ffmpeg" ] && command -v mkvextract >/dev/null 2>&1; then
    echo "DV PREP: extracting source video track ${track_id} with mkvextract" >&2
    if mkvextract tracks "$input_file" "${track_id}:${hevc_p7}" >/dev/null 2>&1 && [ -s "$hevc_p7" ]; then
      extracted=1
    elif [ "$extract_mode" = "mkvextract" ]; then
      echo "DV PREP ERROR: mkvextract failed while preparing P8.1 source" >&2
      rm -rf "$tmpdir"
      return 1
    else
      echo "DV PREP: mkvextract failed; retrying extraction with ffmpeg" >&2
    fi
  fi

  if [ "$extracted" -eq 0 ]; then
    echo "DV PREP: extracting source video track ${track_id} with ffmpeg" >&2
    if ! ffmpeg -hide_banner -nostdin -y \
      -probesize "${PROBE_SIZE:-200M}" -analyzeduration "${ANALYZE_DUR:-200M}" \
      -i "$input_file" \
      -map 0:v:0 -c:v copy -bsf:v hevc_mp4toannexb -f hevc \
      "$hevc_p7" >/dev/null 2>&1; then
      echo "DV PREP ERROR: ffmpeg extraction failed while preparing P8.1 source" >&2
      rm -rf "$tmpdir"
      return 1
    fi
    if [ ! -s "$hevc_p7" ]; then
      echo "DV PREP ERROR: extracted HEVC bitstream is empty: $hevc_p7" >&2
      rm -rf "$tmpdir"
      return 1
    fi
  fi

  echo "DV PREP: converting HEVC profile 7 bitstream to profile 8.1" >&2
  if ! dovi_tool -m "$conversion_mode" convert "$hevc_p7" -o "$hevc_p81" >/dev/null 2>&1; then
    echo "DV PREP ERROR: dovi_tool conversion to P8.1 failed" >&2
    rm -rf "$tmpdir"
    return 1
  fi
  if [ ! -s "$hevc_p81" ]; then
    echo "DV PREP ERROR: converted HEVC bitstream is empty: $hevc_p81" >&2
    rm -rf "$tmpdir"
    return 1
  fi

  echo "DV PREP: remuxing converted video with preserved MKV streams" >&2
  if ! mkvmerge -o "$output_file" --no-video "$input_file" "$hevc_p81" >/dev/null 2>&1; then
    echo "DV PREP ERROR: mkvmerge remux failed while preparing P8.1 source" >&2
    rm -rf "$tmpdir"
    return 1
  fi

  if ! dv_p7_to_p81_validate_output "$output_file"; then
    rm -rf "$tmpdir"
    return 1
  fi

  rm -rf "$tmpdir"
  echo "DV PREP: prepared P8.1 source at $output_file" >&2
  return 0
}

dv_p7_to_p81_prepare_input() {
  local input_file="$1"
  local workdir="$2"
  local detected_profile=""
  local prepared_input=""

  detected_profile="$(dv_p7_to_p81_detect_dv_profile "$input_file")"
  if ! dv_p7_to_p81_profile_is_7_family "$detected_profile"; then
    printf '%s\n' "$input_file"
    return 0
  fi

  if [ "${VFO_DV_CONVERT_P7_TO_81:-1}" != "1" ]; then
    echo "DV PREP: source is Dolby Vision profile ${detected_profile}, but preconversion is disabled; using original input" >&2
    printf '%s\n' "$input_file"
    return 0
  fi

  prepared_input="${workdir}/source_p8_1.mkv"
  if ! dv_p7_to_p81_convert "$input_file" "$prepared_input"; then
    return 1
  fi

  printf '%s\n' "$prepared_input"
}
