#!/usr/bin/env bash
set -euo pipefail

# Shared audio conform helpers for craigstreamy audio-aware profile actions.
#
# Policy:
# - preserve AAC and Dolby-family audio streams by default
# - preserve any non-DTS audio stream as-is unless the caller decides otherwise
# - only DTS-family streams are actively conformed in this helper
# - DTS mono/stereo -> AAC + loudnorm
# - DTS 3.0/4.0/5.0/5.1 -> E-AC-3 when available, else AC-3, preserving channel count
# - DTS > 5.1 (including many DTS:X renders) -> downmix to 5.1 E-AC-3/AC-3
# - loudness normalization is only applied when a DTS-family stream is transcoded
# - if a preserved stream is not MP4-safe, callers should force MKV output

AUDIO_CONFORM_WORK_FILE=""
AUDIO_CONFORM_HAS_STREAMS=0
AUDIO_CONFORM_FORCE_MKV=0
AUDIO_CONFORM_TRANSCODED_STREAMS=0
AUDIO_CONFORM_SUMMARY=""

AUDIO_CONFORM_TARGET_I="${VFO_AUDIO_CONFORM_TARGET_I:--14}"
AUDIO_CONFORM_TARGET_TP="${VFO_AUDIO_CONFORM_TARGET_TP:--1.5}"
AUDIO_CONFORM_TARGET_LRA="${VFO_AUDIO_CONFORM_TARGET_LRA:-11}"
AUDIO_CONFORM_SAMPLE_RATE="${VFO_AUDIO_CONFORM_SAMPLE_RATE:-48000}"
AUDIO_CONFORM_AAC_MONO_BITRATE="${VFO_AUDIO_CONFORM_AAC_MONO_BITRATE:-128k}"
AUDIO_CONFORM_AAC_STEREO_BITRATE="${VFO_AUDIO_CONFORM_AAC_STEREO_BITRATE:-256k}"
AUDIO_CONFORM_EAC3_3CH_BITRATE="${VFO_AUDIO_CONFORM_EAC3_3CH_BITRATE:-384k}"
AUDIO_CONFORM_EAC3_4CH_BITRATE="${VFO_AUDIO_CONFORM_EAC3_4CH_BITRATE:-512k}"
AUDIO_CONFORM_EAC3_5CH_BITRATE="${VFO_AUDIO_CONFORM_EAC3_5CH_BITRATE:-640k}"
AUDIO_CONFORM_EAC3_6CH_BITRATE="${VFO_AUDIO_CONFORM_EAC3_6CH_BITRATE:-640k}"
AUDIO_CONFORM_AC3_3CH_BITRATE="${VFO_AUDIO_CONFORM_AC3_3CH_BITRATE:-384k}"
AUDIO_CONFORM_AC3_4CH_BITRATE="${VFO_AUDIO_CONFORM_AC3_4CH_BITRATE:-448k}"
AUDIO_CONFORM_AC3_5CH_BITRATE="${VFO_AUDIO_CONFORM_AC3_5CH_BITRATE:-640k}"
AUDIO_CONFORM_AC3_6CH_BITRATE="${VFO_AUDIO_CONFORM_AC3_6CH_BITRATE:-640k}"

AUDIO_CONFORM_ENCODER_CACHE_READY=0
AUDIO_CONFORM_HAVE_EAC3=0

audio_conform_reset_state() {
  AUDIO_CONFORM_WORK_FILE=""
  AUDIO_CONFORM_HAS_STREAMS=0
  AUDIO_CONFORM_FORCE_MKV=0
  AUDIO_CONFORM_TRANSCODED_STREAMS=0
  AUDIO_CONFORM_SUMMARY=""
}

audio_conform_lower_text() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

audio_conform_stream_count() {
  ffprobe -v error \
    -select_streams a \
    -show_entries stream=index \
    -of csv=p=0 "$1" 2>/dev/null | wc -l | tr -d ' '
}

