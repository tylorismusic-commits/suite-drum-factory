#!/usr/bin/env bash
#
# Builds the Linux release archive for Suite Drum Factory.
#
# Usage:
#   tools/package_linux.sh [version]
#
# The build must already exist (see README). The version defaults to the
# nearest git tag (without the leading "v"), or 0.1.0 when no tag exists.
#
# The release archive contains the plugin only. Factory samples are distributed
# separately as drag-in .suitefactory packs.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-}"
if [ -z "${VERSION}" ]; then
    VERSION="$(git -C "${ROOT}" describe --tags --abbrev=0 2>/dev/null || true)"
    VERSION="${VERSION#v}"
fi
VERSION="${VERSION:-0.1.0}"

NAME="suite-drum-factory-${VERSION}-linux-x86_64"
BUILD="${ROOT}/build"
ARTEFACTS="${BUILD}/GlorpDrums_artefacts/Release"
STAGE="${BUILD}/stage/${NAME}"
DIST="${BUILD}/dist"
PLUGIN="Suite Drum Factory"

VST3="${ARTEFACTS}/VST3/${PLUGIN}.vst3"
LV2="${ARTEFACTS}/LV2/${PLUGIN}.lv2"

have_vst3=0
have_lv2=0
if [ -d "${VST3}" ]; then
    have_vst3=1
fi
if [ -d "${LV2}" ]; then
    have_lv2=1
fi

if [ "${have_vst3}" -eq 0 ] && [ "${have_lv2}" -eq 0 ]; then
    echo "error: no plugin bundles found under ${ARTEFACTS}" >&2
    echo "Build first with: cmake --build build -j\"\$(nproc)\"" >&2
    exit 1
fi

rm -rf "${STAGE}"
mkdir -p "${STAGE}/docs"

if [ "${have_vst3}" -eq 1 ]; then
    mkdir -p "${STAGE}/vst3"
    cp -r "${VST3}" "${STAGE}/vst3/"
fi

if [ "${have_lv2}" -eq 1 ]; then
    mkdir -p "${STAGE}/lv2"
    cp -r "${LV2}" "${STAGE}/lv2/"
fi

cp "${ROOT}/README.md" "${ROOT}/LICENSE" "${ROOT}/NOTICE.md" "${STAGE}/"
if [ -f "${ROOT}/CHANGELOG.md" ]; then
    cp "${ROOT}/CHANGELOG.md" "${STAGE}/"
fi
cp "${ROOT}/docs/EXPANSIONS.md" "${STAGE}/docs/"

cat > "${STAGE}/INSTALL.txt" <<EOF
SUITE DRUM FACTORY ${VERSION} - LINUX x86_64

This archive contains the plugin only. The factory samples are distributed
separately as drag-in .suitefactory packs.

1. Run ./install.sh to install the VST3 and/or LV2 plugin into your user
   plugin folders, or copy the bundles manually:
     vst3/Suite Drum Factory.vst3 -> ~/.vst3/
     lv2/Suite Drum Factory.lv2  -> ~/.lv2/
2. Rescan plugins in your DAW.
3. Drag a .suitefactory sample pack onto the plugin window. The pack is
   unpacked into ~/.config/SuiteDrumFactory/kits.

The plugin starts without samples. Naked Drums is the base kit; additions and
cymbal expansions are installed with the same drag-in packs.

The full factory pack is about 15 GB. Installing it can take several minutes;
keep the DAW open until the confirmation dialog appears.

Sample attribution and licence links are in NOTICE.md and inside every pack's
README-FACTORY-PACK.txt. The plugin code is AGPL-3.0.
EOF

cat > "${STAGE}/install.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN="Suite Drum Factory"

if [ -d "$HERE/vst3/$PLUGIN.vst3" ]; then
    mkdir -p "$HOME/.vst3"
    rm -rf "$HOME/.vst3/$PLUGIN.vst3"
    cp -r "$HERE/vst3/$PLUGIN.vst3" "$HOME/.vst3/"
    echo "Installed $HOME/.vst3/$PLUGIN.vst3"
fi

if [ -d "$HERE/lv2/$PLUGIN.lv2" ]; then
    mkdir -p "$HOME/.lv2"
    rm -rf "$HOME/.lv2/$PLUGIN.lv2"
    cp -r "$HERE/lv2/$PLUGIN.lv2" "$HOME/.lv2/"
    echo "Installed $HOME/.lv2/$PLUGIN.lv2"
fi

echo "Rescan plugins in your DAW."
echo "Sample packs: drag a .suitefactory file onto the plugin window."
EOF
chmod +x "${STAGE}/install.sh"

mkdir -p "${DIST}"
rm -f "${DIST}/${NAME}.zip" "${DIST}/${NAME}.zip.sha256"
(
    cd "$(dirname "${STAGE}")"
    zip -r -q "${DIST}/${NAME}.zip" "$(basename "${STAGE}")"
)
(
    cd "${DIST}"
    sha256sum "${NAME}.zip" > "${NAME}.zip.sha256"
    echo "${DIST}/${NAME}.zip"
    echo "${DIST}/${NAME}.zip.sha256"
)
