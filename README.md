# Suite Drum Factory

**Suite Drum Factory** is a Linux drum instrument built on the Suite Drums Glorp
engine. It keeps one sample set loaded as the base — Naked Drums — and lets you
swap the shell pieces (kick, snare, toms) from Crocell, DRS, Salamander or
Monster. Cymbals default to Naked and can be replaced as one group by an
installed expansion pack.

This repository is the prototype fork. It is a separate product from Suite
Drums Glorp.

## What works in this prototype

- Naked Drums is loaded as the fixed base.
- One shell module can be selected at a time: Naked, Crocell, DRS, Salamander or
  Monster. Expansion modules that provide shells also appear in those selectors.
- The cymbal group (China, Crash, HiHat, Ride, Splash) can be switched as one
  unit from **PIECES → Cymbals Source**. Naked is the default; a pack whose
  manifest sets `cymbals=1` appears in the same menu.
- Module mics are namespaced (`crocell_SnareTop`, etc.) so their mixer strips do
  not collapse into Naked's mics.
- A source dropdown sits on the kick, snare and every tom image, so a kit can be
  a Monster kick, a Crocell snare and DRS toms. A separate **PIECES** menu
  enables or disables individual pieces.
- **CONFIG** loads a factory configuration or saves the current one. Built-ins
  include Naked, Crocell, DRS, Salamander, Monster, Studio Mix and Vintage Mix;
  user configurations are stored under `~/.config/SuiteDrumFactory/factory_configs`.
- MIDI notes are tied to stable roles (`kick.main`, `snare.main`, `tom.1`, ...),
  so swapping a Crocell snare for a DRS snare keeps note 38 in the same place.
  Loops keep running through a swap; the MIDI browser resolves each note
  against the current map as it plays instead of caching old instrument indices.
- Module kicks are level-matched to the Naked kick, snares to the Naked snare
  and toms to their Naked slot. Kicks stay equal to kicks and snares to snares,
  rather than flattening the whole kit to one loudness.
- The mixer exposes **one strip per active kit piece**, not one strip per
  microphone. A Naked-only kit shows 16 strips; the Monster-kick/Crocell-snare/
  DRS-tom example shows 14.
- Kick, snare, toms, cymbals and room sums sit in the same scrolling strip
  browser as the piece channels, with their own fader, pan, mute, solo and
  meters. They feed the Mix bus, which remains the fixed master strip.
- Every piece and sum has four insert rows directly in the strip. Click an
  empty row to add an EQ, compressor, drive, transient shaper, limiter or
  clipper; click a loaded row to edit it. The left dot bypasses and the `x`
  removes it.
- The EQ is an eight-band console-style curve: low cut, low shelf, four bells,
  high shelf and high cut. Click or drag the coloured nodes, pick a band from
  the chip row, toggle it, cycle the cut slope, and edit frequency, gain, Q and
  output gain with the knobs.
- Each piece strip has a **BLEED** knob that blends that piece’s room/overhead
  content into the channel instead of the shared Room sum.
- The bottom of each piece strip routes either into the internal sums or direct
  to a host output pair. Direct out keeps the strip inserts and carries the
  piece's room content, but bypasses the sums, Mix bus, master inserts and the
  shared reverb return.
- **OUT** in the mixer selects the host routing mode:
  **Default Stereo**, **Multi-Out Easy** (the five sums each on their own
  output), **Multi-Out Advanced** (kick group, snare group, Tom 1–5,
  cymbals/overhead and room/ambient groups) or **Full Multi-Out** (every piece
  on its own output). Multi-out modes bypass the Mix bus and shared reverb so
  the kit can be mixed in the DAW.
- Insert pickers open at the clicked strip row; the editor updates EQ curves,
  compressor gain reduction and clipper transfer curves live.
- Insert racks and bus levels are saved in the plugin state, so reopening a
  project restores the mixer.
- **MIXES** opens the preset browser. Factory mixes are rebuilt for the active
  configuration. **Balanced, Punch, Room and Tight** re-level and pan the kit;
  **Studio Glue, Punch Bus, Tape Room and Modern Tight** additionally build
  insert chains on the pieces, sums and Mix bus. They adapt to the loaded kick,
  snare and tom source modules.
- A new session opens on the **Studio Mix** configuration with the lightly
  processed **Studio Glue** preset.
