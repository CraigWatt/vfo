# vfo Documentation

vfo is an autonomous media optimization engine centered on three layers:

- `mezzanine` (high-quality working inputs)
- `source` (normalized intermediate, optional)
- `profile` (delivery target)

The design goal is outcome ownership:

- detect input characteristics
- decide via profile/scenario rules
- execute deterministic profile actions
- verify results with observability and optional quality scoring

This site is built to answer three practical questions quickly:

- Which stock profile should I use?
- What exactly will a profile do to my media?
- How does vfo flow from input to output?

## Start Here

1. Read [Pipeline](pipeline-end-to-end.md) for end-to-end behavior.
2. Read [Flow Levels](flow-levels.md) for interactive executive/operator/engine React Flow visuals.
3. Check [Capability Matrix](profile-capability-matrix.md) to see what each stock profile targets.
4. Open [Profile Info Sheets](profiles/index.md) for deep behavior details per profile.
5. Review [Latest E2E Toolchain Report](e2e-toolchain-latest.md) for the dependency versions used in recent CI verification.
6. Read [Deployment Modes](deployment-modes.md) to choose between local native, desktop-managed, and container lanes.

## Install and Verify

```bash
vfo --version
vfo doctor
vfo status
vfo visualize
```

## Notes

- Profile pages are generated from live stock presets and action scripts.
- If profile behavior changes in code, docs are regenerated from source to reduce drift.
