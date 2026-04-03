(function () {
  "use strict";

  var roots = document.querySelectorAll("[data-vfo-web-app]");
  if (!roots.length) {
    return;
  }

  var fallbackPipeline = {
    id: "demo",
    label: "Demo lane",
    title: "VFO Demo Pack",
    runLabel: "Replay transcript: 2026-04-02 18:42",
    sourceLabel: "Demo pack",
    sourceWorkflow: "Desktop + Pages view",
    sourceRunUrl: "",
    selectedAsset: "movie_01.mxf",
    selectedNode: "encode",
    assets: [
      { name: "movie_01.mxf", status: "Failed", icon: "✖" },
      { name: "movie_02.mxf", status: "Complete", icon: "✔" },
      { name: "movie_03.mxf", status: "Running", icon: "⏳" },
      { name: "movie_04.mxf", status: "Waiting", icon: "○" },
      { name: "movie_05.mxf", status: "Complete", icon: "✔" },
      { name: "movie_06.mxf", status: "Failed", icon: "✖" },
      { name: "movie_07.mxf", status: "Complete", icon: "✔" },
      { name: "movie_08.mxf", status: "Waiting", icon: "○" },
      { name: "movie_09.mxf", status: "Waiting", icon: "○" }
    ],
    filters: ["All", "Failed", "Running", "Waiting", "Complete"],
    summaryCounts: [
      { label: "Complete", count: 91, icon: "✔" },
      { label: "Failed", count: 7, icon: "✖" },
      { label: "Running", count: 12, icon: "⏳" },
      { label: "Waiting", count: 18, icon: "○" }
    ],
    stageTotals: [
      { label: "Input", count: 128 },
      { label: "Probe", count: 128 },
      { label: "Deint", count: 97 },
      { label: "Encode", count: 91 },
      { label: "HLS", count: 88 },
      { label: "QC", count: 73 },
      { label: "Metadata", count: 128 }
    ],
    workflow: {
      nodes: [
        { id: "input", label: "Input", subtitle: "Singular mezzanine asset", x: 48, y: 146, status: "complete" },
        { id: "probe", label: "Probe", subtitle: "ffprobe summary and QC hints", x: 280, y: 146, status: "complete" },
        { id: "deint", label: "Deint", subtitle: "Optional cleanup / normalization", x: 510, y: 146, status: "running" },
        { id: "encode", label: "Encode", subtitle: "Primary profile action", x: 742, y: 146, status: "failed" },
        { id: "hls", label: "HLS", subtitle: "Delivery packaging", x: 994, y: 236, status: "waiting" },
        { id: "qc", label: "QC", subtitle: "PSNR / SSIM / VMAF", x: 770, y: 302, status: "waiting" },
        { id: "metadata", label: "Metadata", subtitle: "Sidecar manifests and notes", x: 512, y: 302, status: "complete" }
      ],
      edges: [
        { source: "input", target: "probe" },
        { source: "probe", target: "deint" },
        { source: "deint", target: "encode" },
        { source: "encode", target: "hls" },
        { source: "probe", target: "metadata" },
        { source: "encode", target: "qc" }
      ],
      details: {
        input: {
          node: "Input",
          status: "Complete",
          exitCode: 0,
          command: "vfo mezzanine movie_01.mxf --scan",
          output: ["asset=movie_01.mxf", "container=mxf", "tracks=video,audio,timecode", "result=accepted"]
        },
        probe: {
          node: "Probe",
          status: "Complete",
          exitCode: 0,
          command: "ffprobe -hide_banner -show_streams movie_01.mxf",
          output: ["stream.video.codec=hevc", "stream.video.height=2160", "stream.audio.layout=5.1", "result=signal_profile_ready"]
        },
        deint: {
          node: "Deint",
          status: "Running",
          exitCode: null,
          command: "vfo run --stage deint movie_01.mxf",
          output: ["phase=normalize", "step=filter_chain", "progress=68%", "result=in_progress"]
        },
        encode: {
          node: "Encode",
          status: "Failed",
          exitCode: 1,
          command: "ffmpeg -i movie_01.mxf -c:v libx265 ...",
          output: ["error=encoder initialization failed", "stderr=codec context rejected", "hint=inspect profile guardrails", "result=failed"]
        },
        hls: {
          node: "HLS",
          status: "Waiting",
          exitCode: null,
          command: "packager --hls movie_01.mxf",
          output: ["queued=true", "depends_on=Encode", "result=blocked"]
        },
        qc: {
          node: "QC",
          status: "Waiting",
          exitCode: null,
          command: "vfo qc movie_01.mxf --reference source",
          output: ["queued=true", "depends_on=Encode", "result=waiting"]
        },
        metadata: {
          node: "Metadata",
          status: "Complete",
          exitCode: 0,
          command: "vfo metadata emit movie_01.mxf",
          output: ["sidecar=movie_01.json", "tags=preserved", "result=complete"]
        }
      }
    }
  };

  function fetchJson(url) {
    return fetch(url, { cache: "no-store" }).then(function (response) {
      if (!response.ok) {
        throw new Error("request failed");
      }
      return response.json();
    });
  }

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  function statusGlyph(status) {
    var key = String(status || "waiting").toLowerCase();
    if (key === "complete") {
      return "✔";
    }
    if (key === "running") {
      return "⏳";
    }
    if (key === "failed") {
      return "✖";
    }
    if (key === "skipped") {
      return "↷";
    }
    return "○";
  }

  function assetFilterKey(status) {
    var key = String(status || "waiting").toLowerCase();
    if (key === "available") {
      return "complete";
    }
    if (key === "unavailable") {
      return "waiting";
    }
    if (key === "skipped") {
      return "waiting";
    }
    return key;
  }

  function assetToneKey(status) {
    var key = String(status || "waiting").toLowerCase();
    if (key === "skipped") {
      return "waiting";
    }
    return key;
  }

  function assetToneClass(status) {
    return "vfo-web-app__status-" + assetToneKey(status);
  }

  function defaultAssetFilters() {
    return {
      failed: true,
      running: true,
      waiting: true,
      complete: true
    };
  }

  function normalizeAssetFilters(filters) {
    var normalized = defaultAssetFilters();
    var source = filters && typeof filters === "object" ? filters : {};
    var key;

    for (key in normalized) {
      if (Object.prototype.hasOwnProperty.call(normalized, key)) {
        normalized[key] = source[key] !== false;
      }
    }

    return normalized;
  }

  function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  function defaultCanvasState() {
    return {
      zoom: 1,
      panX: 0,
      panY: 0
    };
  }

  function normalizeCanvasState(canvas) {
    var source = canvas && typeof canvas === "object" ? canvas : {};
    return {
      zoom: clamp(typeof source.zoom === "number" ? source.zoom : 1, 0.25, 2.5),
      panX: typeof source.panX === "number" ? source.panX : 0,
      panY: typeof source.panY === "number" ? source.panY : 0
    };
  }

  function slugify(value) {
    return String(value || "pipeline")
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-+|-+$/g, "") || "pipeline";
  }

  function clonePipeline(pipeline, index) {
    var copy = JSON.parse(JSON.stringify(pipeline || fallbackPipeline));
    copy.id = copy.id || slugify(copy.label || copy.title || copy.sourceLabel || ("pipeline-" + (index + 1)));
    copy.label = copy.label || copy.title || copy.id;
    copy.selectedAsset = copy.selectedAsset || (copy.assets && copy.assets[0] && copy.assets[0].name) || "";
    copy.selectedNode = copy.selectedNode || "encode";
    copy.filters = copy.filters || fallbackPipeline.filters.slice();
    copy.summaryCounts = copy.summaryCounts || fallbackPipeline.summaryCounts.slice();
    copy.stageTotals = copy.stageTotals || fallbackPipeline.stageTotals.slice();
    copy.workflow = copy.workflow || fallbackPipeline.workflow;
    copy.title = copy.title || fallbackPipeline.title;
    copy.runLabel = copy.runLabel || fallbackPipeline.runLabel;
    copy.sourceLabel = copy.sourceLabel || fallbackPipeline.sourceLabel;
    copy.sourceWorkflow = copy.sourceWorkflow || "";
    copy.sourceRunUrl = copy.sourceRunUrl || "";
    copy.assets = Array.isArray(copy.assets) ? copy.assets : fallbackPipeline.assets.slice();
    copy.events = Array.isArray(copy.events) ? copy.events : [];
    return copy;
  }

  function normalizeDashboard(raw) {
    var dashboard = raw && typeof raw === "object" ? raw : {};
    var pipelines = [];
    var panelState = dashboard.panelState && typeof dashboard.panelState === "object" ? dashboard.panelState : {};

    if (Array.isArray(dashboard.pipelines) && dashboard.pipelines.length) {
      pipelines = dashboard.pipelines.map(function (pipeline, index) {
        return clonePipeline(pipeline, index);
      });
    } else {
      pipelines = [clonePipeline(dashboard, 0)];
    }

    return {
      title: dashboard.title || pipelines[0].title || "VFO Demo Pack",
      selectedPipelineId: dashboard.selectedPipelineId || pipelines[0].id,
      pipelines: pipelines,
      sourceLabel: dashboard.sourceLabel || pipelines[0].sourceLabel || "Demo pack",
      sourceWorkflow: dashboard.sourceWorkflow || pipelines[0].sourceWorkflow || "",
      sourceRunUrl: dashboard.sourceRunUrl || pipelines[0].sourceRunUrl || "",
      assetQuery: typeof dashboard.assetQuery === "string" ? dashboard.assetQuery : "",
      assetFilters: normalizeAssetFilters(dashboard.assetFilters),
      canvas: normalizeCanvasState(dashboard.canvas),
      panelState: {
        assets: panelState.assets !== false,
        inspector: panelState.inspector !== false
      },
      playback: null
    };
  }

  function cloneFrame(value) {
    return JSON.parse(JSON.stringify(value || {}));
  }

  function applyFrame(pipeline, frame) {
    var next = cloneFrame(frame);

    if (next.title) {
      pipeline.title = next.title;
    }
    if (next.runLabel) {
      pipeline.runLabel = next.runLabel;
    }
    if (next.sourceLabel) {
      pipeline.sourceLabel = next.sourceLabel;
    }
    if (typeof next.sourceWorkflow !== "undefined") {
      pipeline.sourceWorkflow = next.sourceWorkflow;
    }
    if (typeof next.sourceRunUrl !== "undefined") {
      pipeline.sourceRunUrl = next.sourceRunUrl;
    }
    if (typeof next.selectedAsset !== "undefined") {
      pipeline.selectedAsset = next.selectedAsset;
    }
    if (typeof next.selectedNode !== "undefined") {
      pipeline.selectedNode = next.selectedNode;
    }
    if (Array.isArray(next.assets)) {
      pipeline.assets = next.assets;
    }
    if (Array.isArray(next.filters)) {
      pipeline.filters = next.filters;
    }
    if (Array.isArray(next.summaryCounts)) {
      pipeline.summaryCounts = next.summaryCounts;
    }
    if (Array.isArray(next.stageTotals)) {
      pipeline.stageTotals = next.stageTotals;
    }
    if (next.workflow) {
      pipeline.workflow = next.workflow;
    }
  }

  function getSelectedPipeline(state) {
    var selected = state.pipelines.find(function (pipeline) {
      return pipeline.id === state.selectedPipelineId;
    });
    return selected || state.pipelines[0];
  }

  function assetMatchesSearch(asset, query) {
    if (!query) {
      return true;
    }

    return String(asset.name || "").toLowerCase().indexOf(query) !== -1;
  }

  function assetMatchesFilters(asset, filters) {
    var key = assetFilterKey(asset.status);
    var activeFilters = filters || defaultAssetFilters();
    var anySpecificFilter = false;
    var specificVisible = false;

    if (activeFilters.failed) {
      anySpecificFilter = true;
      if (key === "failed") {
        specificVisible = true;
      }
    }
    if (activeFilters.running) {
      anySpecificFilter = true;
      if (key === "running") {
        specificVisible = true;
      }
    }
    if (activeFilters.waiting) {
      anySpecificFilter = true;
      if (key === "waiting") {
        specificVisible = true;
      }
    }
    if (activeFilters.complete) {
      anySpecificFilter = true;
      if (key === "complete") {
        specificVisible = true;
      }
    }

    if (!anySpecificFilter) {
      return false;
    }

    return specificVisible;
  }

  function assetFilterSummary(pipeline, state, visibleCount) {
    var summary = pipeline.assets.length === 1
      ? "1 mezzanine source in corpus"
      : String(pipeline.assets.length) + " mezzanine sources in corpus";

    if (visibleCount !== pipeline.assets.length || (state.assetQuery && state.assetQuery.length)) {
      summary += " (" + String(visibleCount) + " shown)";
    } else {
      var availableAssets = pipeline.assets.filter(function (asset) {
        return assetFilterKey(asset.status) === "complete";
      }).length;
      if (availableAssets !== pipeline.assets.length) {
        summary += " (" + String(availableAssets) + " available)";
      }
    }

    return summary;
  }

  function buildPipelineOptions(state) {
    if (state.pipelines.length <= 1) {
      return "";
    }

    return [
      '<label class="vfo-web-app__pipeline-select">',
      '  <span>Replay lane</span>',
      '  <select data-vfo-pipeline-select>',
      state.pipelines.map(function (pipeline) {
        return '<option value="' + escapeHtml(pipeline.id) + '">' + escapeHtml(pipeline.label) + '</option>';
      }).join(""),
      "  </select>",
      "</label>"
    ].join("\n");
  }

  function buildDrawerTabs(state) {
    var panelState = state.panelState || { assets: true, inspector: true };

    return [
      '<div class="vfo-web-app__drawer-tabs" role="tablist" aria-label="Side drawers">',
      '  <button type="button" class="vfo-web-app__drawer-tab ' + (panelState.assets ? "is-active" : "") + '" data-vfo-panel-toggle="assets" aria-pressed="' + String(Boolean(panelState.assets)) + '">',
      '    Assets',
      '  </button>',
      '  <button type="button" class="vfo-web-app__drawer-tab ' + (panelState.inspector ? "is-active" : "") + '" data-vfo-panel-toggle="inspector" aria-pressed="' + String(Boolean(panelState.inspector)) + '">',
      '    Inspector',
      '  </button>',
      '</div>'
    ].join("\n");
  }

  function buildCanvasControls(state) {
    var zoom = state.canvas && typeof state.canvas.zoom === "number" ? state.canvas.zoom : 1;
    var zoomLabel = Math.round(zoom * 100) + "%";

    return [
      '<div class="vfo-web-app__workflow-toolbar">',
      '  <div class="vfo-web-app__workflow-controls">',
      '    <button type="button" data-vfo-canvas-action="fit">Fit</button>',
      '    <button type="button" data-vfo-canvas-action="reset">Reset</button>',
      '    <button type="button" data-vfo-canvas-action="zoom-out">−</button>',
      '    <button type="button" data-vfo-canvas-action="zoom-in">+</button>',
      "  </div>",
      '  <div class="vfo-web-app__workflow-scale" data-vfo-canvas-scale>' + escapeHtml(zoomLabel) + "</div>",
      "</div>"
    ].join("\n");
  }

  function buildAssetSequence(pipeline) {
    return [
      '<div class="vfo-web-app__asset-sequence" data-vfo-asset-sequence>',
      '  <div class="vfo-web-app__asset-sequence-head">',
      '    <h4>Replay order</h4>',
      '    <span>Corpus chapters in emitted sequence</span>',
      "  </div>",
      pipeline.assets.map(function (asset, index) {
        var status = String(asset.status || "available");
        return [
          '<button type="button" class="vfo-web-app__asset-chip ' + (asset.name === pipeline.selectedAsset ? "is-active" : "") + '" data-vfo-asset-sequence-item data-asset="' + escapeHtml(asset.name) + '">',
          '  <span class="vfo-web-app__asset-chip-order">#' + String(index + 1).padStart(2, "0") + "</span>",
          '  <span class="vfo-web-app__asset-chip-name">' + escapeHtml(asset.name) + "</span>",
          '  <span class="vfo-web-app__asset-chip-status"><span class="vfo-web-app__asset-dot ' + assetToneClass(status) + '"></span>' + escapeHtml(status) + "</span>",
          "</button>"
        ].join("\n");
      }).join(""),
      "</div>"
    ].join("\n");
  }

  function buildReplayMeter(state) {
    var playback = state.playback;
    if (!playback || !playback.total) {
      return "";
    }

    var chapterLabel = playback.assetIndex && playback.assetTotal
      ? "Asset " + String(playback.assetIndex).padStart(2, "0") + " / " + String(playback.assetTotal).padStart(2, "0")
      : "Step " + String(playback.index).padStart(2, "0") + " / " + String(playback.total).padStart(2, "0");
    var frameLabel = "Frame " + String(playback.index).padStart(2, "0") + " / " + String(playback.total).padStart(2, "0");

    return [
      '<div class="vfo-web-app__replay-meter">',
      '  <span>Transcript</span>',
      '  <strong>' + escapeHtml(chapterLabel) + "</strong>",
      "  <em>" + escapeHtml((playback.assetLabel || playback.label || "emit stream") + " · " + frameLabel) + "</em>",
      "</div>"
    ].join("\n");
  }

  function render(container, state) {
    var selectedPipeline = getSelectedPipeline(state);
    var pipelineOptions = buildPipelineOptions(state);
    var drawerTabs = buildDrawerTabs(state);
    var canvasControls = buildCanvasControls(state);
    var replayMeter = buildReplayMeter(state);
    var headerTitle = state.title || "VFO Demo Pack";
    var headerSubtitle = selectedPipeline.title === headerTitle
      ? selectedPipeline.runLabel
      : selectedPipeline.title + "  |  " + selectedPipeline.runLabel;
    var liveAttr = selectedPipeline.sourceRunUrl ? ' data-live="1"' : "";
    var sourceWorkflowHtml = selectedPipeline.sourceWorkflow
      ? '<strong>' + escapeHtml(selectedPipeline.sourceWorkflow) + '</strong>'
      : "";
    var sourceRunHtml = selectedPipeline.sourceRunUrl
      ? '<a href="' + escapeHtml(selectedPipeline.sourceRunUrl) + '" target="_blank" rel="noreferrer">source run</a>'
      : "";
    var assetsCollapsed = state.panelState && state.panelState.assets === false;
    var inspectorCollapsed = state.panelState && state.panelState.inspector === false;

    container.dataset.assetsCollapsed = assetsCollapsed ? "1" : "0";
    container.dataset.inspectorCollapsed = inspectorCollapsed ? "1" : "0";
    container.style.setProperty("--vfo-assets-width", assetsCollapsed ? "0px" : "19rem");
    container.style.setProperty("--vfo-inspector-width", inspectorCollapsed ? "0px" : "20rem");

    container.innerHTML = [
      '<div class="vfo-web-app__shell">',
      '  <header class="vfo-web-app__topbar">',
      '    <div>',
      '      <h2>' + escapeHtml(headerTitle) + "</h2>",
      '      <p>' + escapeHtml(headerSubtitle) + "</p>",
      "    </div>",
      '    <div class="vfo-web-app__topbar-actions">',
      pipelineOptions,
      replayMeter,
      '      <div class="vfo-web-app__source"' + liveAttr + ' data-vfo-web-app-source>',
      '        <span>' + escapeHtml(selectedPipeline.sourceLabel) + "</span>",
      sourceWorkflowHtml,
      sourceRunHtml,
      "      </div>",
      "    </div>",
      "  </header>",
      drawerTabs,
      '  <section class="vfo-web-app__workspace">',
      '    <aside class="vfo-web-app__panel vfo-web-app__assets' + (assetsCollapsed ? " is-collapsed" : "") + '" data-panel="assets">',
      '      <div class="vfo-web-app__panel-head"><h3>Assets</h3><span data-vfo-asset-summary></span></div>',
      '      <label class="vfo-web-app__search">',
      '        <span>Search assets...</span>',
      '        <input type="text" value="' + escapeHtml(state.assetQuery || "") + '" aria-label="Search assets" data-vfo-asset-search />',
      "      </label>",
      buildAssetSequence(selectedPipeline),
      '      <div class="vfo-web-app__asset-list"></div>',
      '      <div class="vfo-web-app__filter-block">',
        "        <h4>Filters</h4>",
      '        <div class="vfo-web-app__filter-list"></div>',
      "      </div>",
      "    </aside>",
      '    <section class="vfo-web-app__panel vfo-web-app__workflow">',
      '      <div class="vfo-web-app__panel-head"><h3>Workflow</h3><span>Diagram canvas with pan and zoom</span></div>',
      canvasControls,
      '      <div class="vfo-web-app__workflow-shell" data-vfo-canvas-shell>',
      '        <div class="vfo-web-app__workflow-stage">',
      '          <svg class="vfo-web-app__workflow-edges" aria-hidden="true">',
      '            <defs>',
      '              <marker id="vfo-arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">',
      '                <path d="M 0 0 L 10 5 L 0 10 z"></path>',
      "              </marker>",
      "            </defs>",
      "          </svg>",
      '          <div class="vfo-web-app__workflow-nodes"></div>',
      "        </div>",
      "      </div>",
      '      <div class="vfo-web-app__caption">The canvas shows the current chapter state for the selected replay step, with connectors, status badges, and pan/zoom controls.</div>',
      "    </section>",
      '    <aside class="vfo-web-app__panel vfo-web-app__inspector' + (inspectorCollapsed ? " is-collapsed" : "") + '" data-panel="inspector">',
      '      <div class="vfo-web-app__panel-head"><h3>Inspector</h3><span>Current transcript step</span></div>',
      '      <div class="vfo-web-app__density-map"></div>',
      '      <div class="vfo-web-app__inspector-card"></div>',
      '      <div class="vfo-web-app__code-panel">',
      '        <div class="vfo-web-app__code-head"><span>stdout / stderr / cmd</span><div class="vfo-web-app__actions"><button type="button">Copy</button><button type="button">Expand</button></div></div>',
      '        <pre class="vfo-web-app__code"></pre>',
      "      </div>",
      "    </aside>",
      "  </section>",
      '  <footer class="vfo-web-app__legend">',
      '    <div class="vfo-web-app__legend-summary"></div>',
      '    <div class="vfo-web-app__stage-badges"></div>',
      "  </footer>",
      "</div>"
    ].join("\n");

    container.dataset.activeAsset = selectedPipeline.selectedAsset;
    container.dataset.activeNode = selectedPipeline.selectedNode;
    container.dataset.selectedPipelineId = selectedPipeline.id;

    var pipelineSelect = container.querySelector("[data-vfo-pipeline-select]");
    if (pipelineSelect) {
      pipelineSelect.value = selectedPipeline.id;
    }

    renderAssets(container, selectedPipeline, state);
    renderAssetSequence(container, selectedPipeline);
    renderWorkflow(container, selectedPipeline, state);
    renderInspector(container, selectedPipeline);
    renderLegend(container, selectedPipeline);
  }

  function renderAssets(container, pipeline, state) {
    var assetList = container.querySelector(".vfo-web-app__asset-list");
    var filterList = container.querySelector(".vfo-web-app__filter-list");
    var summary = container.querySelector("[data-vfo-asset-summary]");
    var searchInput = container.querySelector("[data-vfo-asset-search]");
    var query = String((state && state.assetQuery) || "").toLowerCase();
    var filters = state && state.assetFilters ? state.assetFilters : defaultAssetFilters();
    var visibleAssets = pipeline.assets.filter(function (asset) {
      return assetMatchesSearch(asset, query) && assetMatchesFilters(asset, filters);
    });
    var activeAsset = pipeline.assets.find(function (asset) {
      return asset.name === pipeline.selectedAsset;
    }) || pipeline.assets[0];

    if (summary) {
      summary.textContent = assetFilterSummary(pipeline, state || {}, visibleAssets.length);
    }

    if (searchInput && searchInput.value !== (state && state.assetQuery ? state.assetQuery : "")) {
      searchInput.value = (state && state.assetQuery) || "";
    }

    assetList.innerHTML = visibleAssets.length ? visibleAssets.map(function (asset) {
      var assetStatus = String(asset.status || "available");
      return [
        '<button type="button" class="vfo-web-app__asset ' + (asset.name === activeAsset.name ? "is-active" : "") + '" data-asset="' + escapeHtml(asset.name) + '">',
        '  <span class="vfo-web-app__asset-dot ' + assetToneClass(assetStatus) + '"></span>',
        '  <span class="vfo-web-app__asset-name">' + escapeHtml(asset.name) + '</span>',
        '  <span class="vfo-web-app__asset-status">' + escapeHtml(assetStatus) + '</span>',
        "</button>"
      ].join("\n");
    }).join("") : [
      '<div class="vfo-web-app__asset-empty">',
      '  <strong>No assets match the current search or filters.</strong>',
      '  <span>Try clearing the text box or re-enabling a filter.</span>',
      "</div>"
    ].join("\n");

    filterList.innerHTML = pipeline.filters.map(function (filter) {
      var filterKey = String(filter || "").toLowerCase();
      var checked = filterKey === "all"
        ? Boolean(filters.failed && filters.running && filters.waiting && filters.complete)
        : Boolean(filters[filterKey]);
      return [
        '<label class="vfo-web-app__filter">',
        '  <input type="checkbox" data-vfo-asset-filter="' + escapeHtml(filterKey) + '" ' + (checked ? "checked" : "") + ' />',
        '  <span>' + escapeHtml(filter) + "</span>",
        "</label>"
      ].join("\n");
    }).join("");
  }

  function renderAssetSequence(container, pipeline) {
    var sequence = container.querySelector("[data-vfo-asset-sequence]");
    var activeAsset = pipeline.assets.find(function (asset) {
      return asset.name === pipeline.selectedAsset;
    }) || pipeline.assets[0];

    if (!sequence) {
      return;
    }

    sequence.innerHTML = pipeline.assets.map(function (asset, index) {
      var assetStatus = String(asset.status || "available");
      return [
        '<button type="button" class="vfo-web-app__asset-chip ' + (asset.name === activeAsset.name ? "is-active" : "") + '" data-vfo-asset-sequence-item data-asset="' + escapeHtml(asset.name) + '">',
        '  <span class="vfo-web-app__asset-chip-order">#' + String(index + 1).padStart(2, "0") + "</span>",
        '  <span class="vfo-web-app__asset-chip-name">' + escapeHtml(asset.name) + "</span>",
        '  <span class="vfo-web-app__asset-chip-status"><span class="vfo-web-app__asset-dot ' + assetToneClass(assetStatus) + '"></span>' + escapeHtml(assetStatus) + "</span>",
        "</button>"
      ].join("\n");
    }).join("");
  }

  function syncWorkflowCanvas(container, pipeline, state) {
    var workflowShell = container.querySelector("[data-vfo-canvas-shell]");
    var workflowStage = container.querySelector(".vfo-web-app__workflow-stage");
    var canvasScale = container.querySelector("[data-vfo-canvas-scale]");
    var laidOutNodes = applyNodeLayout(pipeline.workflow.nodes);
    var rawBounds = { width: 0, height: 0 };
    var stageWidth = 1200;
    var stageHeight = 520;
    var availableWidth = workflowShell ? Math.max(workflowShell.clientWidth - 32, 1) : 0;
    var availableHeight = workflowShell ? Math.max(workflowShell.clientHeight - 32, 1) : 0;
    var canvas = state.canvas || defaultCanvasState();
    var fitScale = 1;
    var appliedScale = 1;

    laidOutNodes.forEach(function (node) {
      rawBounds.width = Math.max(rawBounds.width, node.x + 220);
      rawBounds.height = Math.max(rawBounds.height, node.y + 128);
    });

    stageWidth = Math.max(Math.round(rawBounds.width + 64), stageWidth);
    stageHeight = Math.max(Math.round(rawBounds.height + 64), stageHeight);

    if (availableWidth && availableHeight) {
      fitScale = Math.min(
        1,
        availableWidth / Math.max(stageWidth, 1),
        availableHeight / Math.max(stageHeight, 1)
      );
    }

    appliedScale = clamp(fitScale * canvas.zoom, 0.25, 2.5);

    if (workflowStage) {
      workflowStage.style.width = Math.round(stageWidth) + "px";
      workflowStage.style.height = Math.round(stageHeight) + "px";
      workflowStage.style.transform = "translate(" + Math.round(canvas.panX) + "px, " + Math.round(canvas.panY) + "px) scale(" + appliedScale + ")";
      workflowStage.style.transformOrigin = "top left";
    }

    if (canvasScale) {
      canvasScale.textContent = Math.round(appliedScale * 100) + "%";
    }

    return {
      fitScale: fitScale,
      appliedScale: appliedScale,
      bounds: rawBounds
    };
  }

  function renderWorkflow(container, pipeline, state) {
    var workflowNodes = container.querySelector(".vfo-web-app__workflow-nodes");
    var workflowEdges = container.querySelector(".vfo-web-app__workflow-edges");
    var canvasMetrics = syncWorkflowCanvas(container, pipeline, state || { canvas: defaultCanvasState() });
    var laidOutNodes = applyNodeLayout(pipeline.workflow.nodes);
    var rawBounds = canvasMetrics.bounds;

    workflowNodes.innerHTML = laidOutNodes.map(function (node) {
      var width = 220;
      var height = 128;
      var x = node.x;
      var y = node.y;
      return [
        '<button type="button" class="vfo-web-app__node ' + (node.id === pipeline.selectedNode ? "is-active " : "") + 'vfo-web-app__status-' + String(node.status || "waiting").toLowerCase() + '"',
        '        data-node="' + escapeHtml(node.id) + '"',
        '        style="left:' + x + "px; top:" + y + 'px; width:' + width + "px; min-height:" + height + 'px;">',
        '  <span class="vfo-web-app__node-head">',
        '    <strong>' + escapeHtml(node.label) + "</strong>",
        '    <span>' + statusGlyph(node.status) + "</span>",
        "  </span>",
        '  <span class="vfo-web-app__node-body">' + escapeHtml(node.subtitle) + "</span>",
        "</button>"
      ].join("\n");
    }).join("");

    var stageWidth = Math.max(Math.round(rawBounds.width + 64), 1200);
    var stageHeight = Math.max(Math.round(rawBounds.height + 64), 520);

    workflowNodes.style.width = stageWidth + "px";
    workflowNodes.style.height = stageHeight + "px";
    workflowEdges.setAttribute("viewBox", "0 0 " + stageWidth + " " + stageHeight);
    workflowEdges.innerHTML = pipeline.workflow.edges.map(function (edge) {
      var source = laidOutNodes.find(function (node) {
        return node.id === edge.source;
      });
      var target = laidOutNodes.find(function (node) {
        return node.id === edge.target;
      });
      if (!source || !target) {
        return "";
      }
      var x1 = source.x + 210;
      var y1 = source.y + 58;
      var x2 = target.x;
      var y2 = target.y + 58;
      var midX = Math.round((x1 + x2) / 2);
      return '<path d="M ' + x1 + " " + y1 + " C " + midX + " " + y1 + ", " + midX + " " + y2 + ", " + x2 + " " + y2 + '" marker-end="url(#vfo-arrow)" />';
    }).join("");
  }

  function applyNodeLayout(nodes) {
    var layoutById = {
      input: { x: 48, y: 146 },
      probe: { x: 280, y: 146 },
      deint: { x: 510, y: 146 },
      encode: { x: 742, y: 146 },
      hls: { x: 994, y: 236 },
      qc: { x: 770, y: 302 },
      metadata: { x: 512, y: 302 },
      validate: { x: 770, y: 146 },
      report: { x: 1000, y: 146 }
    };

    return nodes.map(function (node, index) {
      var next = JSON.parse(JSON.stringify(node));
      var fallbackX = 48 + (index * 230);
      var fallbackY = 146;
      var layout = layoutById[next.id] || {};
      next.x = typeof next.x === "number" ? next.x : (typeof layout.x === "number" ? layout.x : fallbackX);
      next.y = typeof next.y === "number" ? next.y : (typeof layout.y === "number" ? layout.y : fallbackY);
      return next;
    });
  }

  function renderInspector(container, pipeline) {
    var activeAsset = pipeline.assets.find(function (asset) {
      return asset.name === pipeline.selectedAsset;
    }) || pipeline.assets[0];
    var activeNode = pipeline.workflow.details[pipeline.selectedNode] || pipeline.workflow.details.encode;
    var densityMap = container.querySelector(".vfo-web-app__density-map");
    var inspectorCard = container.querySelector(".vfo-web-app__inspector-card");
    var code = container.querySelector(".vfo-web-app__code");

    densityMap.innerHTML = pipeline.summaryCounts.map(function (item) {
      return [
        '<div class="vfo-web-app__density-item">',
        '  <span class="vfo-web-app__density-icon">' + escapeHtml(item.icon) + "</span>",
        '  <strong>' + escapeHtml(String(item.count)) + "</strong>",
        '  <span>' + escapeHtml(item.label) + "</span>",
        "</div>"
      ].join("\n");
    }).join("");

    inspectorCard.dataset.asset = activeAsset.name;
    inspectorCard.innerHTML = [
      '<div class="vfo-web-app__row"><span>Asset</span><strong>' + escapeHtml(activeAsset.name) + "</strong></div>",
      '<div class="vfo-web-app__row"><span>Node</span><strong>' + escapeHtml(activeNode.node) + "</strong></div>",
      '<div class="vfo-web-app__row"><span>Status</span><strong>' + escapeHtml(activeNode.status) + "</strong></div>",
      '<div class="vfo-web-app__row"><span>Exit code</span><strong>' + escapeHtml(activeNode.exitCode === null ? "pending" : String(activeNode.exitCode)) + "</strong></div>"
    ].join("\n");

    code.textContent = [activeNode.command, "", activeNode.output.join("\n")].join("\n");
  }

  function renderLegend(container, pipeline) {
    var legendSummary = container.querySelector(".vfo-web-app__legend-summary");
    var stageBadges = container.querySelector(".vfo-web-app__stage-badges");

    legendSummary.innerHTML = pipeline.summaryCounts.map(function (item) {
      return '<span>' + escapeHtml(item.icon + " " + item.label + ": " + item.count) + "</span>";
    }).join("");

    stageBadges.innerHTML = pipeline.stageTotals.map(function (stage) {
      return '<span class="vfo-web-app__stage-badge">' + escapeHtml(stage.label + " " + stage.count) + "</span>";
    }).join("");
  }

  function bindInteractions(root) {
    if (root.__vfoBound) {
      return;
    }
    root.__vfoBound = true;

    root.addEventListener("change", function (event) {
      var state = root.__vfoState;
      if (!state) {
        return;
      }

      var pipelineSelect = event.target.closest("[data-vfo-pipeline-select]");
      var assetFilter = event.target.closest("[data-vfo-asset-filter]");
      var pipeline = getSelectedPipeline(state);

      if (pipelineSelect) {
        stopReplay(root);
        state.selectedPipelineId = pipelineSelect.value;
        render(root, state);
        startReplay(root, state);
        return;
      }

      if (assetFilter) {
        var filterKey = assetFilter.getAttribute("data-vfo-asset-filter");
        var filterChecked = assetFilter.checked;
        if (filterKey === "all") {
          state.assetFilters = defaultAssetFilters();
          state.assetFilters.failed = filterChecked;
          state.assetFilters.running = filterChecked;
          state.assetFilters.waiting = filterChecked;
          state.assetFilters.complete = filterChecked;
        } else {
          state.assetFilters = state.assetFilters || defaultAssetFilters();
          state.assetFilters[filterKey] = filterChecked;
        }
        renderAssets(root, pipeline, state);
        return;
      }
    });

    root.addEventListener("input", function (event) {
      var state = root.__vfoState;
      if (!state) {
        return;
      }

      var assetSearch = event.target.closest("[data-vfo-asset-search]");
      if (!assetSearch) {
        return;
      }

      state.assetQuery = assetSearch.value || "";
      renderAssets(root, getSelectedPipeline(state), state);
    });

    root.addEventListener("click", function (event) {
      var state = root.__vfoState;
      if (!state) {
        return;
      }

      var pipeline = getSelectedPipeline(state);
      var panelToggle = event.target.closest("[data-vfo-panel-toggle]");
      var assetButton = event.target.closest("[data-asset]");
      var nodeButton = event.target.closest("[data-node]");
      var canvasAction = event.target.closest("[data-vfo-canvas-action]");
      var assetSequenceItem = event.target.closest("[data-vfo-asset-sequence-item]");

      if (panelToggle) {
        state.panelState = state.panelState || { assets: true, inspector: true };
        var panelName = panelToggle.getAttribute("data-vfo-panel-toggle");
        state.panelState[panelName] = !state.panelState[panelName];
        render(root, state);
        return;
      }

      if (canvasAction) {
        state.canvas = state.canvas || defaultCanvasState();
        switch (canvasAction.getAttribute("data-vfo-canvas-action")) {
          case "fit":
          case "reset":
            state.canvas.zoom = 1;
            state.canvas.panX = 0;
            state.canvas.panY = 0;
            break;
          case "zoom-in":
            state.canvas.zoom = clamp((state.canvas.zoom || 1) * 1.15, 0.25, 2.5);
            break;
          case "zoom-out":
            state.canvas.zoom = clamp((state.canvas.zoom || 1) / 1.15, 0.25, 2.5);
            break;
        }
        renderWorkflow(root, pipeline, state);
        return;
      }

      if (assetSequenceItem) {
        stopReplay(root);
        pipeline.selectedAsset = assetSequenceItem.getAttribute("data-asset");
        render(root, state);
        return;
      }

      if (assetButton) {
        stopReplay(root);
        pipeline.selectedAsset = assetButton.getAttribute("data-asset");
        render(root, state);
        return;
      }

      if (nodeButton) {
        stopReplay(root);
        pipeline.selectedNode = nodeButton.getAttribute("data-node");
        render(root, state);
      }
    });

    root.addEventListener("pointerdown", function (event) {
      var state = root.__vfoState;
      var canvasShell;
      var interactiveTarget;

      if (!state) {
        return;
      }

      canvasShell = event.target.closest("[data-vfo-canvas-shell]");
      interactiveTarget = event.target.closest("button, input, select, textarea, a, [data-node]");
      if (!canvasShell || interactiveTarget) {
        return;
      }

      state.canvas = state.canvas || defaultCanvasState();
      root.__vfoPanState = {
        pointerId: event.pointerId,
        startX: event.clientX,
        startY: event.clientY,
        panX: state.canvas.panX,
        panY: state.canvas.panY
      };

      try {
        canvasShell.setPointerCapture(event.pointerId);
      } catch (error) {
        // Ignore capture failures on browsers that do not support it for this target.
      }
      canvasShell.classList.add("is-panning");
    });

    root.addEventListener("pointermove", function (event) {
      var state = root.__vfoState;
      var panState = root.__vfoPanState;
      var pipeline;

      if (!state || !panState || panState.pointerId !== event.pointerId) {
        return;
      }

      pipeline = getSelectedPipeline(state);
      state.canvas = state.canvas || defaultCanvasState();
      state.canvas.panX = panState.panX + (event.clientX - panState.startX);
      state.canvas.panY = panState.panY + (event.clientY - panState.startY);
      syncWorkflowCanvas(root, pipeline, state);
    });

    root.addEventListener("pointerup", function (event) {
      var panState = root.__vfoPanState;
      var canvasShell = event.target.closest("[data-vfo-canvas-shell]");

      if (!panState || panState.pointerId !== event.pointerId) {
        return;
      }

      if (canvasShell) {
        try {
          canvasShell.releasePointerCapture(event.pointerId);
        } catch (error) {
          // Ignore capture release races during drag end.
        }
        canvasShell.classList.remove("is-panning");
      }
      root.__vfoPanState = null;
    });

    root.addEventListener("pointercancel", function (event) {
      var panState = root.__vfoPanState;
      var canvasShell = event.target.closest("[data-vfo-canvas-shell]");

      if (!panState || panState.pointerId !== event.pointerId) {
        return;
      }

      if (canvasShell) {
        try {
          canvasShell.releasePointerCapture(event.pointerId);
        } catch (error) {
          // Ignore capture release races during drag end.
        }
        canvasShell.classList.remove("is-panning");
      }
      root.__vfoPanState = null;
    });
  }

  function stopReplay(root) {
    var playback = root.__vfoPlayback;
    if (playback && playback.timer) {
      clearTimeout(playback.timer);
    }
    root.__vfoPlayback = null;
  }

  function startReplay(root, state) {
    var selectedPipeline = getSelectedPipeline(state);
    var frames = Array.isArray(selectedPipeline.events) ? selectedPipeline.events.map(cloneFrame) : [];
    var playback;

    if (!frames.length) {
      state.playback = null;
      render(root, state);
      return;
    }

    stopReplay(root);
    playback = {
      frames: frames,
      index: 0,
      timer: null
    };
    root.__vfoPlayback = playback;

    var step = function () {
      var frame = playback.frames[playback.index];
      if (!frame || !root.__vfoPlayback) {
        state.playback = {
          index: playback.frames.length,
          total: playback.frames.length,
          label: "complete"
        };
        render(root, state);
        stopReplay(root);
        return;
      }

      applyFrame(selectedPipeline, frame);
      state.selectedPipelineId = selectedPipeline.id;
      state.playback = {
        index: playback.index + 1,
        total: playback.frames.length,
        label: frame.label || selectedPipeline.label || "emit stream",
        assetIndex: frame.assetIndex,
        assetTotal: frame.assetTotal,
        assetLabel: frame.assetLabel
      };
      render(root, state);
      playback.index += 1;

      if (playback.index < playback.frames.length) {
        playback.timer = setTimeout(step, frame.delayMs || 700);
      } else {
        playback.timer = setTimeout(function () {
          if (root.__vfoPlayback === playback) {
            state.playback = {
              index: playback.frames.length,
              total: playback.frames.length,
              label: "complete",
              assetIndex: playback.frames[playback.frames.length - 1] && playback.frames[playback.frames.length - 1].assetIndex,
              assetTotal: playback.frames[playback.frames.length - 1] && playback.frames[playback.frames.length - 1].assetTotal,
              assetLabel: playback.frames[playback.frames.length - 1] && playback.frames[playback.frames.length - 1].assetLabel
            };
            render(root, state);
            stopReplay(root);
          }
        }, frame.delayMs || 700);
      }
    };

    step();
  }

  function initRoot(root) {
    var source = root.getAttribute("data-vfo-web-app-src");
    var dataPromise = source ? fetchJson(source).catch(function () { return fallbackPipeline; }) : Promise.resolve(fallbackPipeline);

    bindInteractions(root);

    dataPromise.then(function (data) {
      root.__vfoState = normalizeDashboard(data);
      render(root, root.__vfoState);
      startReplay(root, root.__vfoState);
    });
  }

  Array.prototype.forEach.call(roots, initRoot);
})();