- Kick and snare one-shot layers have their own blend, pitch, ADSR, delay,
  trim and polarity controls, including pitch matching against the kit hit.
- The kit view uses a full-width art layout with a compact three-zone studio
  deck: a **Piece Inspector** for source/articulation and tuning, a **Mix Strip**
  with always-visible volume, pan, bleed, output routing and an open-strip
  action, and a master **Monitor**. **Settings → Kit View Layout** switches back
  to the classic sidebar at any time.
- The **KIT** dropdown in the kit view loads any installed external kit, browses
  for a `.glorpkit`/XML file, or rescans the kit folders.
- External kits expose per-strip **Ø (phase)** and **MONO** controls so obvious
  source problems — polarity, one-sided trigger channels, odd mic layouts — can
  be corrected in the mixer. Factory kits keep those corrections baked in and
  hide the controls.
- Source-mic polarity is applied automatically while the piece is summed, so
  modules that mark a mic out of phase (for example DRS Kdrum_back) need no
  manual setup.
- The chosen grooves folder is written to a dedicated
  `~/.config/SuiteDrumFactory/midi_library_path.txt` as well as settings, so
  another plugin instance cannot forget it.
- File pickers are non-native and parented to the plugin window, so they open
  over the DAW instead of tiling behind it.
- **CONFIG** sits under the mixer button and **PIECES** under settings. About
  opens an in-app attribution card.
- **Settings → Appearance** switches between the dark console theme and a light
  paper theme, with separate highlight-colour pickers for each mode. The choice
  and both colours persist in `settings.txt`.
- The plugin can be built and shipped without samples. Dragging a
  `.suitefactory` factory pack onto the window unpacks its `.glorpkit` files,
  installs the manifest and loads a factory configuration that the installed
  kits support.
- Third-party expansions use the same drag-in container. A `pack.txt` manifest
  registers the module id, display name, kit file and cymbal capability. See
  [docs/EXPANSIONS.md](docs/EXPANSIONS.md).
- The existing kit view, MIDI browser, channel insert FX and master strip are
  inherited from Suite Drums Glorp.
- Kit artwork is unchanged; the existing Naked art is reused for the prototype.

## Kit locations

The factory looks for kits in:

```
~/.config/SuiteDrumFactory/kits/       # drag-in packs install here
~/.config/SuiteDrumsGlorp/kits/        # legacy shared location
~/Documents/SuiteDrumsGlorpShare/<kit>/<kit>.glorpkit
```

Expansion manifests live in `~/.config/SuiteDrumFactory/factory_packs/` and
are rescanned when a pack is installed or a kit list is refreshed.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
tools/install_plugins.sh
```

Build the drag-in factory pack (all five kits) or a smaller test pack:

```bash
tools/make_factory_pack.sh dist/SuiteDrumFactory-Factory.suitefactory
PACK_KITS="Naked Drums.glorpkit" \
  tools/make_factory_pack.sh dist/Naked-base.suitefactory
```

The plugin installs as **Suite Drum Factory** in `~/.vst3` and `~/.lv2`.

## Architecture

- `src/core/FactoryKit.*` — dynamic module registry, piece role
  classification, cymbal-group mapping and in-memory assembly of Naked plus
  selected shell/cymbal modules.
- `src/plugin/PluginProcessor.*` — factory selection, persistence and kit reload.
- `src/ui/Editor.*` — the PIECES dropdown on the kit image.
- `src/core/Sampler.*` — inherited sampler, per-mic mixer and insert DSP.

The mixer's fixed Kick / Snare / Room / Mix buses, serial insert chains and
factory presets are inherited from the Glorp engine.

## Documentation

- [Expansion pack format](docs/EXPANSIONS.md)
- [Kit art layout](docs/KIT_ART_LAYOUT.md)
- [Roadmap](docs/ROADMAP.md)

## Licensing

Naked Drums, CrocellKit and DRSKit are CC BY 4.0. Salamander Drumkit and Monster
Drumkit are CC BY-SA 3.0. The sample derivatives keep their original licences;
see `NOTICE.md` and the licence files shipped with each kit.

## Upstream

The DSP, sampler, kit format and mixer are inherited from
[Suite Drums Glorp](https://github.com/tylorismusic-commits/suite-drums-glorp),
which is itself a fork of GlorpDrums.
