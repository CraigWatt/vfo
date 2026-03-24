#!/usr/bin/env bash

e2e_reports_dir() {
  local root_dir="$1"
  printf '%s/tests/e2e/.reports/latest\n' "$root_dir"
}

e2e_reset_toolchain_reports() {
  local root_dir="$1"
  local report_dir
  report_dir="$(e2e_reports_dir "$root_dir")"
  rm -rf "$report_dir"
  mkdir -p "$report_dir"
}

e2e_markdown_escape() {
  printf '%s' "$1" \
    | tr '\n' ' ' \
    | sed -E 's/[[:space:]]+/ /g' \
    | sed -E 's/^[[:space:]]+|[[:space:]]+$//g' \
    | sed 's/|/\\|/g'
}

e2e_strip_ansi() {
  local esc
  esc="$(printf '\033')"
  printf '%s' "$1" | sed "s/${esc}\\[[0-9;]*m//g"
}

e2e_resolve_vfo_version() {
  local root_dir="$1"
  local local_vfo="${root_dir}/services/vfo/bin/vfo"
  local version=""
  local parsed_version=""

  if [ -x "$local_vfo" ]; then
    version="$("$local_vfo" --version 2>/dev/null || true)"
  elif command -v vfo >/dev/null 2>&1; then
    version="$(vfo --version 2>/dev/null || true)"
  fi

  version="$(e2e_strip_ansi "$version")"
  version="$(printf '%s' "$version" | tr '\n' ' ' | sed -E 's/[[:space:]]+/ /g' | sed -E 's/^[[:space:]]+|[[:space:]]+$//g')"

  # Prefer canonical version token in reports so table values stay stable.
  parsed_version="$(printf '%s' "$version" | sed -nE 's/.*Real Version:[[:space:]]*([^[:space:]]+).*/\1/p')"
  if [ -z "$parsed_version" ]; then
    parsed_version="$(printf '%s' "$version" | sed -nE 's/.*[Vv][Ff][Oo][[:space:]]+version:[[:space:]]*([^[:space:]]+).*/\1/p')"
  fi
  if [ -n "$parsed_version" ]; then
    printf '%s\n' "$parsed_version"
    return 0
  fi

  if [ -n "$version" ]; then
    printf '%s\n' "$version"
    return 0
  fi

  printf '%s\n' "not_installed_or_not_built"
}

e2e_resolve_command_version() {
  local command_name="$1"
  local version=""

  if ! command -v "$command_name" >/dev/null 2>&1; then
    printf '%s\n' "not_installed"
    return 0
  fi

  case "$command_name" in
    ffmpeg|ffprobe)
      version="$("$command_name" -hide_banner -version 2>/dev/null | head -n 1 || true)"
      ;;
    mkvmerge|mkvextract)
      version="$("$command_name" --version 2>/dev/null | head -n 1 || true)"
      ;;
    dovi_tool)
      version="$(dovi_tool --version 2>/dev/null | head -n 1 || true)"
      if [ -z "$version" ]; then
        version="$(dovi_tool -V 2>/dev/null | head -n 1 || true)"
      fi
      if [ -z "$version" ]; then
        version="$(dovi_tool -h 2>/dev/null | head -n 1 || true)"
      fi
      ;;
    *)
      version="$("$command_name" --version 2>/dev/null | head -n 1 || true)"
      ;;
  esac

  version="$(e2e_strip_ansi "$version")"
  version="$(printf '%s' "$version" | tr '\n' ' ' | sed -E 's/[[:space:]]+/ /g' | sed -E 's/^[[:space:]]+|[[:space:]]+$//g')"
  if [ -n "$version" ]; then
    printf '%s\n' "$version"
    return 0
  fi

  printf '%s\n' "installed_version_unavailable"
}

e2e_version_is_available() {
  local version="$1"

  case "$version" in
    ""|not_installed|not_installed_or_not_built|installed_version_unavailable)
      return 1
      ;;
    *)
      return 0
      ;;
  esac
}

e2e_detect_libvmaf() {
  if ! command -v ffmpeg >/dev/null 2>&1; then
    printf '%s\n' "not_available"
    return 0
  fi

  if ffmpeg -hide_banner -filters 2>/dev/null | grep -E -q '(^| )libvmaf( |$)'; then
    printf '%s\n' "available"
    return 0
  fi

  printf '%s\n' "missing"
}

e2e_tier_base_outcome() {
  local ffmpeg_version="$1"
  local ffprobe_version="$2"
  local mkvmerge_version="$3"
  local missing=""

  if ! e2e_version_is_available "$ffmpeg_version"; then
    missing="ffmpeg"
  fi
  if ! e2e_version_is_available "$ffprobe_version"; then
    if [ -n "$missing" ]; then
      missing="${missing},ffprobe"
    else
      missing="ffprobe"
    fi
  fi
  if ! e2e_version_is_available "$mkvmerge_version"; then
    if [ -n "$missing" ]; then
      missing="${missing},mkvmerge"
    else
      missing="mkvmerge"
    fi
  fi

  if [ -z "$missing" ]; then
    printf '%s\n' "pass (ffmpeg+ffprobe+mkvmerge ready)"
  else
    printf '%s\n' "fail (missing: ${missing})"
  fi
}

e2e_tier_dv_outcome() {
  local dovi_tool_version="$1"

  if e2e_version_is_available "$dovi_tool_version"; then
    printf '%s\n' "pass (dovi_tool available)"
    return 0
  fi

  printf '%s\n' "warn (dovi_tool missing)"
}

