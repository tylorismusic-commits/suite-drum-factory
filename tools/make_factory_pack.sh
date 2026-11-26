#!/usr/bin/env bash
#
# Builds a drag-and-drop factory sample pack for Suite Drum Factory.
#
# Usage:
#   tools/make_factory_pack.sh [output.suitefactory]
#
# By default all five factory kits are required and included. Set PACK_KITS to
# a comma-separated list to build a smaller pack, for example:
#   PACK_KITS="Naked Drums.glorpkit" tools/make_factory_pack.sh /tmp/naked.suitefactory
#
# To build an expansion pack, point PACK_MANIFEST at a pack.txt:
#   PACK_KITS="/path/to/MyCymbals.glorpkit" \
#   PACK_MANIFEST=pack.txt \
#   tools/make_factory_pack.sh MyCymbals.suitefactory
#
# The pack contains the original .glorpkit archives plus a licence/attribution
# readme. Set PACK_README=/path/to/README.txt to replace the generated notice.
# Drop the resulting file onto the plugin window to install the kits.
# See docs/EXPANSIONS.md for the manifest format.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${1:-${ROOT}/dist/SuiteDrumFactory-Factory.suitefactory}"
mkdir -p "$(dirname "${OUT}")"
OUT="$(realpath -m "$OUT")"

declare -A FOLDERS=(
    ["Naked Drums.glorpkit"]="Naked Drums"
    ["CrocellKit.glorpkit"]="CrocellKit"
    ["DRSKit.glorpkit"]="DRSKit"
    ["Salamander Drumkit.glorpkit"]="Salamander Drumkit"
    ["Monster Drumkit.glorpkit"]="Monster Drumkit"
)

DEFAULT_KITS=(
    "Naked Drums.glorpkit"
    "CrocellKit.glorpkit"
    "DRSKit.glorpkit"
    "Salamander Drumkit.glorpkit"
    "Monster Drumkit.glorpkit"
)

IFS=',' read -r -a REQUESTED <<< "${PACK_KITS:-$(IFS=,; echo "${DEFAULT_KITS[*]}")}"

