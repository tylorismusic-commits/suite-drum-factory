# Expansion Packs

Suite Drum Factory ships as a small plugin that reads sample kits from the
user's kit folder. Factory samples and third-party expansions use the same
container: a **`.suitefactory`** file that the user drags onto the plugin
window.

This document is the format contract for expansion makers. It covers the pack
layout, the `pack.txt` manifest, cymbal-group mapping, fallbacks, licensing and
packaging.

## What an expansion can provide

In v1 an expansion is a **module**: an additional `.glorpkit` kit that the
factory can use as a source. A module can supply shell pieces, a cymbal group,
or both.

| Capability | Manifest key | Notes |
| :--- | :--- | :--- |
| Shell pieces | `shells=1` | Adds the module to the per-piece kick, snare and tom selectors. Defaults to on unless the pack sets `cymbals=1`. |
| Whole cymbal group | `cymbals=1` | Lists the module under **Cymbals Source** and replaces China/Crash/HiHat/Ride/Splash together. |
| Cymbal-only pack | `shells=0` | Keeps a cymbal-only module out of the kick/snare/tom selectors. |

The current release implements **whole-group cymbal swapping**. Per-piece
cymbal overrides may be added later; the pack format is designed to allow that
without changing existing packs.

## Pack layout

A `.suitefactory` file is a single archive containing the kit files and
manifest. The installer accepts **ZIP** and **POSIX/GNU tar**. Small expansions
are built as ZIP; the full factory pack is built as tar because ZIP32 cannot
address archives or entries over 4 GiB and the Crocell archive is larger than
that. Tar is stored uncompressed, which is also what the `.glorpkit` files
already use internally.

```text
MyCymbals.suitefactory
├── pack.txt                     # required for an expansion
├── MyCymbals.glorpkit           # one or more kit archives
├── README-FACTORY-PACK.txt      # attribution and licence notes
└── FACTORY-PACK-VERSION.txt     # optional, format=1
```

The plugin installs every `.glorpkit` in the archive into:

```text
~/.config/SuiteDrumFactory/kits/
```

It reads `pack.txt`, writes a copy of the manifest to:

```text
~/.config/SuiteDrumFactory/factory_packs/<id>.txt
```

and rescans the module registry immediately. No restart and no separate
installer are required. Any `README`, `LICENSE`, `LICENCE`, `ATTRIBUTION`,
`COPYING` or `LICENSES/` entries are copied to
`~/.config/SuiteDrumFactory/pack_docs/<pack-name>/` so the attribution stays
with the installed samples.

## `pack.txt`

The manifest is a plain UTF-8 text file with one `key=value` pair per line.
Blank lines and lines starting with `#` are ignored. Whitespace around keys and
values is trimmed, and both LF and CRLF line endings are accepted.

| Key | Required | Description |
| :--- | :--- | :--- |
| `id` | yes | Unique module id. Use lowercase letters, digits, `-` and `_`. Must not be `naked` and must not reuse a built-in id (`crocell`, `drs`, `salamander`, `monster`). |
| `name` | yes | Display name shown in the **PIECES → Cymbals Source** menu. |
| `kit` | yes | File name of the `.glorpkit` supplied by the pack, for example `MyCymbals.glorpkit`. |
| `folder` | no | Folder name searched under `~/Documents/SuiteDrumsGlorpShare/`. Defaults to the kit file name without `.glorpkit`. |
| `cymbals` | no | `1` or `true` marks the module as a whole-cymbal-group source. |
| `shells` | no | `1` or `true` exposes kick/snare/tom pieces. Defaults to `1` for a normal pack and `0` when `cymbals=1` is set. |

Minimal cymbal expansion:

```ini
# MyCymbals pack
id=mycymbals
name=My Cymbals
kit=MyCymbals.glorpkit
folder=My Cymbals
cymbals=1
shells=0
```

After installation the module appears in **PIECES → Cymbals Source** next to
**Naked Drums**. Selecting it rebuilds the kit with the expansion's cymbal
pieces and keeps the current kick, snare and toms.

## Cymbal-group mapping

The Naked kit defines eight stable cymbal roles:

| Base role | Piece id | Class |
| :--- | :--- | :--- |
| China 1 | `china_1` | china |
| China 2 | `china_2` | china |
| Crash 1 | `crash_1` | crash |
| Crash 2 | `crash_2` | crash |
| Hi-hat | `hihat` | hihat |
| Ride | `ride` | ride |
| Splash 1 | `splash_1` | splash |
| Splash 2 | `splash_2` | splash |

For each role the assembler:

1. looks for an unused piece in the expansion with the same **piece id**;
2. otherwise looks for an unused piece in the same **class**;
3. otherwise keeps the Naked piece and adds a warning.

