#!/usr/bin/env bash
# User-local install from a source build.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build/GlorpDrums_artefacts/Release"
PLUGIN="Suite Drum Factory"

echo "=== Installing ${PLUGIN} ==="

mkdir -p "$HOME/.vst3" "$HOME/.lv2"

if [ -d "${BUILD_DIR}/VST3/${PLUGIN}.vst3" ]; then
    rm -rf "$HOME/.vst3/${PLUGIN}.vst3"
    cp -r "${BUILD_DIR}/VST3/${PLUGIN}.vst3" "$HOME/.vst3/"
    echo "  [OK] VST3: $HOME/.vst3/${PLUGIN}.vst3"
else
    echo "  [WARN] VST3 bundle not found; build with -DGLORP_BUILD_VST3=ON"
fi

if [ -d "${BUILD_DIR}/LV2/${PLUGIN}.lv2" ]; then
    rm -rf "$HOME/.lv2/${PLUGIN}.lv2"
    cp -r "${BUILD_DIR}/LV2/${PLUGIN}.lv2" "$HOME/.lv2/"
    echo "  [OK] LV2: $HOME/.lv2/${PLUGIN}.lv2"
else
    echo "  [WARN] LV2 bundle not found; build with -DGLORP_BUILD_LV2=ON"
fi

echo
echo "=== ${PLUGIN} installed. Rescan plugins in your DAW. ==="
echo "Samples are not bundled. Drag a .suitefactory pack onto the plugin window."
echo "See NOTICE.md for sample licences and docs/EXPANSIONS.md for pack authoring."
