# Changelog

## 0.1.6 - 2026-11-28

- Build the VST3 and LV2 with a real MIDI output bus (`NEEDS_MIDI_OUTPUT`).
  The plugin always wrote remapped MIDI from `processBlock`, but the format
  wrappers never advertised a MIDI output port, so DAWs such as REAPER had
  nothing to route to the next instrument in the chain.
- This makes MIDI Thru work end-to-end: host MIDI, internal loop-browser
  previews and note-offs now reach downstream instruments on the same track.

## 0.1.5 - 2026-11-28

- **Close Kit (MIDI Thru)** now enables MIDI Thru as part of the same action.
  Previously it only unloaded the kit, so if MIDI Thru had not been toggled on
  first, the virtual destination map was never built and the editor/loop
  browser still behaved as if no MIDI Thru was configured.
- Added a regression probe for the close-kit path.

## 0.1.4 - 2026-11-28

- Preserve the loop's MIDI channel when previewing through MIDI Thru, so a
  downstream library listening on channel 10 receives GM drum loops on the
  channel it expects.
- Send all-notes-off on all 16 channels when pausing, stopping or restarting a
  MIDI Thru preview.
- Hide the velocity-curve editor on virtual MIDI Thru rows, where there is no
  sample instrument to shape.

## 0.1.3 - 2026-11-28

- Fix MIDI Thru as a no-kit mapper. With the plugin before a downstream drum
  library (for example Ugritone KVLT) and no samples loaded, the MIDI editor
  keeps showing the destination note names and map instead of going blank.
- Loop-browser previews now play through the plugin's MIDI output in MIDI Thru
  no-kit mode, including note-offs and all-notes-off on pause, stop and loop
  restart, so a source profile can audition straight into the next instrument.
- Changing the loop browser's source profile now rebuilds the MIDI Thru map,
  and loading a kit clears the virtual destination map.
- Fix "No source-map notes matched the current kit mapping" when a valid
  source/destination map exists but no sampler kit is loaded.

## 0.1.2 - 2026-11-27

- Fix insert racks on externally loaded `.glorpkit`/folder kits and one-shot
  layer strips. Their strip racks were never prepared after the host had
  already called `prepareToPlay()`, so inserts appeared in the mixer but
  processed no audio. Inserts now work on external kits.

## 0.1.1 - 2026-11-26

- Fix the optional Suite Kit Studio / converter link on source builds by
  linking `InsertEQ.cpp` into those targets.
- No plugin behaviour change from 0.1.0.

## 0.1.0 - 2026-11-26

First public release of Suite Drum Factory.

### Highlights

- Sample-free plugin build: the plugin starts cleanly without any bundled kits.
- Drag-in `.suitefactory` sample packs install kits and manifests into
  `~/.config/SuiteDrumFactory/`, refresh the kit list and load a supported
  factory configuration. ZIP and tar packs are supported; the pack builder
  switches to tar automatically above the 4 GiB ZIP32 limit.
- Dynamic expansion modules built from `pack.txt` manifests.
- Whole-cymbal-group swapping through **PIECES → Cymbals Source**, with safe
  per-role fallback to Naked Drums for incomplete packs.
- Light and dark themes, with separate persisted highlight colours for each.
  Dark is the default.
- Project state now saves and restores the factory shell/cymbal selection and
  disabled pieces.
- Missing expansion modules fall back to Naked Drums instead of leaving the kit
  unloadable.
- Pack README/licence files are preserved under
  `~/.config/SuiteDrumFactory/pack_docs/`.
- Factory pack builder, pack authoring documentation and licence audit.
- Linux release packaging and GitHub release workflow for the plugin archive.

### Sample packs

Factory samples are separate works under CC BY 4.0 or CC BY-SA 3.0. See
`NOTICE.md` for sources, licence links and conversion notes. The full factory
pack is distributed separately from the sample-free plugin archive.
