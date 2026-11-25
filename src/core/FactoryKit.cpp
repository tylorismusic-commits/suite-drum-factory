#include "FactoryKit.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>

namespace glorp
{
namespace factory
{
namespace
{
namespace fs = std::filesystem;

std::string lowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool has(const std::string &haystack, const char *needle)
{
    return haystack.find(needle) != std::string::npos;
}

fs::path homePath()
{
    if (const char *home = std::getenv("HOME"))
        return fs::path(home);
    return {};
}

std::string firstExisting(const std::vector<fs::path> &candidates)
{
    std::error_code ec;
    for (const auto &p : candidates)
        if (!p.empty() && fs::is_regular_file(p, ec))
            return p.string();
    return {};
}

std::string displayFor(const DrumKit &kit, const std::string &moduleName, const Instrument &inst)
{
    if (moduleName.empty())
        return inst.displayName.empty() ? inst.name : inst.displayName;
    return moduleName + " " + (inst.displayName.empty() ? inst.name : inst.displayName);
}
} // namespace

//==============================================================================
PieceClass classifyPiece(const std::string &name, const std::string &id)
{
    const auto s = lowerCopy(name + " " + id);

    if (has(s, "hihat") || has(s, "hi-hat") || has(s, "hi_hat"))
        return PieceClass::hihat;
    if (has(s, "kick") || has(s, "kdrum") || has(s, "bassdrum") || has(s, "bass drum"))
        return PieceClass::kick;
    if (has(s, "snare"))
        return PieceClass::snare;
    if (has(s, "tom"))
        return PieceClass::tom;
    if (has(s, "ride"))
        return PieceClass::ride;
    if (has(s, "crash"))
        return PieceClass::crash;
    if (has(s, "splash"))
        return PieceClass::splash;
    if (has(s, "china"))
        return PieceClass::china;

    return PieceClass::aux;
}

bool isShellClass(PieceClass cls) noexcept
{
    return cls == PieceClass::kick || cls == PieceClass::snare || cls == PieceClass::tom;
}

std::string pieceKey(const std::string &moduleId, const std::string &pieceId)
{
    return moduleId + ":" + pieceId;
}

//==============================================================================
const std::vector<ModuleInfo> &modules()
{
    static const std::vector<ModuleInfo> list {
        {"crocell", "Crocell", "CrocellKit.glorpkit", "CrocellKit"},
        {"drs", "DRS", "DRSKit.glorpkit", "DRSKit"},
        {"salamander", "Salamander", "Salamander Drumkit.glorpkit", "Salamander Drumkit"},
        {"monster", "Monster", "Monster Drumkit.glorpkit", "Monster Drumkit"},
    };
    return list;
}

std::string findKitPath(const std::string &kitFile, const std::string &folder)
{
    const auto home = homePath();
    if (home.empty())
        return {};

    std::vector<fs::path> candidates;
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"))
        candidates.push_back(fs::path(xdg) / "SuiteDrumsGlorp/kits" / kitFile);

    candidates.push_back(home / ".config/SuiteDrumsGlorp/kits" / kitFile);
    candidates.push_back(home / ".config/SuiteDrumFactory/kits" / kitFile);
    if (!folder.empty())
        candidates.push_back(home / "Documents/SuiteDrumsGlorpShare" / folder / kitFile);

    return firstExisting(candidates);
}

std::string baseKitPath()
{
    return findKitPath("Naked Drums.glorpkit", "Naked Drums");
}

std::string moduleKitPath(const std::string &moduleId)
{
    for (const auto &m : modules())
        if (m.id == moduleId)
            return findKitPath(m.kitFile, m.folder);
    return {};
}

//==============================================================================
BuildResult build(const DrumKit &base,
                  const std::string &baseModuleId,
                  const DrumKit *shell,
                  const std::string &shellModuleId,
                  const Selection &selection)
{
    BuildResult result;
    auto &out = result.kit;

    const bool hasShell = shell != nullptr && !shellModuleId.empty() && !shell->instruments.empty();
    const std::string shellName = [&]
    {
        for (const auto &m : modules())
            if (m.id == shellModuleId)
                return m.name;
        return shellModuleId;
    }();

    // -- identity ----------------------------------------------------------
    out.title = base.title.empty() ? "Naked Drums" : base.title;
    if (hasShell)
    {
        const auto shellTitle = shell->title.empty() ? shellName : shell->title;
        out.title = out.title + " + " + shellTitle;
        out.license = base.license;
        if (!shell->license.empty())
            out.license += out.license.empty() ? shell->license : " / " + shell->license;
    }
    else
    {
        out.license = base.license;
    }
    out.author = base.author;
    out.description = base.description;
    out.version = base.version;
    out.declaredSampleRate = base.declaredSampleRate;
    out.hitGlowColor = base.hitGlowColor;
    out.imagePath = base.imagePath;
    out.imageClickMap = base.imageClickMap;
    out.isArchive = base.isArchive;
    out.archivePath = base.archivePath;
    out.xmlPath = base.xmlPath;
    out.isNativeGlorpKit = base.isNativeGlorpKit;
    out.warnings = base.warnings;

    // -- channels ----------------------------------------------------------
    out.channels = base.channels;

    std::map<std::string, std::string> shellChannelNames;
    if (hasShell)
    {
        for (auto ch : shell->channels)
        {
            if (ch.name.empty())
                continue;

            const auto original = ch.name;
            ch.name = shellModuleId + "_" + original;
            if (!ch.title.empty())
                ch.title = shellName + " " + ch.title;
            else
                ch.title = ch.name;

            shellChannelNames[original] = ch.name;
            out.channels.push_back(std::move(ch));
        }
    }

    // -- candidate pieces --------------------------------------------------
    struct Candidate
    {
        const DrumKit *source = nullptr;
        const DrumKit::LogicalPiece *piece = nullptr;
        PieceClass pieceClass = PieceClass::aux;
        bool include = false;
        bool fromShell = false;
        bool listInMenu = true;
    };

    std::vector<Candidate> candidates;

    for (const auto &lp : base.logicalPieces)
    {
        const auto cls = classifyPiece(lp.name, lp.id);
        const bool isShell = isShellClass(cls);
        Candidate c;
        c.source = &base;
        c.piece = &lp;
        c.pieceClass = cls;
        c.fromShell = false;
        c.listInMenu = !(hasShell && isShell);
        c.include = !(hasShell && isShell)
                    && selection.disabled.count(pieceKey(baseModuleId, lp.id)) == 0;
        candidates.push_back(c);
    }

    if (hasShell)
    {
        for (const auto &lp : shell->logicalPieces)
        {
            const auto cls = classifyPiece(lp.name, lp.id);
            if (!isShellClass(cls))
                continue;

            Candidate c;
            c.source = shell;
            c.piece = &lp;
            c.pieceClass = cls;
            c.fromShell = true;
            c.include = selection.disabled.count(pieceKey(shellModuleId, lp.id)) == 0;
            candidates.push_back(c);
        }
    }

    // -- append instruments and logical pieces ----------------------------
    struct Suggestion
    {
        int note = -1;
        std::string name;
        PieceClass pieceClass = PieceClass::aux;
    };
    std::vector<Suggestion> suggestions;

    for (const auto &c : candidates)
    {
        const auto &srcKit = *c.source;
        const auto &srcLp = *c.piece;
        const auto &moduleId = c.fromShell ? shellModuleId : baseModuleId;

        if (!c.listInMenu)
            continue;

        PieceEntry entry;
        entry.moduleId = moduleId;
        entry.moduleName = c.fromShell ? shellName : "Naked";
        entry.id = srcLp.id;
        entry.name = srcLp.name;
        entry.pieceClass = c.pieceClass;
        result.pieces.push_back(std::move(entry));

        if (!c.include)
            continue;

        DrumKit::LogicalPiece lp = srcLp;
        lp.sourceModule = moduleId;
        lp.sourceId = srcLp.id;
        if (c.fromShell)
        {
            lp.id = moduleId + "_" + srcLp.id;
            lp.name = shellName + " " + srcLp.name;
        }

        for (auto &zi : lp.zones)
        {
            const int oldIndex = zi.samplerIndex;
            if (oldIndex < 0 || oldIndex >= static_cast<int>(srcKit.instruments.size()))
                continue;

            Instrument inst = srcKit.instruments[static_cast<size_t>(oldIndex)];
            inst.index = static_cast<int>(out.instruments.size());
            inst.sourceModule = moduleId;

            if (c.fromShell)
            {
                // Module mics are namespaced so Crocell's SnareTop and Naked's
                // snare_top_1 remain two independent mixer strips.
                for (auto &cm : inst.channelMaps)
                {
                    const auto it = shellChannelNames.find(cm.out);
                    if (it != shellChannelNames.end())
                        cm.out = it->second;
                }
                for (auto &hit : inst.resolvedChannels)
                    for (auto &source : hit)
                    {
                        const auto it = shellChannelNames.find(source.kitChannel);
                        if (it != shellChannelNames.end())
                            source.kitChannel = it->second;
                    }

                // Choke targets are stored by instrument name, so they need
                // the same prefix as the display name.
                for (auto &choke : inst.chokes)
                    choke.instrument = shellName + " " + choke.instrument;

                inst.displayName = displayFor(srcKit, shellName, inst);
            }

            if (zi.note >= 0)
                suggestions.push_back({zi.note, inst.displayName, c.pieceClass});
            if (zi.altNote >= 0)
                suggestions.push_back({zi.altNote, inst.displayName, c.pieceClass});

            zi.samplerIndex = inst.index;
            out.instruments.push_back(std::move(inst));
        }

        out.logicalPieces.push_back(std::move(lp));
    }

    if (out.instruments.empty())
    {
        result.error = "The assembled kit has no playable pieces. Enable at least one piece.";
        return result;
    }

    // -- MIDI suggestions --------------------------------------------------
    std::set<int> usedNotes;
    for (const auto &s : suggestions)
    {
        int note = s.note;
        if (note < 0 || note > 127 || usedNotes.count(note) != 0)
        {
            note = -1;
            for (int candidate = 35; candidate <= 81; ++candidate)
                if (usedNotes.count(candidate) == 0)
                {
                    note = candidate;
                    break;
                }
        }
        if (note < 0)
            continue;

        usedNotes.insert(note);
        out.suggestedMap[note] = s.name;
    }

    // -- factory mixer values ---------------------------------------------
    out.factoryMicLevels = base.factoryMicLevels;
    out.factoryMicPans = base.factoryMicPans;
    out.factoryMicPhaseInvert = base.factoryMicPhaseInvert;
    out.factoryInstrumentLevels = base.factoryInstrumentLevels;
    out.factoryInstrumentPans = base.factoryInstrumentPans;

    if (hasShell)
    {
        for (const auto &[name, value] : shell->factoryMicLevels)
        {
            const auto it = shellChannelNames.find(name);
            out.factoryMicLevels[it != shellChannelNames.end() ? it->second : name] = value;
        }
        for (const auto &[name, value] : shell->factoryMicPans)
        {
            const auto it = shellChannelNames.find(name);
            out.factoryMicPans[it != shellChannelNames.end() ? it->second : name] = value;
        }
        for (const auto &[name, value] : shell->factoryMicPhaseInvert)
        {
            const auto it = shellChannelNames.find(name);
            out.factoryMicPhaseInvert[it != shellChannelNames.end() ? it->second : name] = value;
        }
        out.warnings.insert(out.warnings.end(), shell->warnings.begin(), shell->warnings.end());
    }

    return result;
}

BuildResult buildFromDisk(const std::string &basePath,
                          const std::string &shellModuleId,
                          const Selection &selection)
{
    BuildResult result;

    if (basePath.empty())
    {
        result.error = "Naked Drums is not installed. Add Naked Drums.glorpkit to the kit folder.";
        return result;
    }

    auto base = DrumKit::load(basePath);
    if (!base.error.empty() || base.instruments.empty())
    {
        result.error = base.error.empty() ? "Naked Drums could not be loaded." : base.error;
        return result;
    }

    DrumKit shell;
    DrumKit *shellPtr = nullptr;
    std::string shellId;

    if (!shellModuleId.empty())
    {
        const auto path = moduleKitPath(shellModuleId);
        if (path.empty())
        {
            result.error = "Shell module '" + shellModuleId + "' is not installed.";
            return result;
        }

        shell = DrumKit::load(path);
        if (!shell.error.empty() || shell.instruments.empty())
        {
            result.error = shell.error.empty() ? ("Shell module '" + shellModuleId + "' could not be loaded.")
                                               : shell.error;
            return result;
        }

        shellPtr = &shell;
        shellId = shellModuleId;
    }

    return build(base, "naked", shellPtr, shellId, selection);
}

} // namespace factory
} // namespace glorp