e2e_tier_quality_outcome() {
  local ffmpeg_version="$1"
  local libvmaf_status="$2"

  if ! e2e_version_is_available "$ffmpeg_version"; then
    printf '%s\n' "fail (ffmpeg missing)"
    return 0
  fi

  if [ "$libvmaf_status" = "available" ]; then
    printf '%s\n' "pass (PSNR+SSIM+VMAF ready)"
    return 0
  fi

  printf '%s\n' "warn (PSNR+SSIM only; libvmaf unavailable)"
}

e2e_write_toolchain_report() {
  local root_dir="$1"
  local suite_name="$2"
  shift 2
  local report_dir
  local tsv_path
  local md_path
  local generated_at
  local tools=()
  local tool=""
  local version=""
  local ffmpeg_version=""
  local ffprobe_version=""
  local mkvmerge_version=""
  local dovi_tool_version=""
  local libvmaf_status=""
  local tier_base=""
  local tier_dv=""
  local tier_quality=""

  report_dir="$(e2e_reports_dir "$root_dir")"
  mkdir -p "$report_dir"

  tsv_path="${report_dir}/${suite_name}_toolchain_versions.tsv"
  md_path="${report_dir}/${suite_name}_toolchain_versions.md"
  generated_at="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

  tools+=("vfo")
  for tool in "$@"; do
    [ -n "$tool" ] || continue
    case " ${tools[*]} " in
      *" ${tool} "*) ;;
      *) tools+=("$tool") ;;
    esac
  done

  {
    printf 'generated_at_utc\t%s\n' "$generated_at"
    printf 'tool\tversion\n'
    for tool in "${tools[@]}"; do
      if [ "$tool" = "vfo" ]; then
        version="$(e2e_resolve_vfo_version "$root_dir")"
      else
        version="$(e2e_resolve_command_version "$tool")"
      fi
      printf '%s\t%s\n' "$tool" "$version"
      case "$tool" in
        ffmpeg) ffmpeg_version="$version" ;;
        ffprobe) ffprobe_version="$version" ;;
        mkvmerge) mkvmerge_version="$version" ;;
        dovi_tool) dovi_tool_version="$version" ;;
      esac
    done

    [ -n "$ffmpeg_version" ] || ffmpeg_version="$(e2e_resolve_command_version ffmpeg)"
    [ -n "$ffprobe_version" ] || ffprobe_version="$(e2e_resolve_command_version ffprobe)"
    [ -n "$mkvmerge_version" ] || mkvmerge_version="$(e2e_resolve_command_version mkvmerge)"
    [ -n "$dovi_tool_version" ] || dovi_tool_version="$(e2e_resolve_command_version dovi_tool)"

    libvmaf_status="$(e2e_detect_libvmaf)"
    tier_base="$(e2e_tier_base_outcome "$ffmpeg_version" "$ffprobe_version" "$mkvmerge_version")"
    tier_dv="$(e2e_tier_dv_outcome "$dovi_tool_version")"
    tier_quality="$(e2e_tier_quality_outcome "$ffmpeg_version" "$libvmaf_status")"

    printf 'tier.base\t%s\n' "$tier_base"
    printf 'tier.dv\t%s\n' "$tier_dv"
    printf 'tier.quality\t%s\n' "$tier_quality"
  } > "$tsv_path"

  {
    printf '# E2E Toolchain Versions: `%s`\n\n' "$suite_name"
    printf -- '- Generated at (UTC): `%s`\n\n' "$generated_at"
    printf '| Tool | Version |\n'
    printf '| --- | --- |\n'
    while IFS=$'\t' read -r tool version; do
      if [ "$tool" = "generated_at_utc" ] || [ "$tool" = "tool" ]; then
        continue
      fi
      printf '| `%s` | `%s` |\n' "$tool" "$(e2e_markdown_escape "$version")"
    done < "$tsv_path"
  } > "$md_path"

  e2e_write_toolchain_summary "$root_dir"
}

e2e_write_toolchain_summary() {
  local root_dir="$1"
  local report_dir
  local summary_path
  local tsv_file
  local suite_name
  local generated_at
  local tool
  local version

  report_dir="$(e2e_reports_dir "$root_dir")"
  mkdir -p "$report_dir"
  summary_path="${report_dir}/toolchain_versions_summary.md"

  {
    printf '# E2E Toolchain Versions (Latest Run)\n\n'
    printf 'Generated from suite reports under `tests/e2e/.reports/latest/`.\n\n'

    for tsv_file in "$report_dir"/*_toolchain_versions.tsv; do
      [ -f "$tsv_file" ] || continue
      suite_name="$(basename "$tsv_file" "_toolchain_versions.tsv")"
      generated_at="$(awk -F'\t' '$1=="generated_at_utc"{print $2; exit}' "$tsv_file")"
      printf '## `%s`\n\n' "$suite_name"
      if [ -n "$generated_at" ]; then
        printf -- '- Generated at (UTC): `%s`\n\n' "$generated_at"
      fi
      printf '| Tool | Version |\n'
      printf '| --- | --- |\n'
      while IFS=$'\t' read -r tool version; do
        if [ "$tool" = "generated_at_utc" ] || [ "$tool" = "tool" ]; then
          continue
        fi
        printf '| `%s` | `%s` |\n' "$tool" "$(e2e_markdown_escape "$version")"
      done < "$tsv_file"
      printf '\n'
    done
  } > "$summary_path"
}
