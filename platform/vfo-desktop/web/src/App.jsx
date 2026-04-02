import { useMemo, useState } from "react";
import {
  Background,
  BackgroundVariant,
  Controls,
  Handle,
  Position,
  ReactFlow
} from "@xyflow/react";
import "./app.css";

const assets = [
  { name: "movie_01.mxf", status: "Failed", icon: "✖" },
  { name: "movie_02.mxf", status: "Complete", icon: "✔" },
  { name: "movie_03.mxf", status: "Running", icon: "⏳" },
  { name: "movie_04.mxf", status: "Waiting", icon: "○" },
  { name: "movie_05.mxf", status: "Complete", icon: "✔" },
  { name: "movie_06.mxf", status: "Failed", icon: "✖" },
  { name: "movie_07.mxf", status: "Complete", icon: "✔" },
  { name: "movie_08.mxf", status: "Waiting", icon: "○" }
];

const filters = ["All", "Failed", "Running", "Waiting", "Complete"];

const workflowNodes = [
  makeNode("input", "Input", "Singular mezzanine asset", 40, 145, "complete"),
  makeNode("probe", "Probe", "ffprobe summary and QC hints", 270, 145, "complete"),
  makeNode("deint", "Deint", "Optional cleanup / normalization", 500, 145, "running"),
  makeNode("encode", "Encode", "Primary profile action", 730, 145, "failed"),
  makeNode("hls", "HLS", "Delivery packaging", 980, 235, "waiting"),
  makeNode("qc", "QC", "PSNR / SSIM / VMAF", 760, 300, "waiting"),
  makeNode("metadata", "Metadata", "Sidecar manifests and notes", 500, 300, "complete")
];

const workflowEdges = [
  { id: "input-probe", source: "input", target: "probe" },
  { id: "probe-deint", source: "probe", target: "deint" },
  { id: "deint-encode", source: "deint", target: "encode" },
  { id: "encode-hls", source: "encode", target: "hls" },
  { id: "probe-metadata", source: "probe", target: "metadata" },
  { id: "encode-qc", source: "encode", target: "qc" }
];

const stageTotals = [
  { label: "Input", count: 128 },
  { label: "Probe", count: 128 },
  { label: "Deint", count: 97 },
  { label: "Encode", count: 91 },
  { label: "HLS", count: 88 },
  { label: "QC", count: 73 },
  { label: "Metadata", count: 128 }
];

const summaryCounts = [
  { label: "Complete", count: 91, icon: "✔" },
  { label: "Failed", count: 7, icon: "✖" },
  { label: "Running", count: 12, icon: "⏳" },
  { label: "Waiting", count: 18, icon: "○" }
];

const nodeDetails = {
  input: {
    node: "Input",
    status: "Complete",
    exitCode: 0,
    command: "vfo mezzanine movie_01.mxf --scan",
    output: [
      "asset=movie_01.mxf",
      "container=mxf",
      "tracks=video,audio,timecode",
      "result=accepted"
    ]
  },
  probe: {
    node: "Probe",
    status: "Complete",
    exitCode: 0,
    command: "ffprobe -hide_banner -show_streams movie_01.mxf",
    output: [
      "stream.video.codec=hevc",
      "stream.video.height=2160",
      "stream.audio.layout=5.1",
      "result=signal_profile_ready"
    ]
  },
  deint: {
    node: "Deint",
    status: "Running",
    exitCode: null,
    command: "vfo run --stage deint movie_01.mxf",
    output: [
      "phase=normalize",
      "step=filter_chain",
      "progress=68%",
      "result=in_progress"
    ]
  },
  encode: {
    node: "Encode",
    status: "Failed",
    exitCode: 1,
    command: "ffmpeg -i movie_01.mxf -c:v libx265 ...",
    output: [
      "error=encoder initialization failed",
      "stderr=codec context rejected",
      "hint=inspect profile guardrails",
      "result=failed"
    ]
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
    output: [
      "sidecar=movie_01.json",
      "tags=preserved",
      "result=complete"
    ]
  }
};

