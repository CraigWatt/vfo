#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, List


DOC_FILES = [
    "AGENTS.md",
    "objectives.md",
    "architecture.md",
    "subsystems.md",
    "testing.md",
    "escalation.md",
]


SUBSYSTEM_RULES = [
    {
        "name": "profile routing",
        "paths": [
            "services/vfo/src/Profile/",
            "services/vfo/presets/",
        ],
        "keywords": [
            "profile",
            "scenario",
            "bitrate",
            "device target",
            "preset",
            "matching",
            "hevc",
            "h264",
        ],
        "risk": "high",
        "checks": ["make tests", "make e2e"],
    },
    {
        "name": "source stage",
        "paths": [
            "services/vfo/src/Source/",
            "services/vfo/src/Source_AS/",
        ],
        "keywords": [
            "source",
            "normalize",
            "normalise",
            "intermediate",
            "remux",
        ],
        "risk": "high",
        "checks": ["make tests", "make e2e"],
    },
    {
        "name": "input handling",
        "paths": ["services/vfo/src/InputHandler/"],
        "keywords": [
            "input",
            "scan",
            "discovery",
            "input path",
            "folder",
            "ingest",
        ],
        "risk": "high",
        "checks": ["make tests"],
    },
    {
        "name": "mezzanine stage",
        "paths": [
            "services/vfo/src/Mezzanine/",
            "services/vfo/src/Mezzanine_Clean/",
        ],
        "keywords": [
            "mezzanine",
            "clean",
            "cleanup",
            "hygiene",
            "rename",
        ],
        "risk": "medium",
        "checks": ["make tests", "make e2e"],
    },
    {
        "name": "quality scoring",
        "paths": ["services/vfo/src/quality/"],
        "keywords": [
            "psnr",
            "ssim",
            "vmaf",
            "quality",
            "threshold",
            "gate",
        ],
        "risk": "high",
        "checks": ["make tests", "make e2e"],
    },
    {
        "name": "status and observability",
        "paths": ["services/vfo/src/Status/"],
        "keywords": [
            "status",
            "status-json",
            "visualize",
            "visualise",
            "observability",
            "doctor",
        ],
        "risk": "medium",
        "checks": ["make tests"],
    },
    {
        "name": "config and utilities",
        "paths": [
            "services/vfo/src/Config/",
            "services/vfo/src/Utils/",
        ],
        "keywords": [
            "config",
            "configuration",
            "env var",
            "setting",
            "helper",
            "utility",
        ],
        "risk": "medium",
        "checks": ["make tests"],
    },
    {
        "name": "profile action scripts",
        "paths": ["services/vfo/actions/"],
        "keywords": [
            "ffmpeg",
            "transcode",
            "action script",
            "audio stream",
            "subtitle",
            "crop",
            "dolby vision",
        ],
        "risk": "high",
        "checks": ["make e2e"],
    },
    {
        "name": "GitHub workflows",
        "paths": [".github/workflows/"],
        "keywords": [
            "workflow",
            "action",
            "actions",
            "ci",
            "release",
            "autofix",
            "issue routing",
        ],
        "risk": "high",
        "checks": ["make tests"],
    },
    {
        "name": "docs and navigation",
        "paths": [
            "platform/docs-site/",
            "README.md",
            "mkdocs.yml",
        ],
        "keywords": [
            "docs",
            "documentation",
            "readme",
            "mkdocs",
            "nav",
            "navigation",
            "typo",
            "markdown",
        ],
        "risk": "low",
        "checks": ["make docs-build"],
    },
]


HIGH_RISK_LABELS = {
    "agent-ready",
    "codex",
    "codex-ready",
    "high-risk",
    "release",
    "critical",
}

LOW_RISK_LABELS = {
    "docs",
    "documentation",
    "triage",
    "mini",
    "mini-first",
    "low-reasoning",
    "low-risk",
}

HIGH_RISK_TERMS = [
    "incorrect",
    "regression",
    "crash",
    "corrupt",
    "broken",
    "bitrate",
    "quality gate",
    "workflow failure",
    "release",
    "autofix",
    "data loss",
]

