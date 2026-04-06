# Profile Pack Strategy

This page defines how `vfo` should expose stock profile packs as the feature set grows.

The design goal is:

- clear fixed pack names for users
- composable internal implementation for maintainers
- enough pack variety to support real selection without turning the repo into a cartesian-product graveyard

## Core Decision

The public interface should stay **fixed named profile packs**.

Users should be able to say:

- "I want smart English subtitle preserve"
- "I want all subtitle preserve"
- "I want audio conform"

without needing to understand every implementation detail.

Internally, though, the implementation should be componentized around policy axes so we do not duplicate logic everywhere.

## Public Interface vs Internal Implementation

### Public Interface

Publicly, we should expose named packs that describe meaningful output intent.

Examples:

- `craigstreamy_hevc_smart_eng_sub_audio_conform`
- `craigstreamy_hevc_all_sub_preserve`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert`
- `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`

That keeps pack choice explicit and easy to talk about.

### Internal Implementation

Internally, packs should be assembled from a small set of policy components:

- video strategy
- subtitle selection policy
- subtitle handling policy
- audio policy
- quality mode
- optional content class

This gives us reuse without exposing a messy component system to end users.

## What Should Be A Pack?

Something should become a distinct stock pack when it changes the user-visible output intent in a major way.

That includes:

- subtitle selection changes
- subtitle handling changes
- audio preserve vs audio conform
- codec-family changes
- delivery family changes

Examples:

- `smart_eng_sub` vs `all_sub_preserve`
- `preserve` vs `subtitle_convert`
- `audio_preserve` vs `audio_conform`
- `hevc` vs future `av1_dv_safe`

These are the kinds of choices users actually think in.

## What Should Not Be A Pack First?

Some things should start as modes or knobs instead of immediately becoming new pack names:

- quality mode
- VMAF floor
- VMAF pooling choice
- VMAF model selection
- content class override
- bounded retry count

These settings tune behavior inside a delivery family. They are important, but they do not automatically deserve a new pack name on day one.

The best current example is:

- `standard`
- `aggressive_vmaf`

Those should start as quality modes layered on existing packs.

## Current Stock Packs

Current stock packs in the repository are:

- `balanced_open_audio`
- `device_targets_open_audio`
- `craigstreamy_hevc_selected_english_subtitle_preserve`
- `craigstreamy_hevc_all_sub_preserve`
- `craigstreamy_hevc_smart_eng_sub_audio_conform`
- `craigstreamy_hevc_all_sub_audio_conform`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert`
- `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform`
- `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf`

Conceptually, the current `craigstreamy` packs map like this:

| Pack | Subtitle policy | Audio policy | Quality mode |
| --- | --- | --- | --- |
| `craigstreamy_hevc_selected_english_subtitle_preserve` | `smart_eng_sub + preserve` | preserve | `standard` |
| `craigstreamy_hevc_all_sub_preserve` | `all_sub_preserve + preserve` | preserve | `standard` |
| `craigstreamy_hevc_smart_eng_sub_audio_conform` | `smart_eng_sub + preserve` | `audio_conform` | `standard` |
| `craigstreamy_hevc_all_sub_audio_conform` | `all_sub_preserve + preserve` | `audio_conform` | `standard` |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert` | `smart_eng_sub + subtitle_convert` | preserve | `standard` |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform` | `smart_eng_sub + subtitle_convert` | `audio_conform` | `standard` |
| `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf` | `smart_eng_sub + preserve` | preserve | `aggressive_vmaf` |
| `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf` | `smart_eng_sub + preserve` | `audio_conform` | `aggressive_vmaf` |

## Current Explicit Pack Surface

To give users a meaningful but still manageable pack choice surface, the current shipped craigstreamy set now reads as:

1. shipped: `craigstreamy_hevc_all_sub_preserve`
2. shipped: `craigstreamy_hevc_all_sub_audio_conform`
3. shipped: `craigstreamy_hevc_smart_eng_sub_subtitle_convert`
4. shipped: `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform`
5. shipped: `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`
6. shipped: `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf`

Why this shape:

- they complete the subtitle-policy surface users are already asking for
- they reuse the now-defined audio policy split cleanly
- they expose video-only aggressive VMAF separately from audio-conform aggressive VMAF
- they keep the mental model obvious without exposing internal knobs as the primary UX

## Recommended Pack Matrix

The clean first `craigstreamy` matrix should be:

| Pack | Subtitle selection | Subtitle handling | Audio policy | Video family |
| --- | --- | --- | --- | --- |
| `craigstreamy_hevc_selected_english_subtitle_preserve` | `smart_eng_sub` | `preserve` | preserve | HEVC |
| `craigstreamy_hevc_all_sub_preserve` | `all_sub_preserve` | `preserve` | preserve | HEVC |
| `craigstreamy_hevc_smart_eng_sub_audio_conform` | `smart_eng_sub` | `preserve` | `audio_conform` | HEVC |
| `craigstreamy_hevc_all_sub_audio_conform` | `all_sub_preserve` | `preserve` | `audio_conform` | HEVC |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert` | `smart_eng_sub` | `subtitle_convert` | preserve | HEVC |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform` | `smart_eng_sub` | `subtitle_convert` | `audio_conform` | HEVC |
| `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf` | `smart_eng_sub` | `preserve` | preserve | HEVC |
| `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf` | `smart_eng_sub` | `preserve` | `audio_conform` | HEVC |

That is already a healthy amount of user choice without becoming chaotic.

## Quality Mode Layer

Quality should sit above the pack layer:

- `QUALITY_MODE=standard`
- `QUALITY_MODE=aggressive_vmaf`

This is cleaner than immediately creating:

- `..._all_sub_aggressive_vmaf`
- `..._subtitle_convert_aggressive_vmaf`

We now ship two fixed-name aggressive packs because they make user selection clearer:

- `craigstreamy_hevc_smart_eng_sub_aggressive_vmaf`
- `craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf`

Both still map onto the same internal quality-mode component. They are selection aliases, not a separate implementation family.

That gives us the best of both worlds:

- fixed names for UX
- no duplicate implementation logic

## Content Classes

Content classification should also start as an internal override, not a first-class pack family.

Recommended first content classes:

- `default`
- `grain_heavy`
- `animation`
- `archival`

These should influence quality-mode defaults, not subtitle or audio policy.

## Recommendation

The right operating model is:

1. fixed named packs for meaningful delivery intent
2. internal policy composition under the hood
3. quality mode layered on top
4. content class layered underneath quality mode defaults

That keeps user selection clear while still letting the codebase scale.

## Near-Term Roadmap

If we follow this strategy, the next broad steps are:

1. deepen `aggressive_vmaf` controls and pack-level overrides
2. decide whether `all_sub_preserve + aggressive_vmaf` deserves a fixed-name alias or should remain a mode-only combination
3. decide whether `subtitle_convert + aggressive_vmaf` deserves a fixed-name alias or should remain mode-only
4. decide whether the next fixed-name packs should include AV1 / DV-safe variants
5. strengthen real-media e2e coverage for DTS-family audio and subtitle-convert edge cases
