export const flowPresets = {
  executive: {
    label: "Executive Pipeline",
    nodes: [
      makeNode("A", "Mezzanine Ingest", 20, 130, "stage"),
      makeNode("B", "Mezzanine Clean", 220, 130, "stage"),
      makeNode("C", "KEEP_SOURCE?", 440, 130, "gate"),
      makeNode("D", "Source Normalize", 640, 60, "stage"),
      makeNode("E", "Profile Outputs", 640, 200, "output"),
      makeNode("F", "QUALITY_CHECK_ENABLED?", 900, 130, "gate"),
      makeNode("G", "PSNR / SSIM / optional VMAF", 1120, 60, "stage"),
      makeNode("H", "Complete", 1120, 220, "output")
    ],
    edges: [
      makeEdge("A", "B"),
      makeEdge("B", "C"),
      makeEdge("C", "D", "true"),
      makeEdge("C", "E", "false"),
      makeEdge("D", "E"),
      makeEdge("E", "F"),
      makeEdge("F", "G", "true"),
      makeEdge("F", "H", "false"),
      makeEdge("G", "H")
    ]
  },
  operator: {
    label: "Operator Stage View",
    nodes: [
      makeNode("C0", "vfo wizard", 20, 170, "cmd"),
      makeNode("C1", "vfo doctor + vfo status", 230, 170, "cmd"),
      makeNode("C2", "vfo run", 490, 170, "cmd"),
      makeNode("S1", "stage.mezzanine_clean", 650, 70, "stage"),
      makeNode("S2", "stage.mezzanine", 650, 155, "stage"),
      makeNode("G1", "KEEP_SOURCE?", 650, 240, "gate"),
      makeNode("S3", "stage.source", 900, 210, "stage"),
      makeNode("S4", "stage.profiles", 900, 295, "stage"),
      makeNode("G2", "QUALITY_CHECK_ENABLED?", 1120, 295, "gate"),
      makeNode("S5", "stage.quality", 1340, 230, "stage"),
      makeNode("O1", "Outputs Ready", 1340, 345, "output")
    ],
    edges: [
      makeEdge("C0", "C1"),
      makeEdge("C1", "C2"),
      makeEdge("C2", "S1"),
      makeEdge("S1", "S2"),
      makeEdge("S2", "G1"),
      makeEdge("G1", "S3", "Yes"),
      makeEdge("G1", "S4", "No"),
      makeEdge("S3", "S4"),
      makeEdge("S4", "G2"),
      makeEdge("G2", "S5", "Yes"),
      makeEdge("G2", "O1", "No"),
      makeEdge("S5", "O1")
    ]
  },
  engine: {
    label: "Engine Status Keys",
    nodes: [
      makeNode("E0", "engine.snapshot", 20, 170, "stage"),
      makeNode("C0", "config.directory", 205, 170, "stage"),
      makeNode("D0", "dependency.*", 390, 170, "stage"),
      makeNode("S0", "storage.*", 550, 170, "stage"),
      makeNode("P0", "profiles.detected + profile.*", 700, 170, "stage"),
      makeNode("G0", "stage.mezzanine ready?", 930, 170, "gate"),
      makeNode("X0", "stage.execute blocked", 1150, 90, "warn"),
      makeNode("M0", "stage.mezzanine", 1150, 230, "stage"),
      makeNode("G1", "stage.source enabled?", 1335, 230, "gate"),
      makeNode("SRC", "stage.source", 1540, 170, "stage"),
      makeNode("PRF", "stage.profiles", 1540, 280, "stage"),
      makeNode("G2", "stage.quality enabled?", 1745, 280, "gate"),
      makeNode("Q0", "stage.quality", 1945, 220, "stage"),
      makeNode("DONE", "stage.execute complete", 1945, 340, "output")
    ],
    edges: [
      makeEdge("E0", "C0"),
      makeEdge("C0", "D0"),
      makeEdge("D0", "S0"),
      makeEdge("S0", "P0"),
      makeEdge("P0", "G0"),
      makeEdge("G0", "X0", "No"),
      makeEdge("G0", "M0", "Yes"),
      makeEdge("M0", "G1"),
      makeEdge("G1", "SRC", "Yes"),
      makeEdge("G1", "PRF", "No"),
      makeEdge("SRC", "PRF"),
      makeEdge("PRF", "G2"),
      makeEdge("G2", "Q0", "Yes"),
      makeEdge("G2", "DONE", "No"),
      makeEdge("Q0", "DONE")
    ]
  }
};

function makeNode(id, label, x, y, kind) {
  return {
    id,
    data: { label },
    position: { x, y },
    style: {
      borderRadius: "12px",
      padding: "8px 10px",
      lineHeight: "1.3",
      fontSize: "12px",
      maxWidth: "240px",
      color: nodePalette[kind].text,
      border: "1.5px solid " + nodePalette[kind].border,
      background: nodePalette[kind].bg
    }
  };
}

function makeEdge(source, target, label = "") {
  return {
    id: source + "_" + target + "_" + label.replace(/\s+/g, "_"),
    source,
    target,
    label
  };
}

const nodePalette = {
  stage: { bg: "#e0f2fe", border: "#0284c7", text: "#0c4a6e" },
  gate: { bg: "#fff7ed", border: "#f59e0b", text: "#7c2d12" },
  output: { bg: "#dcfce7", border: "#16a34a", text: "#14532d" },
  warn: { bg: "#fef2f2", border: "#dc2626", text: "#7f1d1d" },
  cmd: { bg: "#f5f3ff", border: "#7c3aed", text: "#4c1d95" }
};