audio_conform_stream_value() {
  local input="$1"
  local stream_pos="$2"
  local field_name="$3"

  ffprobe -v error \
    -select_streams "a:${stream_pos}" \
    -show_entries "stream=${field_name}" \
    -of default=nw=1:nk=1 \
    "$input" 2>/dev/null | head -n 1 | tr -d '\r'
}

audio_conform_is_dts_family_codec() {
  case "$(audio_conform_lower_text "$1")" in
    dca|dts|dts_*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

audio_conform_is_mp4_safe_preserve_codec() {
  case "$(audio_conform_lower_text "$1")" in
    aac|ac3|eac3|alac|mp3)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

audio_conform_prepare_encoder_cache() {
  local encoder_dump=""

  if [ "$AUDIO_CONFORM_ENCODER_CACHE_READY" = "1" ]; then
    return 0
  fi

  encoder_dump="$(ffmpeg -hide_banner -encoders 2>/dev/null || true)"
  if printf '%s\n' "$encoder_dump" | awk 'NF >= 2 { print $2 }' | grep -qx 'eac3'; then
    AUDIO_CONFORM_HAVE_EAC3=1
  else
    AUDIO_CONFORM_HAVE_EAC3=0
  fi

  AUDIO_CONFORM_ENCODER_CACHE_READY=1
}

audio_conform_stream_target_channels() {
  local source_channels="$1"

  case "$source_channels" in
    ''|*[!0-9]*)
      source_channels="2"
      ;;
  esac

  if [ "$source_channels" -le 0 ]; then
    printf '2\n'
  elif [ "$source_channels" -le 6 ]; then
    printf '%s\n' "$source_channels"
  else
    printf '6\n'
  fi
}

audio_conform_stream_target_codec() {
  local source_channels="$1"

  case "$source_channels" in
    ''|*[!0-9]*)
      source_channels="2"
      ;;
  esac

  if [ "$source_channels" -le 2 ]; then
    printf 'aac\n'
  elif [ "$AUDIO_CONFORM_HAVE_EAC3" = "1" ]; then
    printf 'eac3\n'
  else
    printf 'ac3\n'
  fi
}

audio_conform_stream_bitrate() {
  local codec="$1"
  local channels="$2"

  case "$channels" in
    ''|*[!0-9]*)
      channels="2"
      ;;
  esac

  case "$codec" in
    aac)
      if [ "$channels" -le 1 ]; then
        printf '%s\n' "$AUDIO_CONFORM_AAC_MONO_BITRATE"
      else
        printf '%s\n' "$AUDIO_CONFORM_AAC_STEREO_BITRATE"
      fi
      ;;
    eac3)
      case "$channels" in
        1|2) printf '%s\n' "$AUDIO_CONFORM_AAC_STEREO_BITRATE" ;;
        3) printf '%s\n' "$AUDIO_CONFORM_EAC3_3CH_BITRATE" ;;
        4) printf '%s\n' "$AUDIO_CONFORM_EAC3_4CH_BITRATE" ;;
        5) printf '%s\n' "$AUDIO_CONFORM_EAC3_5CH_BITRATE" ;;
        *) printf '%s\n' "$AUDIO_CONFORM_EAC3_6CH_BITRATE" ;;
      esac
      ;;
    ac3)
      case "$channels" in
        1|2) printf '%s\n' "$AUDIO_CONFORM_AAC_STEREO_BITRATE" ;;
        3) printf '%s\n' "$AUDIO_CONFORM_AC3_3CH_BITRATE" ;;
        4) printf '%s\n' "$AUDIO_CONFORM_AC3_4CH_BITRATE" ;;
        5) printf '%s\n' "$AUDIO_CONFORM_AC3_5CH_BITRATE" ;;
        *) printf '%s\n' "$AUDIO_CONFORM_AC3_6CH_BITRATE" ;;
      esac
      ;;
    *)
      printf '%s\n' "$AUDIO_CONFORM_AAC_STEREO_BITRATE"
      ;;
  esac
}

