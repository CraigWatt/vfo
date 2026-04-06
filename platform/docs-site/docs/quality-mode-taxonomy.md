# Quality Mode Taxonomy

This page locks the canonical quality-mode vocabulary for current and future `craigstreamy` profiles.

The goal is to avoid turning quality policy into a second explosion of pack names. Subtitle, audio, and quality should compose cleanly.

## Why This Exists

`vfo` already has a real quality-scoring stage:

- PSNR
- SSIM
- optional VMAF via `libvmaf`
- minimum threshold gates via `QUALITY_CHECK_MIN_*`

What it does **not** have yet is an iterative "keep lowering bitrate until the score floor is hit" mode.

This page defines the policy model for that next step.

## Canonical Terms

The canonical quality-mode terms are:

- `standard`
- `aggressive_vmaf`

These are the only terms new docs should use for the quality-mode layer unless we formally add more later.

## Current Implementation Boundary

Today, `vfo` supports:

- post-profile quality scoring
- optional strict threshold gating
- optional VMAF scoring when `ffmpeg` exposes `libvmaf`

Today, `vfo` does **not** yet support:

- iterative bitrate search
- bitrate re-tries driven by VMAF outcomes
- content-class-specific VMAF targets
- model-aware switching between HDTV, phone, and 4K VMAF models

So the current quality story is **measurement + gate**, not **measurement + search**.

## Quality Modes

### `standard`

Run the normal profile action, then score the output afterwards.

Use this when:

- you want predictable encode behavior
- you want pass/fail guardrails
- you do not want the encoder looping to find a lower bitrate

This is the current live quality mode in the repository.

### `aggressive_vmaf`

Treat VMAF as a bounded optimization target for **video bitrate only**.

The intent is:

- keep the pack's subtitle policy
- keep the pack's audio policy
- keep the pack's container policy
- make video encoding more aggressive **only until** the VMAF floor would be missed

This is the future quality mode for `craigstreamy`.

## What `aggressive_vmaf` Means

`aggressive_vmaf` should mean:

- video-only optimization
- full-reference quality evaluation
- bounded encode retry loop
- preserve the lowest bitrate candidate that still satisfies the configured quality floor

It should **not** mean:

- touch audio policy
- force a new subtitle strategy
- blindly reuse one VMAF model for every viewing condition
- retry forever until the machine melts

## Can VMAF Be Used On Every Video Asset?

Not safely as an **absolute** quality gate for every case.

Based on Netflix's own VMAF documentation:

- the default model is trained for a 1080p HDTV viewed from roughly `3H`
- there are different models for phone and 4K viewing conditions
- native low-resolution scoring can look misleadingly high because viewing assumptions change
- VMAF was designed around compression and scaling artifacts in HTTP adaptive streaming

Sources:

- [VMAF models documentation](https://github.com/Netflix/vmaf/blob/master/resource/doc/models.md)
- [VMAF FAQ](https://raw.githubusercontent.com/Netflix/vmaf/master/resource/doc/faq.md)
- [FFmpeg libvmaf filter docs](https://www.ffmpeg.org/ffmpeg-filters.html#libvmaf)

Practical inference for `vfo`:

- we can use VMAF on most transcode outputs when we have a trustworthy reference
- we should not pretend one absolute VMAF score means the same thing on every asset, resolution, or viewing target
- the safest first rollout is progressive SDR/BT.709 style lanes before HDR/DV edge cases

## Model Selection

The VMAF model should be explicit, not accidental.

Recommended direction:

- `auto`
  - HDTV model for normal HD-style delivery scoring
  - 4K model when the target is genuinely 4K and we want an absolute 4K-quality judgement
  - phone model only for mobile-oriented quality evaluation
- `hd`
- `4k`
- `phone`

This matters because Netflix's own VMAF docs say the different models were trained for different viewing conditions.

## Search Strategy

The cleanest `aggressive_vmaf` design is a bounded retry loop:

1. start from the normal profile encode target
2. score against the reference
3. if the score is comfortably above the floor, lower the video bitrate
4. repeat until the next step would miss the floor or the minimum bitrate bound is hit
5. keep the lowest passing candidate

Recommended first implementation shape:

- maximum passes per output: `3` to `5`
- lower bitrate in bounded steps, not unbounded tiny nudges
- stop early when a candidate drops below the configured VMAF floor
- retain the best lowest-bitrate passing candidate

This should be treated as a **search budget**, not an infinite optimizer.

## Pooling Strategy

Netflix's VMAF FAQ notes that the plain arithmetic mean can bias easy content too much, and the tooling already exposes other pooling methods such as:

- `harmonic_mean`
- `median`
- `min`
- `perc5`
- `perc10`
- `perc20`

Source:

- [VMAF FAQ](https://raw.githubusercontent.com/Netflix/vmaf/master/resource/doc/faq.md)

Practical inference for `craigstreamy`:

- `standard` mode can keep simple mean-based gating
- `aggressive_vmaf` should likely use a more conservative pool such as `harmonic_mean` or `perc5`

That gives us a better chance of avoiding "average score looks fine, ugly shots got sacrificed" behavior.

## Should Users Define The Minimum Score?

Yes.

The right model is:

- pack default
- user override

So the pack can say "this profile family wants a high-quality target", but operators can still decide how hard they want to push.

Recommended future knobs:

- `QUALITY_MODE=standard|aggressive_vmaf`
- `QUALITY_VMAF_MODEL=auto|hd|4k|phone`
- `QUALITY_VMAF_MIN=<score>`
- `QUALITY_VMAF_POOL=mean|harmonic_mean|perc5|...`
- `QUALITY_VMAF_MAX_PASSES=<n>`
- `QUALITY_VMAF_MIN_BITRATE_RATIO=<ratio>`

## Should Thresholds Vary By Content Type?

Probably yes, but not by "mystery genre AI" in the first implementation.

The real problem is that content classes behave differently:

- clean live action
- grain-heavy live action
- animation / flat-color material
- archival / noisy sources

That is a better first taxonomy than trying to infer "`South Park` vs `Dune`" as a cinematic label.

Recommended rollout:

1. one conservative default content class
2. optional operator override for content class
3. only later, heuristic auto-classification if we can prove it helps more than it hurts

## Recommended First Rollout For `craigstreamy`

Use `aggressive_vmaf` as a quality mode on top of existing `craigstreamy` packs, not as a whole new universe of pack names.

Recommended first target:

- HEVC video lanes
- progressive SDR/BT.709 first
- leave audio policy untouched
- leave subtitle policy untouched
- bounded retry loop
- user-overridable VMAF floor

That means the first high-value path is likely:

- existing `craigstreamy_hevc_smart_eng_sub_audio_conform`
- plus future `QUALITY_MODE=aggressive_vmaf`

instead of creating a totally separate pack immediately.

## Non-Goals

This taxonomy does not mean:

- VMAF alone understands all perceptual quality concerns
- HDR/DV are already solved for aggressive iteration
- the first version should auto-detect every content type
- audio bitrate lowering belongs in this mode

## Sources

- [Netflix VMAF repository](https://github.com/Netflix/vmaf)
- [VMAF models documentation](https://github.com/Netflix/vmaf/blob/master/resource/doc/models.md)
- [VMAF FAQ](https://raw.githubusercontent.com/Netflix/vmaf/master/resource/doc/faq.md)
- [FFmpeg libvmaf filter docs](https://www.ffmpeg.org/ffmpeg-filters.html#libvmaf)
- [Netflix TechBlog: Per-Title Encode Optimization](https://netflixtechblog.com/per-title-encode-optimization-7e99442b62a2)
