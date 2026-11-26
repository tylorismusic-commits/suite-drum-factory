# Changelog

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
