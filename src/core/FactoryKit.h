/*
    FactoryKit.h - per-piece shell sourcing for Suite Drum Factory.

    Naked Drums is the fixed base for cymbals, hi-hat, ride and any pieces the
    user has not reassigned. The kick, snare and each tom can come from Naked,
    Crocell, DRS, Salamander or Monster independently, so a kit can be a
    Monster kick, a Crocell snare and DRS toms.

    Modules contribute shell pieces only. Their cymbals are never loaded, so a
    kit cannot end up with two rides or three crashes competing for the same
    notes.
*/

#pragma once

#include "DrumKit.h"

#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace glorp
{
namespace factory
{

enum class PieceClass
{
    kick,
    snare,
    tom,
    hihat,
    ride,
    crash,
    splash,
    china,
    aux
};

constexpr int tomSlots = 5;

/// Classifies a logical piece by its name and id. Unknown pieces stay "aux".
PieceClass classifyPiece(const std::string &name, const std::string &id);

bool isShellClass(PieceClass cls) noexcept;

/// One piece in a module catalog, read without loading its samples.
struct CatalogPiece
{
    std::string id;
    std::string name;
    PieceClass pieceClass = PieceClass::aux;
    int note = -1;
};

struct Catalog
{
    std::string moduleId;
    std::string moduleName;
    std::string title;
    std::vector<CatalogPiece> pieces;
    bool valid = false;
};

/// Reads the piece list from a .glorpkit without mapping any audio.
bool readCatalog(const std::string &moduleId, const std::string &path, Catalog &catalog);

/// Toms in playing order, highest pitch first.
std::vector<CatalogPiece> tomsOf(const Catalog &catalog);

/// A candidate piece before enable/disable filtering. `moduleId` is the stable
/// source id ("naked", "crocell", ...) and `id` is the id inside that source kit.
struct PieceEntry
{
    std::string moduleId;
    std::string moduleName;
    std::string id;
    std::string name;
    PieceClass pieceClass = PieceClass::aux;
};

struct Selection
{
    /// Per-slot source module. Empty means the slot is disabled.
    std::string kickModule = "naked";
    std::string snareModule = "naked";
    std::array<std::string, tomSlots> tomModules {
        "naked", "naked", "naked", "naked", "naked"
    };

    /// Keys are "module:id", used for cymbals and any piece disabled from the
    /// piece list.
    std::set<std::string> disabled;

    std::array<std::string, tomSlots> &toms() noexcept { return tomModules; }
    const std::array<std::string, tomSlots> &toms() const noexcept { return tomModules; }

    /// Every module id this selection references. Empty slots are skipped.
    std::vector<std::string> usedModules() const;
};

struct ModuleInfo
{
    std::string id;
    std::string name;
    std::string kitFile;
    std::string folder;
};

struct BuildResult
{
    DrumKit kit;
    std::vector<PieceEntry> pieces; ///< pieces included by the current selection
    std::string error;
};

/// Built-in shell modules. Naked is the base and is not in this list.
const std::vector<ModuleInfo> &modules();

std::string moduleName(const std::string &moduleId);

/// Absolute path of an installed dependency kit, or empty when not found.
std::string findKitPath(const std::string &kitFile, const std::string &folder);
std::string baseKitPath();
std::string moduleKitPath(const std::string &moduleId);

/// Stable key used by Selection::disabled.
std::string pieceKey(const std::string &moduleId, const std::string &pieceId);

/// Assembles the base kit plus the selected per-piece sources. `kits` maps a
/// module id to a loaded kit; "naked" must be present. Module mics are
/// namespaced so their channel strips stay separate from Naked's.
BuildResult build(const std::vector<std::pair<std::string, const DrumKit *>> &kits,
                  const Selection &selection);

/// Loads the base and every referenced module, then assembles.
BuildResult buildFromDisk(const Selection &selection);

} // namespace factory
} // namespace glorp