audio_conform_append_summary_line() {
  local line="$1"

  if [ -z "$AUDIO_CONFORM_SUMMARY" ]; then
    AUDIO_CONFORM_SUMMARY="$line"
  else
    AUDIO_CONFORM_SUMMARY="${AUDIO_CONFORM_SUMMARY}
$line"
  fi
}

audio_conform_render_audio_work() {
  local input="$1"
  local output="$2"
  local stream_count=""
  local pos=0
  local out_pos=0
  local codec=""
  local profile=""
  local channels=""
  local target_codec=""
  local target_channels=""
  local bitrate=""
  local safe_codec_display=""
  local safe_profile_display=""
  local -a ffmpeg_args=()
  local -a map_args=()
  local -a stream_args=()

  audio_conform_reset_state
  audio_conform_prepare_encoder_cache

  stream_count="$(audio_conform_stream_count "$input")"
  case "$stream_count" in
    ''|*[!0-9]*)
      stream_count="0"
      ;;
  esac
  if [ "$stream_count" -eq 0 ]; then
    echo "Audio conform: no audio streams detected; video-only output path remains valid"
    return 0
  fi

  ffmpeg_args=(-hide_banner -nostdin -y)
  if [ -n "${PROBE_SIZE:-}" ]; then
    ffmpeg_args+=(-probesize "$PROBE_SIZE")
  fi
  if [ -n "${ANALYZE_DUR:-}" ]; then
    ffmpeg_args+=(-analyzeduration "$ANALYZE_DUR")
  fi
  ffmpeg_args+=(-i "$input")

  while [ "$pos" -lt "$stream_count" ]; do
    codec="$(audio_conform_stream_value "$input" "$pos" "codec_name")"
    profile="$(audio_conform_stream_value "$input" "$pos" "profile")"
    channels="$(audio_conform_stream_value "$input" "$pos" "channels")"
    [ -n "$codec" ] || codec="unknown"
    [ -n "$profile" ] || profile="unknown"
    case "$channels" in
      ''|*[!0-9]*)
        channels="2"
        ;;
    esac

    safe_codec_display="$(audio_conform_lower_text "$codec")"
    safe_profile_display="$(audio_conform_lower_text "$profile")"

    map_args+=(-map "0:a:${pos}")
    if audio_conform_is_dts_family_codec "$codec"; then
      target_channels="$(audio_conform_stream_target_channels "$channels")"
      target_codec="$(audio_conform_stream_target_codec "$channels")"
      bitrate="$(audio_conform_stream_bitrate "$target_codec" "$target_channels")"

      stream_args+=(
        "-c:a:${out_pos}" "$target_codec"
        "-ac:a:${out_pos}" "$target_channels"
        "-ar:a:${out_pos}" "$AUDIO_CONFORM_SAMPLE_RATE"
        "-b:a:${out_pos}" "$bitrate"
        "-filter:a:${out_pos}" "loudnorm=I=${AUDIO_CONFORM_TARGET_I}:TP=${AUDIO_CONFORM_TARGET_TP}:LRA=${AUDIO_CONFORM_TARGET_LRA}"
      )
      AUDIO_CONFORM_TRANSCODED_STREAMS=$((AUDIO_CONFORM_TRANSCODED_STREAMS + 1))
      audio_conform_append_summary_line \
        "a:${pos} ${safe_codec_display}/${safe_profile_display}/${channels}ch -> ${target_codec}/${target_channels}ch ${bitrate} with loudnorm"
    else
      stream_args+=("-c:a:${out_pos}" "copy")
      if ! audio_conform_is_mp4_safe_preserve_codec "$codec"; then
        AUDIO_CONFORM_FORCE_MKV=1
      fi
      audio_conform_append_summary_line \
        "a:${pos} preserved ${safe_codec_display}/${safe_profile_display}/${channels}ch"
    fi

    pos=$((pos + 1))
    out_pos=$((out_pos + 1))
  done

  echo "Audio conform plan:"
  while IFS= read -r line; do
    [ -n "$line" ] || continue
    echo "  - ${line}"
  done <<< "$AUDIO_CONFORM_SUMMARY"
  if [ "$AUDIO_CONFORM_FORCE_MKV" = "1" ]; then
    echo "Audio conform note: at least one preserved stream is not MP4-safe; final container should be MKV"
  fi

  ffmpeg "${ffmpeg_args[@]}" \
    "${map_args[@]}" \
    -vn -sn -dn \
    -map_metadata 0 \
    -map_chapters -1 \
    "${stream_args[@]}" \
    -max_muxing_queue_size 4096 \
    "$output"

  AUDIO_CONFORM_WORK_FILE="$output"
  AUDIO_CONFORM_HAS_STREAMS=1
}

