# vfo Documentation

vfo is a profile-driven media workflow engine centered on three layers:

- `mezzanine` (high-quality working inputs)
- `source` (normalized intermediate, optional)
- `profile` (delivery target)

This site is built to answer three practical questions quickly:

- Which stock profile should I use?
- What exactly will a profile do to my media?
- How does vfo flow from input to output?

## Start Here

1. Read [Pipeline](pipeline-end-to-end.md) for end-to-end behavior.
2. Read [Flow Levels](flow-levels.md) for executive/operator/engine visuals.
3. Check [Capability Matrix](profile-capability-matrix.md) to see what each stock profile targets.
4. Open [Profile Info Sheets](profiles/index.md) for deep behavior details per profile.

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
