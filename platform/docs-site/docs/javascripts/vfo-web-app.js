(function () {
  "use strict";

  var roots = document.querySelectorAll("[data-vfo-web-app]");
  if (!roots.length) {
    return;
  }

  var fallbackData = {
    title: "Pipeline: UHD SDR Ladder",
    runLabel: "Run: 2026-04-02 18:42",
    sourceLabel: "Demo payload",
    sourceWorkflow: "",
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

  function getData(container) {
    var source = container.getAttribute("data-vfo-web-app-src");
    if (!source) {
      return Promise.resolve(fallbackData);
    }

    return fetchJson(source).catch(function () {
      return fallbackData;
    });
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
    return "○";
  }

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  function render(container, data) {
    var activeAsset = data.assets.find(function (asset) {
      return asset.name === data.selectedAsset;
    }) || data.assets[0];
    var activeNode = data.workflow.details[data.selectedNode] || data.workflow.details.encode;

    container.innerHTML = [
      '<div class="vfo-web-app__shell">',
      '  <header class="vfo-web-app__topbar">',
      '    <div>',
      '      <h2>' + escapeHtml(data.title) + '</h2>',
      '      <p>' + escapeHtml(data.runLabel) + '</p>',
      '    </div>',
      '    <div class="vfo-web-app__source">',
      '      <span>' + escapeHtml(data.sourceLabel) + '</span>',
      data.sourceWorkflow ? '      <strong>' + escapeHtml(data.sourceWorkflow) + '</strong>' : '',
      data.sourceRunUrl ? '      <a href="' + escapeHtml(data.sourceRunUrl) + '" target="_blank" rel="noreferrer">source run</a>' : '',
      '    </div>',
      '  </header>',
      '  <section class="vfo-web-app__workspace">',
      '    <aside class="vfo-web-app__panel vfo-web-app__assets">',
      '      <div class="vfo-web-app__panel-head"><h3>Assets</h3><span>Mezzanine folder</span></div>',
      '      <label class="vfo-web-app__search">',
      '        <span>Search assets...</span>',
      '        <input type="text" value="" aria-label="Search assets" />',
      '      </label>',
      '      <div class="vfo-web-app__asset-list"></div>',
      '      <div class="vfo-web-app__filter-block">',
      '        <h4>Filters</h4>',
      '        <div class="vfo-web-app__filter-list"></div>',
      '      </div>',
      '    </aside>',
      '    <section class="vfo-web-app__panel vfo-web-app__workflow">',
      '      <div class="vfo-web-app__panel-head"><h3>Workflow</h3><span>All nodes visible by default</span></div>',
      '      <div class="vfo-web-app__workflow-shell">',
      '        <svg class="vfo-web-app__workflow-edges" aria-hidden="true"></svg>',
      '        <div class="vfo-web-app__workflow-nodes"></div>',
      '      </div>',
      '      <div class="vfo-web-app__caption">Node status for the selected asset is shown inline, with failures, running, waiting, and complete states carried directly on the lane.</div>',
      '    </section>',
      '    <aside class="vfo-web-app__panel vfo-web-app__inspector">',
      '      <div class="vfo-web-app__panel-head"><h3>Inspector</h3><span>Current selection</span></div>',
      '      <div class="vfo-web-app__density-map"></div>',
      '      <div class="vfo-web-app__inspector-card"></div>',
      '      <div class="vfo-web-app__code-panel">',
      '        <div class="vfo-web-app__code-head"><span>stdout / stderr / cmd</span><div class="vfo-web-app__actions"><button type="button">Copy</button><button type="button">Expand</button></div></div>',
      '        <pre class="vfo-web-app__code"></pre>',
      '      </div>',
      '    </aside>',
      '  </section>',
      '  <footer class="vfo-web-app__legend">',
      '    <div class="vfo-web-app__legend-summary"></div>',
      '    <div class="vfo-web-app__stage-badges"></div>',
      '  </footer>',
      '</div>'
    ].join("\n");

    container.dataset.activeAsset = data.selectedAsset;
    container.dataset.activeNode = data.selectedNode;

    renderAssets(container, data, activeAsset, activeNode);
    renderWorkflow(container, data, activeNode);
    renderInspector(container, data, activeAsset, activeNode);
    renderLegend(container, data);
    bindInteractions(container, data);
  }

  function renderAssets(container, data, activeAsset, activeNode) {
    var assetList = container.querySelector(".vfo-web-app__asset-list");
    var filterList = container.querySelector(".vfo-web-app__filter-list");

    assetList.innerHTML = data.assets.map(function (asset) {
      return [
        '<button type="button" class="vfo-web-app__asset ' + (asset.name === activeAsset.name ? "is-active" : "") + '" data-asset="' + escapeHtml(asset.name) + '">',
        '  <span class="vfo-web-app__asset-dot vfo-web-app__status-' + String(asset.status || "waiting").toLowerCase() + '"></span>',
        '  <span class="vfo-web-app__asset-name">' + escapeHtml(asset.name) + '</span>',
        '  <span class="vfo-web-app__asset-icon">' + escapeHtml(asset.icon) + '</span>',
        '</button>'
      ].join("\n");
    }).join("");

    filterList.innerHTML = data.filters.map(function (filter) {
      return [
        '<label class="vfo-web-app__filter">',
        '  <input type="checkbox" checked />',
        '  <span>' + escapeHtml(filter) + '</span>',
        '</label>'
      ].join("\n");
    }).join("");

    container.dataset.activeAsset = activeAsset.name;
    container.dataset.activeNode = activeNode.node.toLowerCase();
  }

  function renderWorkflow(container, data, activeNode) {
    var workflowNodes = container.querySelector(".vfo-web-app__workflow-nodes");
    var workflowEdges = container.querySelector(".vfo-web-app__workflow-edges");
    var bounds = { width: 0, height: 0 };

    workflowNodes.innerHTML = data.workflow.nodes.map(function (node) {
      bounds.width = Math.max(bounds.width, node.x + 220);
      bounds.height = Math.max(bounds.height, node.y + 128);
      return [
        '<button type="button" class="vfo-web-app__node ' + (node.id === data.selectedNode ? "is-active " : "") + 'vfo-web-app__status-' + String(node.status || "waiting").toLowerCase() + '"',
        '        data-node="' + escapeHtml(node.id) + '"',
        '        style="left:' + node.x + 'px; top:' + node.y + 'px;">',
        '  <span class="vfo-web-app__node-head">',
        '    <strong>' + escapeHtml(node.label) + '</strong>',
        '    <span>' + statusGlyph(node.status) + '</span>',
        '  </span>',
        '  <span class="vfo-web-app__node-body">' + escapeHtml(node.subtitle) + '</span>',
        '</button>'
      ].join("\n");
    }).join("");

    workflowEdges.setAttribute("viewBox", "0 0 " + Math.max(bounds.width, 1200) + " " + Math.max(bounds.height, 460));
    workflowEdges.innerHTML = data.workflow.edges.map(function (edge) {
      var source = data.workflow.nodes.find(function (node) { return node.id === edge.source; });
      var target = data.workflow.nodes.find(function (node) { return node.id === edge.target; });
      if (!source || !target) {
        return "";
      }
      var x1 = source.x + 210;
      var y1 = source.y + 58;
      var x2 = target.x;
      var y2 = target.y + 58;
      var midX = Math.round((x1 + x2) / 2);
      return '<path d="M ' + x1 + ' ' + y1 + ' C ' + midX + ' ' + y1 + ', ' + midX + ' ' + y2 + ', ' + x2 + ' ' + y2 + '" />';
    }).join("");

    updateInspector(container, data, activeNode);
  }

  function renderInspector(container, data, activeAsset, activeNode) {
    var densityMap = container.querySelector(".vfo-web-app__density-map");
    var inspectorCard = container.querySelector(".vfo-web-app__inspector-card");
    var code = container.querySelector(".vfo-web-app__code");

    densityMap.innerHTML = data.summaryCounts.map(function (item) {
      return [
        '<div class="vfo-web-app__density-item">',
        '  <span class="vfo-web-app__density-icon">' + escapeHtml(item.icon) + '</span>',
        '  <strong>' + escapeHtml(String(item.count)) + '</strong>',
        '  <span>' + escapeHtml(item.label) + '</span>',
        '</div>'
      ].join("\n");
    }).join("");

    inspectorCard.dataset.asset = activeAsset.name;
    inspectorCard.innerHTML = [
      '<div class="vfo-web-app__row"><span>Asset</span><strong>' + escapeHtml(activeAsset.name) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Node</span><strong>' + escapeHtml(activeNode.node) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Status</span><strong>' + escapeHtml(activeNode.status) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Exit code</span><strong>' + escapeHtml(activeNode.exitCode === null ? "pending" : String(activeNode.exitCode)) + '</strong></div>'
    ].join("\n");

    code.textContent = [activeNode.command, "", activeNode.output.join("\n")].join("\n");
  }

  function updateInspector(container, data, activeNode) {
    var code = container.querySelector(".vfo-web-app__code");
    var inspectorCard = container.querySelector(".vfo-web-app__inspector-card");
    var assetName = container.dataset.activeAsset || data.selectedAsset;
    var selectedAsset = data.assets.find(function (asset) { return asset.name === assetName; }) || data.assets[0];

    if (!activeNode) {
      activeNode = data.workflow.details[data.selectedNode] || data.workflow.details.encode;
    }

    inspectorCard.innerHTML = [
      '<div class="vfo-web-app__row"><span>Asset</span><strong>' + escapeHtml(selectedAsset.name) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Node</span><strong>' + escapeHtml(activeNode.node) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Status</span><strong>' + escapeHtml(activeNode.status) + '</strong></div>',
      '<div class="vfo-web-app__row"><span>Exit code</span><strong>' + escapeHtml(activeNode.exitCode === null ? "pending" : String(activeNode.exitCode)) + '</strong></div>'
    ].join("\n");

    code.textContent = [activeNode.command, "", activeNode.output.join("\n")].join("\n");
  }

  function renderLegend(container, data) {
    var legendSummary = container.querySelector(".vfo-web-app__legend-summary");
    var stageBadges = container.querySelector(".vfo-web-app__stage-badges");

    legendSummary.innerHTML = [
      '<span>⏺ Total: 128</span>',
      '<span>✔ Complete: 91</span>',
      '<span>✖ Failed: 7</span>',
      '<span>⏳ Running: 12</span>',
      '<span>○ Waiting: 18</span>'
    ].join("");

    stageBadges.innerHTML = data.stageTotals.map(function (stage) {
      return '<span class="vfo-web-app__stage-badge">' + escapeHtml(stage.label + " " + stage.count) + '</span>';
    }).join("");

    if (data.sourceRunUrl) {
      container.querySelector(".vfo-web-app__source").classList.add("has-link");
    }
  }

  function bindInteractions(container, data) {
    var selectedNodeId = data.selectedNode;
    var selectedAssetName = data.selectedAsset;

    container.addEventListener("click", function (event) {
      var assetButton = event.target.closest("[data-asset]");
      var nodeButton = event.target.closest("[data-node]");

      if (assetButton) {
        selectedAssetName = assetButton.getAttribute("data-asset");
        data.selectedAsset = selectedAssetName;
        container.dataset.activeAsset = selectedAssetName;
        renderAssets(container, data, data.assets.find(function (asset) { return asset.name === selectedAssetName; }) || data.assets[0], data.workflow.details[selectedNodeId] || data.workflow.details.encode);
        updateInspector(container, data, data.workflow.details[selectedNodeId] || data.workflow.details.encode);
        return;
      }

      if (nodeButton) {
        selectedNodeId = nodeButton.getAttribute("data-node");
        data.selectedNode = selectedNodeId;
        container.dataset.activeNode = selectedNodeId;
        var activeNode = data.workflow.details[selectedNodeId] || data.workflow.details.encode;
        renderWorkflow(container, data, activeNode);
        updateInspector(container, data, activeNode);
      }
    });
  }

  Array.prototype.forEach.call(roots, function (root) {
    getData(root).then(function (data) {
      render(root, data);
    });
  });
})();