LOW_RISK_TERMS = [
    "typo",
    "wording",
    "docs only",
    "documentation",
    "readme",
    "markdown",
    "summary",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Classify a GitHub issue for AI routing.")
    parser.add_argument("--issue-json", required=True, help="Path to the issue JSON payload.")
    parser.add_argument("--repo-root", required=True, help="Repository root.")
    parser.add_argument("--output-json", required=True, help="Path to write machine-readable output.")
    parser.add_argument(
        "--output-markdown",
        required=True,
        help="Path to write the issue comment markdown.",
    )
    return parser.parse_args()


def load_issue(path: Path) -> Dict[str, object]:
    return json.loads(path.read_text())


def load_docs(repo_root: Path) -> Dict[str, str]:
    docs = {}
    for doc_name in DOC_FILES:
        doc_path = repo_root / doc_name
        if doc_path.exists():
            docs[doc_name] = doc_path.read_text()
    return docs


def normalize_labels(raw_labels: List[object]) -> List[str]:
    labels = []
    for label in raw_labels:
        if isinstance(label, dict):
            name = str(label.get("name", "")).strip().lower()
        else:
            name = str(label).strip().lower()
        if name:
            labels.append(name)
    return labels


def detect_subsystems(text: str) -> List[Dict[str, object]]:
    matched = []
    lowered = text.lower()
    for rule in SUBSYSTEM_RULES:
        if any(keyword in lowered for keyword in rule["keywords"]):
            matched.append(rule)
    if not matched:
        matched.append(
            {
                "name": "general repository triage",
                "paths": ["README.md", "services/vfo/", ".github/workflows/"],
                "risk": "medium",
                "checks": ["make tests"],
            }
        )
    return matched


def dedupe(values: List[str]) -> List[str]:
    seen = set()
    ordered = []
    for value in values:
        if value not in seen:
            seen.add(value)
            ordered.append(value)
    return ordered


def score_risk(text: str, labels: List[str], subsystems: List[Dict[str, object]]) -> Dict[str, object]:
    score = 0
    reasons: List[str] = []

    if any(label in HIGH_RISK_LABELS for label in labels):
        score += 4
        reasons.append("The issue carries a label that implies explicit premium escalation or higher risk.")

    if any(label in LOW_RISK_LABELS for label in labels):
        score -= 2
        reasons.append("The issue labels suggest a docs, triage, or cheap-first first pass.")

    lowered = text.lower()
    for term in HIGH_RISK_TERMS:
        if term in lowered:
            score += 2
            reasons.append(f"The issue mentions `{term}`, which usually needs tighter behavior control.")

    for term in LOW_RISK_TERMS:
        if term in lowered:
            score -= 1
            reasons.append(f"The issue mentions `{term}`, which is often safe for a cheap first pass.")

    for subsystem in subsystems:
        risk = subsystem["risk"]
        if risk == "high":
            score += 2
            reasons.append(
                f"The likely subsystem `{subsystem['name']}` is treated as correctness-sensitive in repo policy."
            )
        elif risk == "medium":
            score += 1
            reasons.append(f"The likely subsystem `{subsystem['name']}` usually needs targeted verification.")

    if len(subsystems) >= 3:
        score += 2
        reasons.append("The issue touches multiple subsystems, which increases routing and verification risk.")

    if score >= 6:
        risk_level = "high"
    elif score >= 2:
        risk_level = "medium"
    else:
        risk_level = "low"

    if risk_level == "high" or any(label in {"codex", "codex-ready", "agent-ready"} for label in labels):
        route = "GPT-5.4 high now"
    else:
        route = "GPT-5.4 mini first"

    if len(reasons) >= 4:
        confidence = "high"
    elif len(reasons) >= 2:
        confidence = "medium"
    else:
        confidence = "low"

    return {
        "risk_level": risk_level,
        "route": route,
        "confidence": confidence,
        "reasons": dedupe(reasons),
    }


def verification_bar(subsystems: List[Dict[str, object]], route: str) -> List[str]:
    checks: List[str] = []
    for subsystem in subsystems:
        checks.extend(subsystem["checks"])

    if route == "GPT-5.4 high now" and "make ci" not in checks:
        checks.append("make ci")

    return dedupe(checks)


def escalation_triggers(subsystems: List[Dict[str, object]], route: str) -> List[str]:
    triggers = [
        "The lightweight pass reports low confidence or unclear reproduction steps.",
        "A bounded lightweight attempt fails verification or grows beyond the expected file set.",
    ]

    high_risk_subsystems = [subsystem["name"] for subsystem in subsystems if subsystem["risk"] == "high"]
    if high_risk_subsystems and route != "GPT-5.4 high now":
        triggers.append(
            "The task turns out to touch correctness-sensitive areas such as "
            + ", ".join(high_risk_subsystems)
            + "."
        )

    triggers.append("A maintainer explicitly promotes the issue to the premium GPT-5.4 high lane.")
    return dedupe(triggers)


def next_steps(route: str) -> List[str]:
    if route == "GPT-5.4 high now":
        return [
            "Treat this as an explicit premium escalation and read the repo policy files before editing.",
            "Keep the patch small, behavior-safe, and verification-backed.",
        ]
    return [
        "Use GPT-5.4 mini or lower reasoning for one scoped first pass: triage, file shortlist, plan, or bounded patch attempt.",
        "Do not keep retrying the lightweight path if confidence stays low after the first good attempt.",
    ]


def render_comment(
    issue: Dict[str, object],
    docs_used: List[str],
    subsystems: List[Dict[str, object]],
    classification: Dict[str, object],
    checks: List[str],
    triggers: List[str],
) -> str:
    subsystem_names = ", ".join(subsystem["name"] for subsystem in subsystems)
    likely_paths = dedupe(path for subsystem in subsystems for path in subsystem["paths"])

    lines = [
        "<!-- vfo-ai-routing -->",
        "## AI Routing Summary",
        "",
        f"- Risk level: `{classification['risk_level']}`",
        f"- Recommended route: `{classification['route']}`",
        f"- Confidence: `{classification['confidence']}`",
        f"- Likely subsystem(s): `{subsystem_names}`",
        "",
        "Why this route:",
    ]

    for reason in classification["reasons"]:
        lines.append(f"- {reason}")

    lines.extend(
        [
            "",
            "Likely file areas:",
        ]
    )
    for path in likely_paths:
        lines.append(f"- `{path}`")

    lines.extend(
        [
            "",
            "Verification bar:",
        ]
    )
    for check in checks:
        lines.append(f"- `{check}`")

    lines.extend(
        [
            "",
            "Escalation triggers:",
        ]
    )
    for trigger in triggers:
        lines.append(f"- {trigger}")

    lines.extend(
        [
            "",
            "Recommended next step:",
        ]
    )
    for step in next_steps(str(classification["route"])):
        lines.append(f"- {step}")

    lines.extend(
        [
            "",
            "Docs consulted:",
        ]
    )
    for doc_name in docs_used:
        lines.append(f"- `{doc_name}`")

    lines.extend(
        [
            "",
            f"Issue: #{issue['number']} - {issue['title']}",
        ]
    )

    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    issue = load_issue(Path(args.issue_json))
    docs = load_docs(repo_root)

    title = str(issue.get("title", "")).strip()
    body = str(issue.get("body", "") or "").strip()
    labels = normalize_labels(issue.get("labels", []))
    combined_text = "\n".join([title, body, " ".join(labels)])

    subsystems = detect_subsystems(combined_text)
    classification = score_risk(combined_text, labels, subsystems)
    checks = verification_bar(subsystems, str(classification["route"]))
    triggers = escalation_triggers(subsystems, str(classification["route"]))
    docs_used = sorted(docs.keys())

    output = {
        "issue_number": issue.get("number"),
        "issue_title": title,
        "risk_level": classification["risk_level"],
        "route": classification["route"],
        "confidence": classification["confidence"],
        "labels": labels,
        "subsystems": [subsystem["name"] for subsystem in subsystems],
        "likely_paths": dedupe(path for subsystem in subsystems for path in subsystem["paths"]),
        "verification_bar": checks,
        "escalation_triggers": triggers,
        "reasons": classification["reasons"],
        "docs_used": docs_used,
    }

    Path(args.output_json).write_text(json.dumps(output, indent=2) + "\n")
    Path(args.output_markdown).write_text(
        render_comment(issue, docs_used, subsystems, classification, checks, triggers)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
