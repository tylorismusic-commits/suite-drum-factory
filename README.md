# Suite Drum Factory

**Suite Drum Factory** is a Linux drum instrument built on the Suite Drums Glorp
engine. It keeps one sample set loaded as the base — Naked Drums — and lets you
swap the shell pieces (kick, snare, toms) from Crocell, DRS, Salamander or
Monster without losing the Naked cymbals, hi-hat or ride.

This repository is the prototype fork. It is a separate product from Suite
Drums Glorp.

## What works in this prototype

- Naked Drums is loaded as the fixed base.
- One shell module can be selected at a time: Naked, Crocell, DRS, Salamander or
  Monster. Module cymbals are ignored; only kick, snare and toms are used.
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
- Insert pickers open at the clicked strip row; the editor updates EQ curves,
  compressor gain reduction and clipper transfer curves live.
- Insert racks and bus levels are saved in the plugin state, so reopening a
  project restores the mixer.
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
- The existing kit view, MIDI browser, channel insert FX and master strip are
  inherited from Suite Drums Glorp.
- Kit artwork is unchanged; the existing Naked art is reused for the prototype.

## Kit locations

The factory looks for:

```
~/.config/SuiteDrumsGlorp/kits/Naked Drums.glorpkit
~/.config/SuiteDrumsGlorp/kits/CrocellKit.glorpkit
~/.config/SuiteDrumsGlorp/kits/DRSKit.glorpkit
~/.config/SuiteDrumsGlorp/kits/Salamander Drumkit.glorpkit
~/.config/SuiteDrumsGlorp/kits/Monster Drumkit.glorpkit
```

The released share-folder layout is also searched:

```
~/Documents/SuiteDrumsGlorpShare/<kit>/<kit>.glorpkit
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
tools/install_plugins.sh
```

The plugin installs as **Suite Drum Factory** in `~/.vst3` and `~/.lv2`.

## Architecture

- `src/core/FactoryKit.*` — module catalog, piece role classification and
  in-memory assembly of Naked + one shell module.
- `src/plugin/PluginProcessor.*` — factory selection, persistence and kit reload.
- `src/ui/Editor.*` — the PIECES dropdown on the kit image.
- `src/core/Sampler.*` — inherited sampler, per-mic mixer and insert DSP.

The next stage is the fixed Kick / Snare / Room / Mix bus mixer with serial
insert chains on each bus, fed from these piece strips.

## Licensing

Naked Drums, CrocellKit and DRSKit are CC BY 4.0. Salamander Drumkit and Monster
Drumkit are CC BY-SA 3.0. The sample derivatives keep their original licences;
see `NOTICE.md` and the licence files shipped with each kit.

## Upstream

The DSP, sampler, kit format and mixer are inherited from
[Suite Drums Glorp](https://github.com/tylorismusic-commits/suite-drums-glorp),
which is itself a fork of GlorpDrums.
