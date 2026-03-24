(function () {
  "use strict";

  var roots = new Map();
  var libsPromise = null;

  function createNode(id, label, x, y, kind) {
    var styles = {
      stage: {
        background: "#e0f2fe",
        border: "1.5px solid #0284c7",
        color: "#0c4a6e"
      },
      gate: {
        background: "#fff7ed",
        border: "1.5px solid #f59e0b",
        color: "#7c2d12"
      },
      output: {
        background: "#dcfce7",
        border: "1.5px solid #16a34a",
        color: "#14532d"
      },
      warn: {
        background: "#fef2f2",
        border: "1.5px solid #dc2626",
        color: "#7f1d1d"
      },
      cmd: {
        background: "#f5f3ff",
        border: "1.5px solid #7c3aed",
        color: "#4c1d95"
      }
    };

    return {
      id: id,
      data: { label: label },
      position: { x: x, y: y },
      style: Object.assign(
        {
          borderRadius: "10px",
          fontSize: "12px",
          lineHeight: "1.3",
          padding: "8px 10px",
          maxWidth: "220px"
        },
        styles[kind] || styles.stage
      )
    };
  }

  var FLOWS = {
    "level-1": {
      nodes: [
        createNode("A", "Mezzanine Ingest", 10, 90, "stage"),
        createNode("B", "Mezzanine Clean (optional)", 200, 90, "stage"),
        createNode("C", "KEEP_SOURCE?", 420, 90, "gate"),
        createNode("D", "Source Normalize", 620, 30, "stage"),
        createNode("E", "Profile Outputs", 620, 150, "output"),
        createNode("F", "QUALITY_CHECK_ENABLED?", 860, 90, "gate"),
        createNode("G", "PSNR / SSIM / optional VMAF", 1080, 30, "stage"),
        createNode("H", "Complete", 1080, 150, "output")
      ],
      edges: [
        { id: "eAB", source: "A", target: "B" },
        { id: "eBC", source: "B", target: "C" },
        { id: "eCD", source: "C", target: "D", label: "true" },
        { id: "eCE", source: "C", target: "E", label: "false" },
        { id: "eDE", source: "D", target: "E" },
        { id: "eEF", source: "E", target: "F" },
        { id: "eFG", source: "F", target: "G", label: "true" },
        { id: "eFH", source: "F", target: "H", label: "false" },
        { id: "eGH", source: "G", target: "H" }
      ]
    },
    "level-2": {
      nodes: [
        createNode("C0", "vfo wizard", 10, 120, "cmd"),
        createNode("C1", "vfo doctor + vfo status", 220, 120, "cmd"),
        createNode("C2", "vfo run", 470, 120, "cmd"),
        createNode("S1", "Stage: mezzanine-clean", 620, 30, "stage"),
        createNode("S2", "Stage: mezzanine", 620, 110, "stage"),
        createNode("G1", "KEEP_SOURCE=true?", 620, 190, "gate"),
        createNode("S3", "Stage: source", 860, 150, "stage"),
        createNode("S4", "Stage: profiles", 860, 230, "stage"),
        createNode("G2", "QUALITY_CHECK_ENABLED=true?", 1090, 230, "gate"),
        createNode("S5", "Stage: quality scoring", 1320, 170, "stage"),
        createNode("O1", "Profile outputs ready", 1320, 280, "output")
      ],
      edges: [
        { id: "l2e1", source: "C0", target: "C1" },
        { id: "l2e2", source: "C1", target: "C2" },
        { id: "l2e3", source: "C2", target: "S1" },
        { id: "l2e4", source: "S1", target: "S2" },
        { id: "l2e5", source: "S2", target: "G1" },
        { id: "l2e6", source: "G1", target: "S3", label: "Yes" },
        { id: "l2e7", source: "G1", target: "S4", label: "No" },
        { id: "l2e8", source: "S3", target: "S4" },
        { id: "l2e9", source: "S4", target: "G2" },
        { id: "l2e10", source: "G2", target: "S5", label: "Yes" },
        { id: "l2e11", source: "G2", target: "O1", label: "No" },
        { id: "l2e12", source: "S5", target: "O1" }
      ]
    },
    "level-3": {
      nodes: [
        createNode("E0", "engine.snapshot", 10, 130, "stage"),
        createNode("C0", "config.directory", 180, 130, "stage"),
        createNode("D0", "dependency.ffmpeg / ffprobe / mkvmerge / dovi_tool", 350, 130, "stage"),
        createNode("S0", "storage.mezzanine[index] / storage.source[index]", 660, 130, "stage"),
        createNode("P0", "profiles.detected + profile.*.scenarios", 960, 130, "stage"),
        createNode("G0", "stage.mezzanine ready?", 1240, 130, "gate"),
        createNode("X0", "stage.execute blocked", 1470, 60, "warn"),
        createNode("M0", "stage.mezzanine", 1470, 170, "stage"),
        createNode("G1", "stage.source enabled?", 1680, 170, "gate"),
        createNode("SRC", "stage.source", 1890, 120, "stage"),
        createNode("PRF", "stage.profiles", 1890, 220, "stage"),
        createNode("G2", "stage.quality enabled?", 2090, 220, "gate"),
        createNode("Q0", "stage.quality", 2290, 160, "stage"),
        createNode("DONE", "stage.execute complete", 2290, 280, "output")
      ],
      edges: [
        { id: "l3e1", source: "E0", target: "C0" },
        { id: "l3e2", source: "C0", target: "D0" },
        { id: "l3e3", source: "D0", target: "S0" },
        { id: "l3e4", source: "S0", target: "P0" },
        { id: "l3e5", source: "P0", target: "G0" },
        { id: "l3e6", source: "G0", target: "X0", label: "No" },
        { id: "l3e7", source: "G0", target: "M0", label: "Yes" },
        { id: "l3e8", source: "M0", target: "G1" },
        { id: "l3e9", source: "G1", target: "SRC", label: "Yes" },
        { id: "l3e10", source: "G1", target: "PRF", label: "No" },
        { id: "l3e11", source: "SRC", target: "PRF" },
        { id: "l3e12", source: "PRF", target: "G2" },
        { id: "l3e13", source: "G2", target: "Q0", label: "Yes" },
        { id: "l3e14", source: "G2", target: "DONE", label: "No" },
        { id: "l3e15", source: "Q0", target: "DONE" }
      ]
    }
  };

  function loadLibs() {
    if (!libsPromise) {
      libsPromise = Promise.all([
        import("https://esm.sh/react@18.3.1"),
        import("https://esm.sh/react-dom@18.3.1/client"),
        import("https://esm.sh/@xyflow/react@12.9.2")
      ]);
    }
    return libsPromise;
  }

  function renderDiagram(container, libs) {
    var flowKey = container.getAttribute("data-vfo-reactflow");
    var flow = FLOWS[flowKey];
    if (!flow) {
      container.textContent = "Unknown React Flow preset: " + flowKey;
      return;
    }

    var React = libs[0].default;
    var ReactDOMClient = libs[1];
    var XYFlow = libs[2];
    var createElement = React.createElement;

    var app = createElement(
      XYFlow.ReactFlowProvider,
      null,
      createElement(
        XYFlow.ReactFlow,
        {
          nodes: flow.nodes,
          edges: flow.edges,
          fitView: true,
          fitViewOptions: { padding: 0.16 },
          nodesDraggable: false,
          nodesConnectable: false,
          elementsSelectable: false,
          zoomOnScroll: true,
          panOnDrag: true,
          minZoom: 0.35,
          maxZoom: 2,
          proOptions: { hideAttribution: true }
        },
        createElement(XYFlow.Background, {
          variant: XYFlow.BackgroundVariant.Dots,
          gap: 20,
          size: 1
        }),
        createElement(XYFlow.Controls, {
          showInteractive: false,
          position: "bottom-right"
        })
      )
    );

    var root = ReactDOMClient.createRoot(container);
    roots.set(container, root);
    root.render(app);
    container.setAttribute("data-reactflow-mounted", "1");
  }

  function unmountDetached() {
    roots.forEach(function (root, container) {
      if (!document.body.contains(container)) {
        root.unmount();
        roots.delete(container);
      }
    });
  }

  function mountReactFlows() {
    var containers = document.querySelectorAll("[data-vfo-reactflow]");
    if (!containers.length) {
      return;
    }

    loadLibs()
      .then(function (libs) {
        for (var i = 0; i < containers.length; i += 1) {
          if (containers[i].getAttribute("data-reactflow-mounted") === "1") {
            continue;
          }
          renderDiagram(containers[i], libs);
        }
      })
      .catch(function (error) {
        console.error("[vfo-docs] React Flow load failed", error);
      });
  }

  function refresh() {
    unmountDetached();
    mountReactFlows();
  }

  if (window.document$ && typeof window.document$.subscribe === "function") {
    window.document$.subscribe(function () {
      refresh();
    });
  } else {
    document.addEventListener("DOMContentLoaded", function () {
      refresh();
    });
  }
})();
