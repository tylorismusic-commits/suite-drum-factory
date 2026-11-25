/*
    FactoryKit.h - Naked Drums base plus one optional shell module.

    Suite Drum Factory keeps Naked Drums as the fixed base for cymbals, hi-hat,
    ride and the base kit. Crocell, DRS, Salamander and Monster contribute shell
    pieces only (kick, snare, toms); their cymbals are deliberately ignored so a
    kit never ends up with two rides or three crashes fighting for the same
    notes.

    Assembly happens in memory before the sampler sees the kit. Module mics are
    renamed with a module prefix, so their channel strips stay separate from
    Naked's mics instead of silently summing into them.
*/

#pragma once

#include "DrumKit.h"

#include <set>
#include <string>
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

/// Classifies a logical piece by its name and id. Unknown pieces stay "aux" and
/// are always kept.
PieceClass classifyPiece(const std::string &name, const std::string &id);

bool isShellClass(PieceClass cls) noexcept;

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
    /// Empty means the base Naked shells. Otherwise a module id from modules().
    std::string shellModule;

    /// Keys are "module:id", e.g. "naked:crash_1" or "crocell:ftom1".
    std::set<std::string> disabled;
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
    std::vector<PieceEntry> pieces; ///< all candidates for the current module choice
    std::string error;
};

/// Built-in shell modules. The base (Naked) is not in this list.
const std::vector<ModuleInfo> &modules();

/// Absolute path of an installed dependency kit, or empty when not found.
std::string findKitPath(const std::string &kitFile, const std::string &folder);
std::string baseKitPath();
std::string moduleKitPath(const std::string &moduleId);

/// Stable key used by Selection::disabled and the UI.
std::string pieceKey(const std::string &moduleId, const std::string &pieceId);

/// Assembles the base kit and, when selected, one shell module. `shell` may be
/// null to use the base shells. All pieces are listed in result.pieces whether
/// enabled or not, so the dropdown can still show a disabled piece.
BuildResult build(const DrumKit &base,
                  const std::string &baseModuleId,
                  const DrumKit *shell,
                  const std::string &shellModuleId,
                  const Selection &selection);

/// Convenience overload that loads the module from disk itself.
BuildResult buildFromDisk(const std::string &basePath,
                          const std::string &shellModuleId,
                          const Selection &selection);

} // namespace factory
} // namespace glorp
