# Task Packets

This repository uses small task packets to keep Codex context lean.

## Packet Shape

Use this structure when opening or handing off a bounded task:

```md
# Task Packet

## Objective
- one sentence describing the exact change

## Subsystem
- primary subsystem name

## Likely Files
- file or directory shortlist only

## Constraints
- behavior rules, naming rules, compatibility rules, or non-goals

## Verification
- expected commands or evidence bar

## Escalation Trigger
- what would justify moving from `GPT-5.4 mini first` to `GPT-5.4 high now`
```

## Routing Guidance

- Default to `GPT-5.4 mini first` for triage, file discovery, docs, issue shaping, and bounded cleanup.
- Use `GPT-5.4 high now` for correctness-sensitive logic, subtle regressions, architecture-sensitive work, or stalled lightweight attempts.
- One good lightweight pass is enough. Do not keep expanding the packet just to avoid escalation.

## Packet Rules

- Keep packets scoped to one repo problem.
- Prefer a short likely-file list over a broad directory dump.
- Always name the expected verification bar.
- If the packet is derived from a GitHub issue, mirror the issue title and preserve the risk notes from routing.
