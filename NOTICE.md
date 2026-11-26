# Notices and Attributions

Suite Drums Glorp is a fork of **GlorpDrums** by asuka-glorp:

- Upstream: https://github.com/asuka-glorp/GlorpDrums
- Upstream license: GNU Affero General Public License v3.0
- Fork: https://github.com/tylorismusic-commits/suite-drums-glorp

The drum engine, kit format, UI foundations and kit editor are derived from
GlorpDrums and remain under AGPL-3.0. Suite Drums Glorp removes the practice,
recording, streaming and standalone-host features, adds the Suite theme and
rebrands the product.

## Insert EQ topology

The eight-band insert EQ adapts the band layout and filter topologies from
**Common Sense EQ**:

- Upstream: https://github.com/tylorismusic-commits/common-sense-eq
- License: GNU General Public License v3.0 or later
- Used for: low-cut, low-shelf, four bell, high-shelf and high-cut bands with
  selectable cut slopes.

Combined with this AGPL-3.0 project under the GPL/AGPL compatibility terms.

## Fonts

The following fonts are bundled for the UI and are distributed under the SIL
Open Font License 1.1. The full license texts are in `resources/fonts/`.

| Font | File | License text |
| :--- | :--- | :--- |
| Jost ExtraBold | `resources/fonts/Jost-ExtraBold.ttf` | `resources/fonts/Jost-OFL.txt` |
| Red Hat Text Medium | `resources/fonts/RedHatText-Medium.otf` | `resources/fonts/RedHatText-OFL.txt` |

If a license text file is not present for a bundled font, the font's upstream
OFL notice applies. Jost is Copyright 2020 The Jost Project Authors
(https://github.com/indestructible-type).

## Drum kit samples

The factory sample pack contains converted copies of the kits below. Each
`.glorpkit` keeps the licence stated in its own `kit.xml`, and the generated
`README-FACTORY-PACK.txt` inside the pack repeats this attribution. The plugin
code is AGPL-3.0; these sample libraries are separate works under their own
Creative Commons licences.

Licence audit: checked on 2026-11-26 against the upstream project pages, the
source archives, and the local `ATTRIBUTION.md`/licence files for each kit.

| Kit | Credit | Source | Licence |
| :--- | :--- | :--- | :--- |
| Naked Drums | Wilkinson Audio; SFZ conversion by kinwie | https://github.com/sfzinstruments/WilkinsonAudio.NakedDrums | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| CrocellKit | DrumGizmo Team / Crocell | https://drumgizmo.org/wiki/doku.php?id=kits:crocellkit | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| DRSKit | DRSDrums / Jes Eiler and the DrumGizmo Team | https://drumgizmo.org/wiki/doku.php?id=kits:drskit | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Salamander Drumkit | Alexander Holm; SFZ reconstruction by kinwie | https://archive.org/details/SalamanderDrumkit | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) |
| Monster Drumkit | Processed Salamander samples by Alexander Holm; processing author not stated in the source archive | https://archive.org/details/SalamanderDrumkit | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) |

### Changes made for Suite Drum Factory

- Naked Drums: merged the 16 original mic channels, transcoded FLAC to
  24-bit WAV and repacked the samples as `.glorpkit`.
- CrocellKit: converted from the 15-mic DrumGizmo kit to `.glorpkit`; the
  default articulation set and 48 kHz sample rate are preserved.
- DRSKit: converted the 13-mic version 2.1 kit to `.glorpkit`; logical pieces
  were grouped and the usual kick-back polarity correction is applied in the
  mixer rather than to the samples.
- Salamander Drumkit: downmixed the stereo overhead samples to mono 24-bit WAV
  and repacked them as `.glorpkit`.
- Monster Drumkit: a processed Salamander derivative; stereo overhead samples
  were downmixed to mono 24-bit WAV and repacked as `.glorpkit`. The processing
  author is not identified in the downloaded archive, so the attribution keeps
  Alexander Holm and the source archive.

The Salamander Drumkit author dedicated the original library to the public
domain in 2022. This project still distributes the Salamander and Monster
derivatives under CC BY-SA 3.0 to preserve the original attribution and
share-alike terms.

Both CC BY 4.0 and CC BY-SA 3.0 allow commercial redistribution. Share-alike
applies to modified sample libraries, not to music made with the samples or to
the AGPL plugin code. Anyone redistributing these packs must keep the
attribution, licence links and change notes with the samples.

The optional **Glorp Demo Kit** shipped with releases is a compact subset of the
GlorpDrums demo kit:

- Title: Glorp Demo Kit
- Author / credit: Alexander Holm / GlorpDrums
- Metadata license: CC-BY-SA / CC0

Sample files retain the license stated in the kit's `drumkit.xml`. When
redistributing the kit, keep the metadata and this attribution with the samples.
If you need an unambiguously CC0-only starter kit, replace the kit folder with
your own CC0 samples; the plugin does not require any bundled kit.

## Trademarks and branding

"Suite Drums Glorp" and the Suite wordmark identify this fork. The AGPL covers
the source code, not trademarks or branding. GlorpDrums names, logos and
artwork remain the property of their original authors and are not relicensed by
this project.