export default function App() {
  const [selectedAsset, setSelectedAsset] = useState(assets[0].name);
  const [selectedNode, setSelectedNode] = useState("encode");

  const nodes = useMemo(() => workflowNodes, []);
  const edges = useMemo(() => workflowEdges, []);
  const activeAsset = assets.find((asset) => asset.name === selectedAsset) ?? assets[0];
  const activeNode = nodeDetails[selectedNode] ?? nodeDetails.encode;

  return (
    <main className="shell">
      <header className="topbar">
        <div className="brand">
          <h1>VFO</h1>
          <p>Pipeline: UHD SDR ladder</p>
        </div>

        <div className="topbar-meta">
          <span>Run: 2026-04-02 18:42</span>
          <button type="button" className="topbar-chip">
            Search
          </button>
          <button type="button" className="topbar-chip">
            Filters
          </button>
          <button type="button" className="topbar-chip">
            Profile
          </button>
        </div>
      </header>

      <section className="workspace">
        <aside className="panel asset-panel">
          <div className="panel-head">
            <h2>Assets</h2>
            <span>Mezzanine folder</span>
          </div>

          <label className="searchbox" htmlFor="asset-search">
            <span>Search assets...</span>
            <input id="asset-search" type="text" defaultValue="" />
          </label>

          <div className="asset-list" role="list" aria-label="Assets">
            {assets.map((asset) => (
              <button
                key={asset.name}
                type="button"
                className={asset.name === selectedAsset ? "asset-row active" : "asset-row"}
                onClick={() => setSelectedAsset(asset.name)}
              >
                <span className={`asset-dot ${asset.status.toLowerCase()}`} />
                <span className="asset-name">{asset.name}</span>
                <span className="asset-status">{asset.icon}</span>
              </button>
            ))}
          </div>

          <div className="filter-block">
            <h3>Filters</h3>
            <div className="filter-list">
              {filters.map((filter) => (
                <label key={filter} className="filter-item">
                  <input type="checkbox" defaultChecked />
                  <span>{filter}</span>
                </label>
              ))}
            </div>
          </div>
        </aside>

        <section className="panel workflow-panel">
          <div className="panel-head">
            <h2>Workflow</h2>
            <span>All nodes visible by default</span>
          </div>

          <div className="workflow-shell">
            <ReactFlow
              nodes={nodes}
              edges={edges}
              nodeTypes={nodeTypes}
              fitView
              fitViewOptions={{ padding: 0.18 }}
              nodesDraggable={false}
              nodesConnectable={false}
              elementsSelectable={false}
              onNodeClick={(_, node) => setSelectedNode(node.id)}
              minZoom={0.45}
              maxZoom={1.6}
              proOptions={{ hideAttribution: true }}
            >
              <Background variant={BackgroundVariant.Dots} gap={20} size={1} />
              <Controls showInteractive={false} position="bottom-right" />
            </ReactFlow>
          </div>

          <div className="workflow-caption">
            Node status for the selected asset is shown inline, with failures, running, waiting,
            and complete states carried directly on the lane.
          </div>
        </section>

        <aside className="panel inspector-panel">
          <div className="panel-head">
            <h2>Inspector</h2>
            <span>Current selection</span>
          </div>

          <div className="density-map" aria-label="Run totals">
            {summaryCounts.map((item) => (
              <div key={item.label} className="density-item">
                <span className="density-icon">{item.icon}</span>
                <strong>{item.count}</strong>
                <span>{item.label}</span>
              </div>
            ))}
          </div>

          <div className="inspector-card">
            <div className="inspector-row">
              <span>Asset</span>
              <strong>{activeAsset.name}</strong>
            </div>
            <div className="inspector-row">
              <span>Node</span>
              <strong>{activeNode.node}</strong>
            </div>
            <div className="inspector-row">
              <span>Status</span>
              <strong>{activeNode.status}</strong>
            </div>
            <div className="inspector-row">
              <span>Exit code</span>
              <strong>{activeNode.exitCode ?? "pending"}</strong>
            </div>
          </div>

          <div className="code-panel">
            <div className="code-panel-head">
              <span>stdout / stderr / cmd</span>
              <div className="code-actions">
                <button type="button">Copy</button>
                <button type="button">Expand</button>
              </div>
            </div>
            <pre>{[activeNode.command, "", ...activeNode.output].join("\n")}</pre>
          </div>
        </aside>
      </section>

      <footer className="legend-panel">
        <div className="legend-summary">
          <span>⏺ Total: 128</span>
          <span>✔ Complete: 91</span>
          <span>✖ Failed: 7</span>
          <span>⏳ Running: 12</span>
          <span>○ Waiting: 18</span>
        </div>

        <div className="stage-badges">
          {stageTotals.map((stage) => (
            <span key={stage.label} className="stage-badge">
              {stage.label} {stage.count}
            </span>
          ))}
        </div>
      </footer>
    </main>
  );
}

function WorkflowNode({ data }) {
  return (
    <div className={`workflow-node ${data.status}`}>
      <Handle type="target" position={Position.Left} />
      <div className="workflow-node-top">
        <span className="workflow-node-kind">{data.label}</span>
        <span className="workflow-node-status">{statusGlyph[data.status]}</span>
      </div>
      <div className="workflow-node-body">{data.subtitle}</div>
      <Handle type="source" position={Position.Right} />
    </div>
  );
}

const statusGlyph = {
  complete: "✔",
  running: "⏳",
  failed: "✖",
  waiting: "○"
};

const nodeTypes = {
  wire: WorkflowNode
};

function makeNode(id, label, subtitle, x, y, status) {
  return {
    id,
    type: "wire",
    data: { label, subtitle, status },
    position: { x, y }
  };
}