audio_conform_resolve_mp4_movflags() {
  case "$1" in
    fmp4_faststart)
      printf '%s\n' "+faststart+frag_keyframe+empty_moov+default_base_moof"
      ;;
    fmp4)
      printf '%s\n' "+frag_keyframe+empty_moov+default_base_moof"
      ;;
    faststart)
      printf '%s\n' "+faststart"
      ;;
    *)
      echo "Invalid VFO_MP4_STREAM_MODE value: $1"
      echo "Expected one of: fmp4_faststart, fmp4, faststart"
      return 1
      ;;
  esac
}

audio_conform_finalize_streamable_mp4() {
  local input_video_mp4="$1"
  local input_audio_media="$2"
  local output_mp4="$3"
  local stream_mode="$4"
  local movflags=""

  movflags="$(audio_conform_resolve_mp4_movflags "$stream_mode")"
  echo "Finalizing MP4 stream packaging mode=${stream_mode} movflags=${movflags}"

  if [ -n "$input_audio_media" ] && [ -s "$input_audio_media" ]; then
    ffmpeg -hide_banner -nostdin -y \
      -i "$input_video_mp4" -i "$input_audio_media" \
      -map 0:v:0 -map 1:a? \
      -sn -dn \
      -c copy \
      -write_tmcd 0 \
      -movflags "$movflags" \
      -max_muxing_queue_size 4096 \
      "$output_mp4"
  else
    ffmpeg -hide_banner -nostdin -y \
      -i "$input_video_mp4" \
      -map 0:v:0 \
      -an -sn -dn \
      -c copy \
      -write_tmcd 0 \
      -movflags "$movflags" \
      -max_muxing_queue_size 4096 \
      "$output_mp4"
  fi
}

audio_conform_mux_mkv() {
  local input_video_media="$1"
  local input_audio_media="$2"
  local subtitle_source_media="$3"
  local subtitle_pos="$4"
  local output_mkv="$5"
  local next_input_index=1
  local subtitle_input_index=0
  local -a cmd=()
  local -a maps=()

  cmd=(-hide_banner -nostdin -y -i "$input_video_media")
  maps=(-map 0:v:0)

  if [ -n "$input_audio_media" ] && [ -s "$input_audio_media" ]; then
    cmd+=(-i "$input_audio_media")
    maps+=(-map "${next_input_index}:a?")
    next_input_index=$((next_input_index + 1))
  fi

  if [ -n "$subtitle_pos" ] && [ -n "$subtitle_source_media" ]; then
    subtitle_input_index="$next_input_index"
    cmd+=(-i "$subtitle_source_media")
    maps+=(-map "${subtitle_input_index}:s:${subtitle_pos}")
  fi

  ffmpeg "${cmd[@]}" \
    "${maps[@]}" \
    -c copy \
    -max_muxing_queue_size 4096 \
    "$output_mkv"
}
