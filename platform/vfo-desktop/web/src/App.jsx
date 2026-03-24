import { useMemo, useState } from "react";
import { Background, BackgroundVariant, Controls, ReactFlow } from "@xyflow/react";
import { flowPresets } from "./flow-presets";
import "./app.css";

const presetKeys = Object.keys(flowPresets);

export default function App() {
  const [presetKey, setPresetKey] = useState("executive");
  const preset = flowPresets[presetKey];

  const nodes = useMemo(() => preset.nodes, [preset]);
  const edges = useMemo(() => preset.edges, [preset]);

  return (
    <main className="shell">
      <header className="topbar">
        <h1>vfo Desktop Flow Canvas</h1>
        <p>React Flow scaffold for the Tauri desktop shell.</p>
      </header>

      <section className="controls">
        {presetKeys.map((key) => (
          <button
            key={key}
            type="button"
            className={key === presetKey ? "chip chip-active" : "chip"}
            onClick={() => setPresetKey(key)}
          >
            {flowPresets[key].label}
          </button>
        ))}
      </section>

      <section className="canvas-card">
        <ReactFlow
          nodes={nodes}
          edges={edges}
          fitView
          fitViewOptions={{ padding: 0.16 }}
          nodesDraggable={false}
          nodesConnectable={false}
          elementsSelectable={false}
          minZoom={0.35}
          maxZoom={2}
          proOptions={{ hideAttribution: true }}
        >
          <Background variant={BackgroundVariant.Dots} gap={20} size={1} />
          <Controls showInteractive={false} position="bottom-right" />
        </ReactFlow>
      </section>
    </main>
  );
}