find_kit() {
    local name="$1"
    local folder="${FOLDERS[$name]:-}"
    local candidates=(
        "${KITS_DIR:-}/$name"
        "$HOME/.config/SuiteDrumFactory/kits/$name"
        "$HOME/.config/SuiteDrumsGlorp/kits/$name"
        "$HOME/Documents/SuiteDrumsGlorpShare/${folder}/$name"
    )

    if [ -f "$name" ]; then
        realpath "$name"
        return 0
    fi

    for candidate in "${candidates[@]}"; do
        if [ -n "$candidate" ] && [ -f "$candidate" ]; then
            realpath "$candidate"
            return 0
        fi
    done

    for candidate in "$HOME/Documents/SuiteDrumsGlorpShare"/*/"$name"; do
        if [ -f "$candidate" ]; then
            realpath "$candidate"
            return 0
        fi
    done
    return 1
}

MANIFEST="${PACK_MANIFEST:-}"
if [ -n "$MANIFEST" ] && [ ! -f "$MANIFEST" ]; then
    echo "error: PACK_MANIFEST not found: $MANIFEST" >&2
    exit 1
fi

if [ -n "${PACK_README:-}" ] && [ ! -f "$PACK_README" ]; then
    echo "error: PACK_README not found: $PACK_README" >&2
    exit 1
fi

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

FILES=()
for raw in "${REQUESTED[@]}"; do
    name="$(echo "$raw" | xargs)"
    [ -z "$name" ] && continue
    if ! path="$(find_kit "$name")"; then
        echo "error: missing factory kit: $name" >&2
        echo "Install it under ~/.config/SuiteDrumFactory/kits or set KITS_DIR." >&2
        exit 1
    fi
    FILES+=("$path")
done

if [ "${#FILES[@]}" -eq 0 ]; then
    echo "error: no kits selected" >&2
    exit 1
fi

README_OUT="$STAGE/README-FACTORY-PACK.txt"
if [ -n "${PACK_README:-}" ]; then
    cp "$PACK_README" "$README_OUT"
else
    cat > "$README_OUT" <<'EOF'
SUITE DRUM FACTORY - FACTORY SAMPLE PACK

Drop this .suitefactory file onto the Suite Drum Factory plugin window.
The plugin will unpack the .glorpkit archives into its kits folder and load
the factory configuration that the installed modules support.

Sample attribution:
EOF

    for kit in "${FILES[@]}"; do
        case "$(basename "$kit")" in
            "Naked Drums.glorpkit")
                cat >> "$README_OUT" <<'EOF'
- Naked Drums - Wilkinson Audio; SFZ conversion by kinwie
  Source: https://github.com/sfzinstruments/WilkinsonAudio.NakedDrums
  Licence: CC BY 4.0 - https://creativecommons.org/licenses/by/4.0/
  Changes: 16 mic channels merged; FLAC transcoded to 24-bit WAV; repacked as .glorpkit.
EOF
                ;;
            "CrocellKit.glorpkit")
                cat >> "$README_OUT" <<'EOF'
- CrocellKit - DrumGizmo Team / Crocell
  Source: https://drumgizmo.org/wiki/doku.php?id=kits:crocellkit
  Licence: CC BY 4.0 - https://creativecommons.org/licenses/by/4.0/
  Changes: converted the 15-mic kit to .glorpkit; default articulations and 48 kHz rate preserved.
EOF
                ;;
            "DRSKit.glorpkit")
                cat >> "$README_OUT" <<'EOF'
- DRSKit - DRSDrums / Jes Eiler and the DrumGizmo Team
  Source: https://drumgizmo.org/wiki/doku.php?id=kits:drskit
  Licence: CC BY 4.0 - https://creativecommons.org/licenses/by/4.0/
  Changes: converted the 13-mic version 2.1 kit to .glorpkit; logical pieces grouped.
EOF
                ;;
            "Salamander Drumkit.glorpkit")
                cat >> "$README_OUT" <<'EOF'
- Salamander Drumkit - Alexander Holm; SFZ reconstruction by kinwie
  Source: https://archive.org/details/SalamanderDrumkit
  Licence: CC BY-SA 3.0 - https://creativecommons.org/licenses/by-sa/3.0/
  Changes: stereo overhead samples downmixed to mono 24-bit WAV; repacked as .glorpkit.
EOF
                ;;
            "Monster Drumkit.glorpkit")
                cat >> "$README_OUT" <<'EOF'
- Monster Drumkit - processed Salamander samples by Alexander Holm
  Source: https://archive.org/details/SalamanderDrumkit
  Licence: CC BY-SA 3.0 - https://creativecommons.org/licenses/by-sa/3.0/
  Changes: processed Salamander derivative; stereo overhead samples downmixed to
  mono 24-bit WAV and repacked as .glorpkit. The processing author is not stated
  in the source archive.
EOF
                ;;
            *)
                cat >> "$README_OUT" <<EOF
- $(basename "$kit") - author, source and licence not supplied by this builder.
  Add the full attribution, licence link and change notes before distributing.
EOF
                ;;
        esac
    done

    cat >> "$README_OUT" <<'EOF'

Expansion packs add a pack.txt manifest. It tells the plugin which module id,
display name and kit file the pack provides. Set cymbals=1 when the kit can
replace the whole cymbal group, and shells=0 for a cymbal-only pack. The plugin
then lists the pack under PIECES. See docs/EXPANSIONS.md for the full format.

Each .glorpkit archive keeps its own metadata and licence notice. These
sample packs are separate from the AGPL-3.0 plugin code.
EOF
fi

cat > "$STAGE/FACTORY-PACK-VERSION.txt" <<EOF
format=1
created=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

# Stage only the small generated files. The kit archives are read from their
# installed locations so a 14 GB factory pack does not need a second 14 GB
# temporary copy under /tmp.
EXTRA=("$README_OUT" "$STAGE/FACTORY-PACK-VERSION.txt")
if [ -n "$MANIFEST" ]; then
    cp "$MANIFEST" "$STAGE/pack.txt"
    EXTRA+=("$STAGE/pack.txt")
fi

PACK_FORMAT="${PACK_FORMAT:-auto}"
if [ "$PACK_FORMAT" = "auto" ]; then
    total_bytes=0
    for kit in "${FILES[@]}"; do
        total_bytes=$((total_bytes + $(stat -c %s "$kit")))
    done

    # ZIP32 cannot address archives or entries over 4 GiB, and JUCE's ZIP
    # reader does not implement Zip64. Use a streaming tar for the full
    # factory pack and keep ZIP for compact expansions.
    if [ "$total_bytes" -ge $((4 * 1024 * 1024 * 1024)) ]; then
        PACK_FORMAT=tar
    else
        PACK_FORMAT=zip
    fi
fi

rm -f "$OUT"
if [ "$PACK_FORMAT" = "tar" ]; then
    TAR_ARGS=()
    for kit in "${FILES[@]}"; do
        TAR_ARGS+=(-C "$(dirname "$kit")" "$(basename "$kit")")
    done
    TAR_ARGS+=(-C "$STAGE" "README-FACTORY-PACK.txt" "FACTORY-PACK-VERSION.txt")
    if [ -n "$MANIFEST" ]; then
        TAR_ARGS+=(-C "$STAGE" "pack.txt")
    fi
    tar --format=gnu -cf "$OUT" "${TAR_ARGS[@]}"
else
    zip -0 -q -j "$OUT" "${FILES[@]}" "${EXTRA[@]}"
fi

echo "$OUT"
echo "kits: ${#FILES[@]}"
echo "format: $PACK_FORMAT"
if [ -n "$MANIFEST" ]; then
    echo "manifest: $MANIFEST"
fi
