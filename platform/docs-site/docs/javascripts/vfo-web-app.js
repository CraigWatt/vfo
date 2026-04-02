(function () {
  "use strict";

  var roots = document.querySelectorAll("[data-vfo-web-app]");
  if (!roots.length) {
    return;
  }

  var fallbackPipeline = {
    id: "demo",
    label: "Demo payload",
    title: "Pipeline: UHD SDR Ladder",
    runLabel: "Run: 2026-04-02 18:42",
    sourceLabel: "Demo payload",
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

    if (Array.isArray(dashboard.pipelines) && dashboard.pipelines.length) {
      pipelines = dashboard.pipelines.map(function (pipeline, index) {
        return clonePipeline(pipeline, index);
      });
    } else {
      pipelines = [clonePipeline(dashboard, 0)];
    }

    return {
      title: dashboard.title || pipelines[0].title || "VFO Web App Demo",
      selectedPipelineId: dashboard.selectedPipelineId || pipelines[0].id,
      pipelines: pipelines,
      sourceLabel: dashboard.sourceLabel || pipelines[0].sourceLabel || "Demo payload",
      sourceWorkflow: dashboard.sourceWorkflow || pipelines[0].sourceWorkflow || "",
      sourceRunUrl: dashboard.sourceRunUrl || pipelines[0].sourceRunUrl || "",
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

  function buildPipelineOptions(state) {
    if (state.pipelines.length <= 1) {
      return "";
    }

    return [
      '<label class="vfo-web-app__pipeline-select">',
      '  <span>Pipeline</span>',
      '  <select data-vfo-pipeline-select>',
      state.pipelines.map(function (pipeline) {
        return '<option value="' + escapeHtml(pipeline.id) + '">' + escapeHtml(pipeline.label) + '</option>';
      }).join(""),
      "  </select>",
      "</label>"
    ].join("\n");
  }

  function buildReplayMeter(state) {
    var playback = state.playback;
    if (!playback || !playback.total) {
      return "";
    }

    return [
      '<div class="vfo-web-app__replay-meter">',
      '  <span>Replay</span>',
      '  <strong>' + escapeHtml(String(playback.index).padStart(2, "0") + " / " + String(playback.total).padStart(2, "0")) + "</strong>",
      "  <em>" + escapeHtml(playback.label || "emit stream") + "</em>",
      "</div>"
    ].join("\n");
  }

  function render(container, state) {
    var selectedPipeline = getSelectedPipeline(state);
    var pipelineOptions = buildPipelineOptions(state);
    var replayMeter = buildReplayMeter(state);
    var liveAttr = selectedPipeline.sourceRunUrl ? ' data-live="1"' : "";
    var sourceWorkflowHtml = selectedPipeline.sourceWorkflow
      ? '<strong>' + escapeHtml(selectedPipeline.sourceWorkflow) + '</strong>'
      : "";
    var sourceRunHtml = selectedPipeline.sourceRunUrl
      ? '<a href="' + escapeHtml(selectedPipeline.sourceRunUrl) + '" target="_blank" rel="noreferrer">source run</a>'
      : "";

    container.innerHTML = [
      '<div class="vfo-web-app__shell">',
      '  <header class="vfo-web-app__topbar">',
      '    <div>',
      '      <h2>' + escapeHtml(selectedPipeline.title) + "</h2>",
      '      <p>' + escapeHtml(selectedPipeline.runLabel) + "</p>",
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
      '  <section class="vfo-web-app__workspace">',
      '    <aside class="vfo-web-app__panel vfo-web-app__assets">',
      '      <div class="vfo-web-app__panel-head"><h3>Assets</h3><span>Mezzanine folder</span></div>',
      '      <label class="vfo-web-app__search">',
      '        <span>Search assets...</span>',
      '        <input type="text" value="" aria-label="Search assets" />',
      "      </label>",
      '      <div class="vfo-web-app__asset-list"></div>',
      '      <div class="vfo-web-app__filter-block">',
      "        <h4>Filters</h4>",
      '        <div class="vfo-web-app__filter-list"></div>',
      "      </div>",
      "    </aside>",
      '    <section class="vfo-web-app__panel vfo-web-app__workflow">',
      '      <div class="vfo-web-app__panel-head"><h3>Workflow</h3><span>All nodes visible by default</span></div>',
      '      <div class="vfo-web-app__workflow-shell">',
      '        <svg class="vfo-web-app__workflow-edges" aria-hidden="true"></svg>',
      '        <div class="vfo-web-app__workflow-nodes"></div>',
      "      </div>",
      '      <div class="vfo-web-app__caption">Node status for the selected asset is shown inline, with failures, running, waiting, and complete states carried directly on the lane.</div>',
      "    </section>",
      '    <aside class="vfo-web-app__panel vfo-web-app__inspector">',
      '      <div class="vfo-web-app__panel-head"><h3>Inspector</h3><span>Current selection</span></div>',
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

    renderAssets(container, selectedPipeline);
    renderWorkflow(container, selectedPipeline);
    renderInspector(container, selectedPipeline);
    renderLegend(container, selectedPipeline);
  }

  function renderAssets(container, pipeline) {
    var assetList = container.querySelector(".vfo-web-app__asset-list");
    var filterList = container.querySelector(".vfo-web-app__filter-list");
    var activeAsset = pipeline.assets.find(function (asset) {
      return asset.name === pipeline.selectedAsset;
    }) || pipeline.assets[0];

    assetList.innerHTML = pipeline.assets.map(function (asset) {
      return [
        '<button type="button" class="vfo-web-app__asset ' + (asset.name === activeAsset.name ? "is-active" : "") + '" data-asset="' + escapeHtml(asset.name) + '">',
        '  <span class="vfo-web-app__asset-dot vfo-web-app__status-' + String(asset.status || "waiting").toLowerCase() + '"></span>',
        '  <span class="vfo-web-app__asset-name">' + escapeHtml(asset.name) + '</span>',
        '  <span class="vfo-web-app__asset-icon">' + escapeHtml(asset.icon) + '</span>',
        "</button>"
      ].join("\n");
    }).join("");

    filterList.innerHTML = pipeline.filters.map(function (filter) {
      return [
        '<label class="vfo-web-app__filter">',
        '  <input type="checkbox" checked />',
        '  <span>' + escapeHtml(filter) + "</span>",
        "</label>"
      ].join("\n");
    }).join("");
  }

  function renderWorkflow(container, pipeline) {
    var workflowNodes = container.querySelector(".vfo-web-app__workflow-nodes");
    var workflowEdges = container.querySelector(".vfo-web-app__workflow-edges");
    var bounds = { width: 0, height: 0 };

    workflowNodes.innerHTML = pipeline.workflow.nodes.map(function (node) {
      bounds.width = Math.max(bounds.width, node.x + 220);
      bounds.height = Math.max(bounds.height, node.y + 128);
      return [
        '<button type="button" class="vfo-web-app__node ' + (node.id === pipeline.selectedNode ? "is-active " : "") + 'vfo-web-app__status-' + String(node.status || "waiting").toLowerCase() + '"',
        '        data-node="' + escapeHtml(node.id) + '"',
        '        style="left:' + node.x + "px; top:" + node.y + 'px;">',
        '  <span class="vfo-web-app__node-head">',
        '    <strong>' + escapeHtml(node.label) + "</strong>",
        '    <span>' + statusGlyph(node.status) + "</span>",
        "  </span>",
        '  <span class="vfo-web-app__node-body">' + escapeHtml(node.subtitle) + "</span>",
        "</button>"
      ].join("\n");
    }).join("");

    workflowEdges.setAttribute("viewBox", "0 0 " + Math.max(bounds.width, 1200) + " " + Math.max(bounds.height, 460));
    workflowEdges.innerHTML = pipeline.workflow.edges.map(function (edge) {
      var source = pipeline.workflow.nodes.find(function (node) {
        return node.id === edge.source;
      });
      var target = pipeline.workflow.nodes.find(function (node) {
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
      return '<path d="M ' + x1 + " " + y1 + " C " + midX + " " + y1 + ", " + midX + " " + y2 + ", " + x2 + " " + y2 + '" />';
    }).join("");
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
      if (pipelineSelect) {
        stopReplay(root);
        state.selectedPipelineId = pipelineSelect.value;
        render(root, state);
        startReplay(root, state);
      }
    });

    root.addEventListener("click", function (event) {
      var state = root.__vfoState;
      if (!state) {
        return;
      }

      var pipeline = getSelectedPipeline(state);
      var assetButton = event.target.closest("[data-asset]");
      var nodeButton = event.target.closest("[data-node]");

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
        label: frame.label || selectedPipeline.label || "emit stream"
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
              label: "complete"
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
