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

e2e_write_web_app_dashboard() {
  local output_path="$1"
  local pipeline_id="$2"
  local pipeline_label="$3"
  local pipeline_title="$4"
  local run_label="$5"
  local source_label="$6"
  local source_workflow="$7"
  local source_run_url="$8"
  local selected_asset="$9"
  local mode="${10}"
  local asset_status="${11:-Waiting}"
  local asset_manifest_file="${12:-}"
  local asset_list_file="${13:-}"

  python3 - "$output_path" "$pipeline_id" "$pipeline_label" "$pipeline_title" "$run_label" "$source_label" "$source_workflow" "$source_run_url" "$selected_asset" "$mode" "$asset_status" "$asset_manifest_file" "$asset_list_file" <<'PY'
import copy
import json
import pathlib
import sys
from os.path import basename

out_path = pathlib.Path(sys.argv[1])
pipeline_id = sys.argv[2]
pipeline_label = sys.argv[3]
pipeline_title = sys.argv[4]
run_label = sys.argv[5]
source_label = sys.argv[6]
source_workflow = sys.argv[7]
source_run_url = sys.argv[8]
selected_asset = sys.argv[9] or "mezzanine_asset.mkv"
mode = sys.argv[10]
asset_status = sys.argv[11]
asset_manifest_file = sys.argv[12]
asset_list_file = sys.argv[13]

status_icons = {
    "complete": "✔",
    "failed": "✖",
    "running": "⏳",
    "skipped": "↷",
    "waiting": "○",
}

def icon_for(status):
    return status_icons.get(status, "○")

def stage_node(stage_id, label, subtitle, x, y):
    return {
        "id": stage_id,
        "label": label,
        "subtitle": subtitle,
        "x": x,
        "y": y,
    }

def stage_totals(stages, completed):
    return [
        {"label": stage["label"], "count": 1 if index < completed else 0}
        for index, stage in enumerate(stages)
    ]

def summary_counts(stages, completed, final=False, skipped=False):
    total = len(stages)
    if skipped:
        return [
            {"label": "Skipped", "count": 1, "icon": "↷"},
            {"label": "Complete", "count": 0, "icon": "✔"},
            {"label": "Running", "count": 0, "icon": "⏳"},
            {"label": "Waiting", "count": 0, "icon": "○"},
            {"label": "Failed", "count": 0, "icon": "✖"},
        ]

    running = 0 if final else 1
    waiting = max(total - completed - running, 0)
    return [
        {"label": "Complete", "count": completed, "icon": "✔"},
        {"label": "Failed", "count": 0, "icon": "✖"},
        {"label": "Running", "count": running, "icon": "⏳"},
        {"label": "Waiting", "count": waiting, "icon": "○"},
    ]

def assessment_rows(error_value="guardrail skip, missing toolchain, strict DV/HDR mismatch, or unknown error placeholder"):
    return [
        {"label": "Dynamic range", "value": "HDR/DV aware on 4K, SDR-gated on 1080p, broad intake on legacy sub-HD", "tone": "info"},
        {"label": "Resolution", "value": "4K / 1080p / legacy sub-HD lane family", "tone": "info"},
        {"label": "Audio codecs", "value": "preserved by default", "tone": "ok"},
        {"label": "Video codecs", "value": "HEVC transcode target", "tone": "warn"},
        {"label": "Interlacing", "value": "legacy lane only; optional deinterlace", "tone": "neutral"},
        {"label": "Volume normalisation", "value": "not applied by default", "tone": "neutral"},
        {"label": "Crop", "value": "legacy lane auto-crop enabled", "tone": "warn"},
        {"label": "Lowered video bitrate", "value": "yes", "tone": "warn"},
        {"label": "Lowered audio bitrate", "value": "no by default", "tone": "neutral"},
        {"label": "Audio transcoded", "value": "no by default", "tone": "neutral"},
        {"label": "Video transcoded", "value": "yes", "tone": "warn"},
        {"label": "Audio switched", "value": "no; stream copy preferred", "tone": "ok"},
        {"label": "Subtitle retained", "value": "smart_eng_sub + preserve", "tone": "ok"},
        {"label": "Subtitle transformed", "value": "no; preserve mode only", "tone": "neutral"},
        {"label": "Container changed", "value": "yes when the smart_eng_sub + preserve policy requires MKV, otherwise fragmented MP4", "tone": "warn"},
        {"label": "Container targets", "value": "MKV / fragmented MP4", "tone": "info"},
        {"label": "Bitrate targets", "value": "practical efficiency over source bit-for-bit preservation", "tone": "warn"},
        {"label": "Audio bitrate targets", "value": "copy/preserve unless a future audio profile says otherwise", "tone": "neutral"},
        {"label": "Overall bitrate targets", "value": "reduce video bitrate while maintaining viewing intent", "tone": "warn"},
        {"label": "Error", "value": error_value, "tone": "error"},
    ]

def make_node(node, status):
    item = copy.deepcopy(node)
    item["status"] = status
    return item

def load_asset_names(file_path):
    names = []
    if not file_path:
        return names

    path = pathlib.Path(file_path)
    if not path.is_file():
        return names

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        candidate = raw_line.strip()
        if not candidate:
            continue
        asset_name = basename(candidate)
        if asset_name not in names:
            names.append(asset_name)

    return names

manifest_assets = load_asset_names(asset_manifest_file)
discovered_assets = load_asset_names(asset_list_file)
discovered_set = set(discovered_assets)
selected_asset_name = basename(selected_asset) if selected_asset else ""

def build_asset_corpus():
    corpus = []
    seen = set()

    for asset_name in discovered_assets + manifest_assets:
        if asset_name and asset_name not in seen:
            seen.add(asset_name)
            corpus.append(asset_name)

    if not corpus and selected_asset_name:
        corpus.append(selected_asset_name)

    return corpus

corpus_assets = build_asset_corpus()
selected_asset_name = corpus_assets[0] if corpus_assets else selected_asset_name

def make_asset_entries(active_asset_name, active_status=None, completed_assets=None):
    assets = []
    completed = set(completed_assets or [])

    for asset_name in corpus_assets:
        if asset_name in completed:
            asset_state = "Complete"
            icon_state = "complete"
        elif asset_name == active_asset_name and active_status:
            asset_state = active_status
            icon_state = active_status.lower()
        elif asset_name in discovered_set:
            asset_state = "Available"
            icon_state = "available"
        else:
            asset_state = "Unavailable"
            icon_state = "unavailable"

        assets.append({
            "name": asset_name,
            "status": asset_state,
            "icon": icon_for(icon_state),
        })

    if active_asset_name and active_asset_name not in corpus_assets:
        asset_state = active_status if active_status else "Unavailable"
        assets.append({
            "name": active_asset_name,
            "status": asset_state,
            "icon": icon_for(asset_state.lower()),
        })

    return assets

def make_workflow(stages, edges, details, status_map):
    nodes = [make_node(stage, status_map.get(stage["id"], "waiting")) for stage in stages]
    workflow_details = {}
    for stage in stages:
        detail = copy.deepcopy(details[stage["id"]])
        status = status_map.get(stage["id"], "waiting")
        detail["status"] = status.capitalize()
        if status == "complete":
            detail["exitCode"] = 0
        elif status == "failed":
            detail["exitCode"] = 1
        else:
            detail["exitCode"] = None
        workflow_details[stage["id"]] = detail
    return {"nodes": nodes, "edges": copy.deepcopy(edges), "details": workflow_details}

def frame(label, stages, edges, details, completed, selected_node, asset_name, asset_index, asset_total, asset_state, completed_assets=None, final=False, skipped=False):
    status_map = {}
    if skipped:
        for stage in stages:
            status_map[stage["id"]] = "skipped"
    else:
        for index, stage in enumerate(stages):
            if final and index < completed:
                status_map[stage["id"]] = "complete"
            elif index < completed - 1:
                status_map[stage["id"]] = "complete"
            elif index == completed - 1 and not final:
                status_map[stage["id"]] = "running"
            else:
                status_map[stage["id"]] = "waiting"
        if final:
            for stage in stages:
                status_map[stage["id"]] = "complete"

    workflow = make_workflow(stages, edges, details, status_map)
    selected = asset_state if asset_state else ("Skipped" if skipped else ("Complete" if final else "Running"))
    completed_assets = list(completed_assets or [])
    return {
        "label": label,
        "delayMs": 600 if not skipped else 450,
        "selectedAsset": asset_name,
        "selectedNode": selected_node,
        "assetIndex": asset_index + 1,
        "assetTotal": asset_total,
        "assetLabel": asset_name,
        "assets": make_asset_entries(asset_name, selected, completed_assets),
        "summaryCounts": summary_counts(stages, completed, final=final, skipped=skipped),
        "stageTotals": stage_totals(stages, completed if not skipped else 0),
        "workflow": workflow,
    }

def build_asset_chapter_frames(asset_name, asset_index, asset_total, stages, edges, details, skipped=False):
    completed_assets = corpus_assets[:asset_index]
    if skipped:
        return [
            frame(
                "waiting for corpus asset",
                stages,
                edges,
                details,
                0,
                stages[0]["id"],
                asset_name,
                asset_index,
                asset_total,
                "Skipped",
                completed_assets=completed_assets,
                skipped=True,
            ),
            frame(
                "asset unavailable",
                stages,
                edges,
                details,
                0,
                stages[-1]["id"],
                asset_name,
                asset_index,
                asset_total,
                "Skipped",
                completed_assets=completed_assets,
                skipped=True,
            ),
        ]

    frames = [
        frame(
            "queued",
            stages,
            edges,
            details,
            0,
            stages[0]["id"],
            asset_name,
            asset_index,
            asset_total,
            "Waiting",
            completed_assets=completed_assets,
        ),
    ]
    for index, stage in enumerate(stages):
        frames.append(
            frame(
                f"{stage['label']} running",
                stages,
                edges,
                details,
                index + 1,
                stage["id"],
                asset_name,
                asset_index,
                asset_total,
                "Running",
                completed_assets=completed_assets,
            )
        )
    frames.append(
        frame(
            "complete",
            stages,
            edges,
            details,
            len(stages),
            stages[-1]["id"],
            asset_name,
            asset_index,
            asset_total,
            "Complete",
            completed_assets=completed_assets + [asset_name],
            final=True,
        )
    )
    return frames

def profile_mode(asset_name, asset_index, asset_total):
    stages = [
        stage_node("input", "Input", "Mezzanine ingest", 48, 146),
        stage_node("probe", "Probe", "Signal and stream summary", 280, 146),
        stage_node("deint", "Deint", "Optional cleanup / normalization", 510, 146),
        stage_node("encode", "Encode", "Primary profile action", 742, 146),
        stage_node("hls", "HLS", "Delivery packaging", 994, 236),
        stage_node("qc", "QC", "Device and quality guardrails", 770, 302),
        stage_node("metadata", "Metadata", "Sidecar manifests and notes", 512, 302),
    ]
    edges = [
        {"source": "input", "target": "probe"},
        {"source": "probe", "target": "deint"},
        {"source": "deint", "target": "encode"},
        {"source": "encode", "target": "hls"},
        {"source": "probe", "target": "metadata"},
        {"source": "encode", "target": "qc"},
    ]
    details = {
        "input": {
            "node": "Input",
            "status": "Running",
            "exitCode": None,
            "command": f"vfo mezzanine scan {asset_name}",
            "output": [f"asset={asset_name}", "result=queued"],
        },
        "probe": {
            "node": "Probe",
            "status": "Running",
            "exitCode": None,
            "command": f"ffprobe -hide_banner -show_streams {asset_name}",
            "output": ["stream.video.codec=hevc", "stream.audio.layout=5.1", "result=profile_ready"],
        },
        "deint": {
            "node": "Deint",
            "status": "Running",
            "exitCode": None,
            "command": f"bash services/vfo/actions/transcode_hevc_1080_profile.sh {asset_name} output.mkv",
            "output": ["phase=normalize", "result=in_progress"],
        },
        "encode": {
            "node": "Encode",
            "status": "Running",
            "exitCode": None,
            "command": f"bash services/vfo/actions/transcode_hevc_4k_profile.sh {asset_name} output.mkv",
            "output": ["encode=hevc", "result=in_progress"],
        },
        "hls": {
            "node": "HLS",
            "status": "Waiting",
            "exitCode": None,
            "command": f"bash services/vfo/actions/transcode_hevc_4k_main_subtitle_preserve_profile.sh {asset_name} output.mp4",
            "output": ["queued=true", "result=blocked"],
        },
        "qc": {
            "node": "QC",
            "status": "Waiting",
            "exitCode": None,
            "command": f"bash tests/e2e/validate_device_conformance.sh roku_4k {asset_name}",
            "output": ["queued=true", "result=blocked"],
        },
        "metadata": {
            "node": "Metadata",
            "status": "Waiting",
            "exitCode": None,
            "command": f"vfo metadata emit {asset_name}",
            "output": ["sidecar=queued", "result=waiting"],
        },
    }
    skipped = asset_name not in discovered_set
    frames = build_asset_chapter_frames(asset_name, asset_index, asset_total, stages, edges, details, skipped=skipped)
    return stages, edges, details, frames

def device_mode(asset_name, asset_index, asset_total):
    stages = [
        stage_node("input", "Input", "Mezzanine ingest", 48, 146),
        stage_node("probe", "Probe", "Stream and codec inspection", 300, 146),
        stage_node("encode", "Encode", "1080p and 4K conformance encodes", 560, 146),
        stage_node("validate", "Validate", "Device compatibility checks", 820, 146),
        stage_node("report", "Report", "Conformance summary", 1080, 146),
    ]
    edges = [
        {"source": "input", "target": "probe"},
        {"source": "probe", "target": "encode"},
        {"source": "encode", "target": "validate"},
        {"source": "validate", "target": "report"},
    ]
    details = {
        "input": {
            "node": "Input",
            "status": "Running",
            "exitCode": None,
            "command": f"vfo mezzanine scan {asset_name}",
            "output": [f"asset={asset_name}", "result=queued"],
        },
        "probe": {
            "node": "Probe",
            "status": "Running",
            "exitCode": None,
            "command": f"ffprobe -hide_banner -show_streams {asset_name}",
            "output": ["stream.video.codec=hevc", "stream.video.height=2160", "result=profile_ready"],
        },
        "encode": {
            "node": "Encode",
            "status": "Running",
            "exitCode": None,
            "command": f"bash services/vfo/actions/transcode_hevc_4k_profile.sh {asset_name} output.mkv",
            "output": ["encode=hevc", "result=in_progress"],
        },
        "validate": {
            "node": "Validate",
            "status": "Waiting",
            "exitCode": None,
            "command": f"bash tests/e2e/validate_device_conformance.sh roku_4k output.mkv",
            "output": ["devices=roku,fire_tv,chromecast,apple_tv", "result=queued"],
        },
        "report": {
            "node": "Report",
            "status": "Waiting",
            "exitCode": None,
            "command": "vfo report device-conformance",
            "output": ["conformance=waiting", "result=waiting"],
        },
    }
    skipped = asset_name not in discovered_set
    frames = build_asset_chapter_frames(asset_name, asset_index, asset_total, stages, edges, details, skipped=skipped)
    return stages, edges, details, frames

def dv_mode(asset_name, asset_index, asset_total):
    stages = [
        stage_node("input", "Input", "Dolby Vision mezzanine", 80, 146),
        stage_node("probe", "Probe", "DV profile and stream summary", 360, 146),
        stage_node("encode", "Encode", "4K DV profile action", 640, 146),
        stage_node("metadata", "Metadata", "DV sidecar and preservation", 920, 146),
    ]
    edges = [
        {"source": "input", "target": "probe"},
        {"source": "probe", "target": "encode"},
        {"source": "encode", "target": "metadata"},
    ]
    details = {
        "input": {
            "node": "Input",
            "status": "Running",
            "exitCode": None,
            "command": f"vfo mezzanine scan {asset_name}",
            "output": [f"asset={asset_name}", "result=queued"],
        },
        "probe": {
            "node": "Probe",
            "status": "Running",
            "exitCode": None,
            "command": f"ffprobe -hide_banner -show_streams {asset_name}",
            "output": ["stream.video.codec=hevc", "stream.video.dv_side_data=true", "result=profile_ready"],
        },
        "encode": {
            "node": "Encode",
            "status": "Running",
            "exitCode": None,
            "command": f"bash services/vfo/actions/transcode_hevc_4k_dv_profile.sh {asset_name} output.mkv",
            "output": ["dovi=retained", "result=in_progress"],
        },
        "metadata": {
            "node": "Metadata",
            "status": "Waiting",
            "exitCode": None,
            "command": f"vfo metadata emit {asset_name}",
            "output": ["dv_sidecar=queued", "result=waiting"],
        },
    }

    skipped = asset_name not in discovered_set or asset_name.startswith("DV source unavailable")
    frames = build_asset_chapter_frames(asset_name, asset_index, asset_total, stages, edges, details, skipped=skipped)
    return stages, edges, details, frames

def build_corpus_replay(mode_builder):
    corpus = corpus_assets if corpus_assets else [selected_asset_name]
    if not corpus:
        corpus = ["mezzanine_asset.mkv"]

    stages = []
    edges = []
    details = {}
    frames = []

    for asset_index, asset_name in enumerate(corpus):
        asset_stages, asset_edges, asset_details, asset_frames = mode_builder(asset_name, asset_index, len(corpus))
        if not stages:
            stages = asset_stages
            edges = asset_edges
            details = asset_details
        frames.extend(asset_frames)

    return stages, edges, details, frames

if mode == "device_conformance":
    stages, edges, details, frames = build_corpus_replay(device_mode)
elif mode == "dv_metadata":
    stages, edges, details, frames = build_corpus_replay(dv_mode)
else:
    stages, edges, details, frames = build_corpus_replay(profile_mode)

dashboard = {
    "title": "VFO Workflow Replay",
    "selectedPipelineId": pipeline_id,
    "sourceLabel": source_label,
    "sourceWorkflow": source_workflow,
    "sourceRunUrl": source_run_url,
    "sourceSet": basename(asset_manifest_file) if asset_manifest_file else "",
    "pipelines": [
        {
            "id": pipeline_id,
            "label": pipeline_label,
            "title": pipeline_title,
            "runLabel": run_label,
            "sourceLabel": source_label,
            "sourceWorkflow": source_workflow,
            "sourceRunUrl": source_run_url,
            "selectedAsset": selected_asset_name,
            "selectedNode": stages[0]["id"],
            "sourceSet": basename(asset_manifest_file) if asset_manifest_file else "",
            "assets": make_asset_entries(selected_asset_name, completed_assets=[]),
            "filters": ["All", "Failed", "Running", "Waiting", "Complete"],
            "summaryCounts": summary_counts(stages, 0, skipped=asset_status.lower() == "skipped"),
            "stageTotals": stage_totals(stages, 0),
            "assessments": assessment_rows(),
            "workflow": make_workflow(stages, edges, details, {stage["id"]: "waiting" for stage in stages}),
            "events": frames,
        }
    ],
}

out_path.write_text(json.dumps(dashboard, indent=2) + "\n", encoding="utf-8")
PY
}
