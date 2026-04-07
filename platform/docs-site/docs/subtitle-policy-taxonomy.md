# Subtitle Policy Taxonomy

This page locks the canonical subtitle-policy vocabulary for current and future `craigstreamy` profile packs.

The goal is to stop subtitle behavior from turning into one-off pack names and instead describe it as a small, composable policy model.

## Why This Exists

We now have a real audio policy story:

- preserve-first audio
- `audio_conform` when needed

Subtitle handling needs the same clarity.

For `craigstreamy`, the default should feel streaming-oriented and intent-aware, not archival by accident. At the same time, we need a path for broader preservation and future subtitle conversion work.

## Canonical Terms

The canonical subtitle-policy terms are:

- `smart_eng_sub`
- `all_sub_preserve`
- `subtitle_convert`

These are the only terms new docs should use for the subtitle-policy layer unless we formally add more later.

## Policy Axes

Treat subtitle behavior as two related decisions:

1. selection scope
2. handling mode

### Selection Scope

#### `smart_eng_sub`

Select one English subtitle track using the current intent heuristic:

- forced english first
- forced untagged or unknown fallback
- optional default english fallback when explicitly enabled
- non-english forced tracks are intentionally ignored for this policy

This is the `craigstreamy` default for most streaming-style output.

#### `all_sub_preserve`

Carry every subtitle stream through when packaging permits it.

This is more archival, curation, or mezzanine-friendly than `smart_eng_sub`. It should not be the default for most `craigstreamy` streaming outputs.

### Handling Mode

#### `preserve`

Keep the chosen subtitle stream or streams in their existing form when packaging allows it.

Examples:

- `smart_eng_sub` + `preserve`
- `all_sub_preserve` + `preserve`

This is the current live behavior in the shipping `craigstreamy` packs.

#### `subtitle_convert`

Normalize subtitles into a delivery-friendly text-first subtitle representation when preservation is not enough.

For `craigstreamy`, this is now a real stock-pack policy. The current live implementation focuses on text subtitle conversion first and keeps bitmap behavior explicit instead of quietly stripping or burning.

## What `subtitle_convert` Means

`subtitle_convert` is not “do anything to subtitles.” It means:

- prefer text subtitle outputs over image-based subtitle carry-over when conversion is required
- preserve language, forced, and default semantics where possible
- keep subtitle handling explicit instead of silently stripping or burning subtitles into video
- normalize into a delivery-oriented subtitle form rather than preserving source quirks

For `craigstreamy`, the practical rule should be:

- if the chosen subtitle is already text-based and target-safe, normalize or rewrap it as needed
- if the chosen subtitle is bitmap-based, only convert when we have a trustworthy OCR or authoring path
- if we do not have a trustworthy conversion path, do not silently drop it

Current implementation detail:

- `smart_eng_sub + subtitle_convert` converts selected text subtitles to `mov_text` when MP4 remains viable
- bitmap subtitles fail by default
- `VFO_SUBTITLE_CONVERT_BITMAP_POLICY=preserve_mkv` allows an explicit MKV-preserve fallback instead
- when `audio_conform` or another safety rule forces MKV, selected subtitles are preserved rather than pretending conversion still happened

That last point matters. `subtitle_convert` should fail or fall back explicitly, not pretend conversion succeeded.

## Netflix-Like Baseline

The closest public Netflix subtitle guidance points to a text-first timed-text workflow:

- timed text created specifically for Netflix should follow Netflix timed-text style guides
- subtitle template deliveries use `IMSC 1.1`
- forced narratives are treated explicitly, not as a vague side effect

Sources:

- [Timed Text Style Guide: Subtitle Templates](https://partnerhelp.netflixstudios.com/hc/en-us/articles/219375728-English-Template-Timed-Text-Style-Guide)
- [Subtitle File Types](https://partnerhelp.netflixstudios.com/hc/en-us/sections/38074500767635-Subtitle-File-Types)

Inference:

- `craigstreamy` should be text-first when conversion is required
- `craigstreamy` should preserve forced/default semantics carefully
- `craigstreamy` should not treat bitmap subtitle carry-over as the only long-term answer for streaming-oriented packs

## Recommended `craigstreamy` Defaults

For most `craigstreamy` outputs:

- default selection scope: `smart_eng_sub`
- default handling mode: `preserve`

Use `all_sub_preserve` when the product goal is broader subtitle carry-over rather than streamlined viewing intent.

Use `subtitle_convert` when delivery compatibility or subtitle normalization is the goal, not when simple preservation is enough.

## Current Mapping

Current stock packs map like this:

| Pack | Subtitle policy |
| --- | --- |
| `craigstreamy_hevc_selected_english_subtitle_preserve` | `smart_eng_sub` + `preserve` |
| `craigstreamy_hevc_smart_eng_sub_audio_conform` | `smart_eng_sub` + `preserve` |
| `craigstreamy_hevc_all_sub_preserve` | `all_sub_preserve` + `preserve` |
| `craigstreamy_hevc_all_sub_audio_conform` | `all_sub_preserve` + `preserve` |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert` | `smart_eng_sub` + `subtitle_convert` |
| `craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform` | `smart_eng_sub` + `subtitle_convert` |
| `roku_family_all_sub_convert_audio_conform` | `all_sub_preserve` + `subtitle_convert` |
| `fire_tv_family_all_sub_convert_audio_conform` | `all_sub_preserve` + `subtitle_convert` |
| `chromecast_google_tv_family_all_sub_convert_audio_conform` | `all_sub_preserve` + `subtitle_convert` |
| `apple_tv_family_all_sub_convert_audio_conform` | `all_sub_preserve` + `subtitle_convert` |
| `fire_tv_stick_4k_dv_all_sub_convert_audio_conform` | `all_sub_preserve` + `subtitle_convert` |

## Future Composition

These are the combinations we should design around:

| Selection scope | Handling mode | Meaning |
| --- | --- | --- |
| `smart_eng_sub` | `preserve` | current streaming-style default |
| `smart_eng_sub` | `subtitle_convert` | selected English subtitle retained, but normalized into delivery text form |
| `all_sub_preserve` | `preserve` | preserve all subtitle streams |
| `all_sub_preserve` | `subtitle_convert` | preserve all subtitle intent, but normalize each convertible stream into delivery text form |

## Non-Goals

This taxonomy does not mean:

- every pack must carry all subtitles
- every pack must OCR bitmap subtitles
- subtitle conversion is safe to do blindly
- `craigstreamy` should burn subtitles into video by default

## What Still Needs More Depth

The main remaining subtitle-policy work is now deeper behavior, not more naming:

1. define how far we want to go on bitmap subtitle OCR or authoring
2. tighten default/forced/SDH/commentary semantics as more subtitle fixtures land in e2e
3. decide whether future non-device packs should also expose `all_sub_preserve + subtitle_convert`

Recommendation:

- keep `smart_eng_sub + preserve` as the stable default
- keep `subtitle_convert` explicit and conservative
- only widen convert-oriented pack names when the underlying conversion behavior is trustworthy enough to ship