Matching is one-to-one, so `china1` and `china2` supply `china_1` and
`china_2`, while `crash1`/`crash2` supply `crash_1`/`crash_2`. A single
`splash` piece fills `splash_1`; `splash_2` falls back to Naked.

The assembled pieces keep the Naked role ids (`china_1`, `crash_1`, ...).
Artwork, MIDI roles, mixer strips, inserts and saved configurations therefore
stay stable when the cymbal source changes. The mixer shows the expansion's
strip titles and audio, not a second set of cymbal strips.

### Complete vs. partial packs

A complete cymbal pack supplies at least one piece for every class, with two
chinas, two crashes and two splashes for a full replacement. Partial packs are
safe: missing roles fall back to Naked Drums and the kit still loads.

For a predictable support experience, test and advertise only the roles the
pack actually provides. If the pack is meant to replace the whole group, supply
two chinas, two crashes, two splashes, a hi-hat and a ride.

## Building a pack

Use the repository helper:

```bash
PACK_KITS="/path/to/MyCymbals.glorpkit" \
PACK_MANIFEST="/path/to/pack.txt" \
tools/make_factory_pack.sh MyCymbals.suitefactory
```

The builder chooses ZIP for compact packs and tar when the total exceeds
4 GiB. Set `PACK_FORMAT=zip` or `PACK_FORMAT=tar` to force one.

`PACK_KITS` accepts a comma-separated list. Each entry can be:

- a full path to a `.glorpkit`;
- a file name looked up in `~/.config/SuiteDrumFactory/kits`,
  `~/.config/SuiteDrumsGlorp/kits`, a configured folder under
  `~/Documents/SuiteDrumsGlorpShare/<folder>/`, or any immediate subfolder of
  `~/Documents/SuiteDrumsGlorpShare/`;
- a path relative to `KITS_DIR` when that variable is set.

The script writes a `README-FACTORY-PACK.txt` with attribution text. Set
`PACK_README=/path/to/README.txt` to supply your own, or edit the generated
file before distributing the pack. You can also build the ZIP yourself with
the same layout.

To build the five-kit factory pack:

```bash
tools/make_factory_pack.sh dist/SuiteDrumFactory-Factory.suitefactory
```

## Licensing and attribution

The plugin is AGPL-3.0. Sample packs are **separate works** and keep their own
licences.

- Include the original licence and attribution for every sample set in the
  pack, either inside the `.glorpkit` metadata or in
  `README-FACTORY-PACK.txt`.
- CC BY packs must credit the author and state the licence.
- CC BY-SA packs must also carry the share-alike terms to derivatives.
- Do not include samples you do not have permission to redistribute.
- Do not describe a pack as "royalty free" unless its licence actually says so.

The built-in factory pack uses:

| Kit | Author / source | Licence |
| :--- | :--- | :--- |
| Naked Drums | Wilkinson Audio; SFZ conversion by kinwie — https://github.com/sfzinstruments/WilkinsonAudio.NakedDrums | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| CrocellKit | DrumGizmo Team / Crocell — https://drumgizmo.org/wiki/doku.php?id=kits:crocellkit | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| DRSKit | DRSDrums / Jes Eiler and the DrumGizmo Team — https://drumgizmo.org/wiki/doku.php?id=kits:drskit | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Salamander Drumkit | Alexander Holm; SFZ reconstruction by kinwie — https://archive.org/details/SalamanderDrumkit | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) |
| Monster Drumkit | Processed Salamander samples by Alexander Holm; processing author not stated in the source archive | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) |

See `NOTICE.md` and each kit's own metadata for the authoritative notices,
sources and conversion notes. The Monster kit is a Salamander derivative, not a
separate DrumGizmo recording.

## Versioning and compatibility

- `FACTORY-PACK-VERSION.txt` currently contains `format=1` and a creation
  timestamp. It is informational; the plugin does not reject other values.
- Unknown keys in `pack.txt` are ignored, so packs may add author, version or
  licence metadata for their own tooling.
- Keep the `id` stable across releases. Configurations and settings store the
  id, not the display name.
- The installed manifest is copied verbatim. Reinstalling a pack with the same
  `id` replaces the manifest and can update the kit archive.

## Testing checklist

1. Start with an empty config directory and install the factory pack
   (`.suitefactory` containing Naked Drums).
2. Drag the expansion pack onto the plugin. Confirm the success message.
3. Open **PIECES → Cymbals Source**. Confirm the expansion is listed.
4. Select it. Confirm the kit loads, cymbal strips change source, and kick,
   snare and toms stay unchanged.
5. Save a configuration and reopen the project. Confirm the cymbal source is
   restored.
6. Remove the expansion kit and manifest, then reload. Confirm the plugin
   falls back to Naked cymbals instead of failing to load.
