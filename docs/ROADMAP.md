# Roadmap

## Prototype (this fork)

- [x] Naked Drums as the fixed base.
- [x] One shell module at a time (Crocell, DRS, Salamander, Monster).
- [x] Module shell pieces only; module cymbals ignored.
- [x] Namespaced module mics.
- [x] PIECES dropdown with per-piece enable/disable.
- [x] Selection persisted between sessions.
- [x] Per-piece shell sourcing: kick, snare and each tom pick their module.
- [x] Per-piece source dropdowns over the kick, snare and tom images.
- [x] Mixer exposes one strip per active kit piece, not one per microphone.
- [x] Source-mic polarity is applied automatically during piece summing.
- [x] Kit dropdown removed; configurations replace kit switching.
- [x] Stable role-based MIDI mapping across piece swaps.
- [x] Factory and user kit configurations.
- [x] Per-category automatic level matching (kicks to kicks, snares to snares, toms by slot).
- [x] Piece swaps keep MIDI loops running without a stop/restart.
- [x] CONFIG and PIECES moved into the old kit-dropdown position.
- [x] Grooves folder persisted in a dedicated file; file pickers float over the DAW.
- [x] Fixed Kick / Snare / Room / Mix bus mixer with scrollable bus strips.
- [ ] Serial insert slots (graphical EQ, compressor, drive, transient, limiter, clipper).
- [ ] Configuration-aware mix-ready presets.
- [ ] Mix presets tied to the active configuration instead of generic presets.

## Mixer

The factory mixer has exactly four buses:

```
kick close mics      -> Kick sum  --\
snare close mics     -> Snare sum ----> Mix bus -> stereo out
room/OH/ambient mics -> Room sum  --/
toms, cymbals, hi-hat, one-shots ----------> (direct to Mix bus)
```

- **Kick sum** — the active kick piece's close mics.
- **Snare sum** — the active snare piece's close mics.
- **Room sum** — room, overhead and ambient mics from the loaded modules.
- **Mix bus** — toms, cymbals, hi-hat and user one-shots, plus the three sums.

Each sum and the mix bus gets a serial insert chain: graphical EQ, compressor,
drive, transient shaper, limiter, clipper. Slots are bypassable and reorderable.
No arbitrary bus graph or free sends.

## Presets

A versioned `.factorypreset` stores module selection, enabled pieces, user
one-shots, MIDI map, bus insert chains, and channel gain/pan/mute/solo. Module
IDs stay stable across versions.

## Asset list

37 piece sprites are needed later (Naked 16, Crocell 8, DRS 5, Salamander 4,
Monster 4), plus dropdown, module badge and insert icons. The prototype reuses
the existing kit art.

## Licensing

Naked, Crocell and DRS are CC BY 4.0; Salamander and Monster are CC BY-SA 3.0.
Samples may ship with attribution, but BY-SA sample derivatives stay BY-SA.
Code can remain separately licensed; the installer and About must include the
full notices.
