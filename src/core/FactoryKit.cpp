#include "FactoryKit.h"
#include "GlorpKit.h"

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

int firstZoneNote(const DrumKit::LogicalPiece &piece)
{
    for (const auto &zone : piece.zones)
        if (zone.note >= 0)
            return zone.note;
    return -1;
}

std::vector<const DrumKit::LogicalPiece *> sortedTomPieces(const DrumKit &kit)
{
    std::vector<const DrumKit::LogicalPiece *> toms;
    for (const auto &lp : kit.logicalPieces)
        if (classifyPiece(lp.name, lp.id) == PieceClass::tom)
            toms.push_back(&lp);

    std::stable_sort(toms.begin(), toms.end(),
                     [](const DrumKit::LogicalPiece *a, const DrumKit::LogicalPiece *b)
                     {
                         const int an = firstZoneNote(*a);
                         const int bn = firstZoneNote(*b);
                         if (an != bn)
                             return an > bn;
                         return a->id < b->id;
                     });
    return toms;
}

const DrumKit *findKit(const std::vector<std::pair<std::string, const DrumKit *>> &kits,
                       const std::string &moduleId)
{
    for (const auto &[id, kit] : kits)
        if (id == moduleId)
            return kit;
    return nullptr;
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
bool readCatalog(const std::string &moduleId, const std::string &path, Catalog &catalog)
{
    catalog = {};
    catalog.moduleId = moduleId;
    catalog.moduleName = moduleName(moduleId);

    GlorpKitReader reader;
    if (!reader.open(path))
        return false;

    GlorpKitManifest manifest;
    if (reader.quickReadManifest(path, manifest))
        catalog.title = manifest.title;
    if (catalog.title.empty())
        catalog.title = catalog.moduleName;

    for (const auto &entry : reader.listEntries())
    {
        if (entry.size() < 15 || entry.compare(entry.size() - 15, 15, "/instrument.xml") != 0)
            continue;

        const auto xml = reader.readEntryString(entry);
        if (xml.empty())
            continue;

        std::string error;
        auto root = parseXml(xml, error);
        if (root == nullptr || root->tag != "glorp-instrument")
            continue;

        CatalogPiece piece;
        piece.name = root->attribute("name");
        if (piece.name.empty())
            continue;

        // instruments/<piece-id>/instrument.xml
        const auto firstSlash = entry.find('/');
        const auto secondSlash = entry.find('/', firstSlash + 1);
        if (firstSlash != std::string::npos && secondSlash != std::string::npos)
            piece.id = entry.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        if (piece.id.empty())
            piece.id = piece.name;

        for (auto *zone : root->childrenNamed("zone"))
        {
            const int note = zone->attributeInt("note", -1);
            if (note >= 0)
            {
                piece.note = note;
                break;
            }
        }

        piece.pieceClass = classifyPiece(piece.name, piece.id);
        catalog.pieces.push_back(std::move(piece));
    }

    catalog.valid = !catalog.pieces.empty();
    return catalog.valid;
}

int tomOrdinalForSlot(const std::string &moduleId, int slot)
{
    if (slot < 0 || slot >= tomSlots)
        return -1;

    if (moduleId == "salamander" || moduleId == "monster")
    {
        if (slot == 0)
            return 0;
        if (slot == 2)
            return 1;
        return -1;
    }

    return slot;
}

int tomCountForModule(const std::string &moduleId)
{
    if (moduleId == "naked")
        return 5;
    if (moduleId == "crocell")
        return 4;
    if (moduleId == "drs")
        return 3;
    if (moduleId == "salamander" || moduleId == "monster")
        return 2;
    return 0;
}

int tomSlotForOrdinal(const std::string &moduleId, int ordinal)
{
    if (ordinal < 0)
        return -1;

    if (moduleId == "salamander" || moduleId == "monster")
    {
        if (ordinal == 0)
            return 0;
        if (ordinal == 1)
            return 2;
        return -1;
    }

    return ordinal < tomSlots ? ordinal : -1;
}

std::vector<CatalogPiece> tomsOf(const Catalog &catalog)
{
    std::vector<CatalogPiece> toms;
    for (const auto &piece : catalog.pieces)
        if (piece.pieceClass == PieceClass::tom)
            toms.push_back(piece);

    std::stable_sort(toms.begin(), toms.end(),
                     [](const CatalogPiece &a, const CatalogPiece &b)
                     {
                         if (a.note != b.note)
                             return a.note > b.note;
                         return a.id < b.id;
                     });
    return toms;
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

std::string moduleName(const std::string &moduleId)
{
    if (moduleId == "naked")
        return "Naked Drums";
    for (const auto &m : modules())
        if (m.id == moduleId)
            return m.name;
    return moduleId;
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

std::vector<std::string> Selection::usedModules() const
{
    std::vector<std::string> used;
    auto add = [&used](const std::string &rawId)
    {
        if (rawId.empty())
            return;
        const std::string id = rawId == "naked2" ? "naked" : rawId;
        if (std::find(used.begin(), used.end(), id) == used.end())
            used.push_back(id);
    };

    add("naked");
    add(kickModule);
    add(snareModule);
    for (const auto &id : tomModules)
        add(id);

    return used;
}

//==============================================================================
std::vector<Configuration> factoryConfigurations()
{
    std::vector<Configuration> list;

    auto make = [](const std::string &id, const std::string &name, const std::string &module)
    {
        Configuration c;
        c.id = id;
        c.name = name;
        c.factory = true;
        c.selection.kickModule = module;
        c.selection.snareModule = module;
        c.selection.tomModules = {"", "", "", "", ""};
        for (int slot = 0; slot < tomSlots; ++slot)
        {
            const int ordinal = tomOrdinalForSlot(module, slot);
            c.selection.tomModules[static_cast<size_t>(slot)] =
                ordinal >= 0 && ordinal < tomCountForModule(module) ? module : std::string();
        }
        return c;
    };

    list.push_back(make("naked", "Naked Drums", "naked"));
    list.push_back(make("crocell", "Crocell Shells", "crocell"));
    list.push_back(make("drs", "DRS Shells", "drs"));
    list.push_back(make("salamander", "Salamander Shells", "salamander"));
    list.push_back(make("monster", "Monster Shells", "monster"));

    Configuration studio;
    studio.id = "studio";
    studio.name = "Studio Mix";
    studio.factory = true;
    studio.selection.kickModule = "monster";
    studio.selection.snareModule = "crocell";
    studio.selection.tomModules = {"drs", "drs", "drs", "", ""};
    list.push_back(std::move(studio));

    Configuration vintage;
    vintage.id = "vintage";
    vintage.name = "Vintage Mix";
    vintage.factory = true;
    vintage.selection.kickModule = "salamander";
    vintage.selection.snareModule = "drs";
    vintage.selection.tomModules = {"monster", "", "monster", "", ""};
    list.push_back(std::move(vintage));

    return list;
}

//==============================================================================
BuildResult build(const std::vector<std::pair<std::string, const DrumKit *>> &kits,
                  const Selection &selection)
{
    BuildResult result;
    auto &out = result.kit;

    const auto *base = findKit(kits, "naked");
    if (base == nullptr || base->instruments.empty())
    {
        result.error = "Naked Drums is not loaded.";
        return result;
    }

    const auto usedModules = selection.usedModules();
    for (const auto &id : usedModules)
        if (findKit(kits, id) == nullptr)
        {
            result.error = "Module '" + id + "' is not loaded.";
            return result;
        }

    // -- identity ----------------------------------------------------------
    out.title = base->title.empty() ? "Naked Drums" : base->title;
    out.license = base->license;
    out.author = base->author;
    out.description = base->description;
    out.version = base->version;
    out.declaredSampleRate = base->declaredSampleRate;
    out.hitGlowColor = base->hitGlowColor;
    out.imagePath = base->imagePath;
    out.imageClickMap = base->imageClickMap;
    out.isArchive = base->isArchive;
    out.archivePath = base->archivePath;
    out.xmlPath = base->xmlPath;
    out.isNativeGlorpKit = base->isNativeGlorpKit;
    out.isFactoryAssembly = true;
    out.warnings = base->warnings;

    std::vector<std::string> sourceNames;
    for (const auto &id : usedModules)
    {
        if (id == "naked")
            continue;
        const auto name = moduleName(id);
        if (std::find(sourceNames.begin(), sourceNames.end(), name) == sourceNames.end())
            sourceNames.push_back(name);
    }
    for (const auto &name : sourceNames)
        out.title += " + " + name;

    for (const auto &id : usedModules)
    {
        const auto *kit = findKit(kits, id);
        if (kit == nullptr || id == "naked" || kit->license.empty())
            continue;
        if (!out.license.empty())
            out.license += " / ";
        out.license += kit->license;
    }

    // -- channels ----------------------------------------------------------
    out.channels = base->channels;

    std::map<std::string, std::map<std::string, std::string>> channelNames;
    for (const auto &id : usedModules)
    {
        if (id == "naked")
            continue;

        const auto *kit = findKit(kits, id);
        if (kit == nullptr)
            continue;

        auto &names = channelNames[id];
        const auto name = moduleName(id);
        for (auto ch : kit->channels)
        {
            if (ch.name.empty())
                continue;

            const auto original = ch.name;
            ch.name = id + "_" + original;

            // A bottom snare mic is normally polarity-inverted; Crocell ships
            // it unflagged, which makes the snare thick and forward.
            const auto micLower = lowerCopy(original);
            if (id == "crocell"
                && (micLower.find("snarebottom") != std::string::npos
                    || micLower.find("snare_bottom") != std::string::npos))
                ch.phaseInvert = true;
            if (!ch.title.empty())
                ch.title = name + " " + ch.title;
            else
                ch.title = ch.name;

            names[original] = ch.name;
            out.channels.push_back(std::move(ch));
        }
    }

    // -- pieces ------------------------------------------------------------
    struct Suggestion
    {
        int note = -1;
        std::string name;
    };
    std::vector<Suggestion> suggestions;

    auto moduleDisplay = [](const std::string &moduleId, const DrumKit &kit, const Instrument &inst)
    {
        const auto baseName = inst.displayName.empty() ? inst.name : inst.displayName;
        if (moduleId == "naked")
            return baseName;
        return moduleName(moduleId) + " " + baseName;
    };

    enum class ShellKind
    {
        none,
        kick,
        snare,
        tom
    };
    int kickCounter = 0;

    auto roleToken = [](std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        for (auto &c : s)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.')
                c = '_';
        return s;
    };

    auto appendPiece = [&](const std::string &moduleId,
                           const DrumKit &sourceKit,
                           const DrumKit::LogicalPiece &srcLp,
                           bool include,
                           ShellKind shellKind,
                           int shellSlot,
                           bool forceSnareMain = false)
    {
        PieceEntry entry;
        entry.moduleId = moduleId;
        entry.moduleName = moduleName(moduleId);
        entry.id = srcLp.id;
        entry.name = srcLp.name;
        entry.pieceClass = classifyPiece(srcLp.name, srcLp.id);
        result.pieces.push_back(std::move(entry));

        if (!include)
            return;

        DrumKit::LogicalPiece lp = srcLp;
        lp.sourceModule = moduleId;
        lp.sourceId = srcLp.id;

        if (moduleId != "naked")
        {
            lp.id = moduleId + "_" + srcLp.id;
            lp.name = moduleName(moduleId) + " " + srcLp.name;
        }

        const auto channelIt = channelNames.find(moduleId);

        for (auto &zi : lp.zones)
        {
            const int oldIndex = zi.samplerIndex;
            if (oldIndex < 0 || oldIndex >= static_cast<int>(sourceKit.instruments.size()))
                continue;

            Instrument inst = sourceKit.instruments[static_cast<size_t>(oldIndex)];
            inst.index = static_cast<int>(out.instruments.size());
            inst.sourceModule = moduleId;

            if (moduleId != "naked" && channelIt != channelNames.end())
            {
                for (auto &cm : inst.channelMaps)
                {
                    const auto it = channelIt->second.find(cm.out);
                    if (it != channelIt->second.end())
                        cm.out = it->second;
                }
                for (auto &hit : inst.resolvedChannels)
                    for (auto &source : hit)
                    {
                        const auto it = channelIt->second.find(source.kitChannel);
                        if (it != channelIt->second.end())
                            source.kitChannel = it->second;
                    }

                for (auto &choke : inst.chokes)
                    choke.instrument = moduleName(moduleId) + " " + choke.instrument;
            }

            inst.displayName = moduleDisplay(moduleId, sourceKit, inst);

            int suggestedNote = zi.note;
            std::string midiRole;

            if (shellKind == ShellKind::kick)
            {
                const int ordinal = kickCounter++;
                suggestedNote = ordinal == 0 ? 36 : std::max(1, 36 - ordinal);
                midiRole = ordinal == 0 ? "kick.main" : "kick.alt" + std::to_string(ordinal);
            }
            else if (shellKind == ShellKind::snare)
            {
                if (forceSnareMain)
                {
                    suggestedNote = 38;
                    midiRole = "snare.main";
                }
                else
                {
                    const auto lower = roleToken(srcLp.name + " " + srcLp.id + " " + zi.name);
                    if (lower.find("stick") != std::string::npos || lower.find("rim") != std::string::npos)
                    {
                        suggestedNote = 37;
                        midiRole = "snare.stick";
                    }
                    else if (lower.find("rest") != std::string::npos || lower.find("off") != std::string::npos)
                    {
                        suggestedNote = lower.find("2") != std::string::npos ? 41 : 39;
                        midiRole = lower.find("2") != std::string::npos ? "snare.rest2" : "snare.rest";
                    }
                    else if (lower.find("2") != std::string::npos)
                    {
                        suggestedNote = 40;
                        midiRole = "snare.2";
                    }
                    else
                    {
                        suggestedNote = 38;
                        midiRole = "snare.main";
                    }
                }
            }
            else if (shellKind == ShellKind::tom)
            {
                static const int tomNotes[tomSlots] {48, 47, 45, 43, 41};
                const int roleSlot = std::clamp(shellSlot, 0, tomSlots - 1);
                suggestedNote = tomNotes[roleSlot];
                midiRole = "tom." + std::to_string(roleSlot + 1);
            }
            else
            {
                midiRole = "base." + roleToken(srcLp.id) + "." + roleToken(zi.name);
            }

            inst.midiRole = midiRole;

            if (suggestedNote >= 0)
                suggestions.push_back({suggestedNote, inst.displayName});
            if (shellKind == ShellKind::none && zi.altNote >= 0)
                suggestions.push_back({zi.altNote, inst.displayName});

            zi.samplerIndex = inst.index;
            out.instruments.push_back(std::move(inst));
        }

        out.logicalPieces.push_back(std::move(lp));
    };

    auto enabled = [&selection](const std::string &moduleId, const std::string &pieceId)
    {
        return selection.disabled.count(pieceKey(moduleId, pieceId)) == 0;
    };

    // Cymbals, hi-hat, ride and auxiliary base pieces are always Naked.
    for (const auto &lp : base->logicalPieces)
    {
        if (isShellClass(classifyPiece(lp.name, lp.id)))
            continue;
        appendPiece("naked", *base, lp, enabled("naked", lp.id), ShellKind::none, 0);
    }

    // Kick: every kick piece from the chosen source (Crocell has two).
    if (!selection.kickModule.empty())
    {
        const auto *kit = findKit(kits, selection.kickModule);
        if (kit != nullptr)
            for (const auto &lp : kit->logicalPieces)
                if (classifyPiece(lp.name, lp.id) == PieceClass::kick)
                    appendPiece(selection.kickModule, *kit, lp, enabled(selection.kickModule, lp.id),
                                ShellKind::kick, 0);
    }

    // Snare: one Naked snare variant, or every snare piece from a module
    // (Crocell also has a rest).
    if (!selection.snareModule.empty())
    {
        if (selection.snareModule == "naked" || selection.snareModule == "naked2")
        {
            const auto *kit = base;
            const std::string wanted = selection.snareModule == "naked2" ? "snare_2" : "snare_1";
            for (const auto &lp : kit->logicalPieces)
                if (lp.id == wanted || (wanted == "snare_1" && lp.id == "snare"))
                {
                    appendPiece("naked", *kit, lp, enabled("naked", lp.id), ShellKind::snare, 0,
                                selection.snareModule == "naked2");
                    break;
                }
        }
        else
        {
            const auto *kit = findKit(kits, selection.snareModule);
            if (kit != nullptr)
                for (const auto &lp : kit->logicalPieces)
                    if (classifyPiece(lp.name, lp.id) == PieceClass::snare)
                        appendPiece(selection.snareModule, *kit, lp, enabled(selection.snareModule, lp.id),
                                    ShellKind::snare, 0);
        }
    }

    // Toms: each slot can come from a different module.
    for (int slot = 0; slot < tomSlots; ++slot)
    {
        const auto &moduleId = selection.tomModules[static_cast<size_t>(slot)];
        if (moduleId.empty())
            continue;

        const auto *kit = findKit(kits, moduleId);
        if (kit == nullptr)
            continue;

        const int ordinal = tomOrdinalForSlot(moduleId, slot);
        if (ordinal < 0)
            continue;

        const auto toms = sortedTomPieces(*kit);
        if (ordinal >= static_cast<int>(toms.size()))
        {
            out.warnings.push_back(moduleName(moduleId) + " has only " + std::to_string(toms.size())
                                   + " toms; tom slot " + std::to_string(slot + 1) + " is empty.");
            continue;
        }

        const auto *lp = toms[static_cast<size_t>(ordinal)];
        appendPiece(moduleId, *kit, *lp, enabled(moduleId, lp->id), ShellKind::tom, slot);
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
    out.factoryMicLevels = base->factoryMicLevels;
    out.factoryMicPans = base->factoryMicPans;
    out.factoryMicPhaseInvert = base->factoryMicPhaseInvert;
    out.factoryInstrumentLevels = base->factoryInstrumentLevels;
    out.factoryInstrumentPans = base->factoryInstrumentPans;

    for (const auto &id : usedModules)
    {
        if (id == "naked")
            continue;
        const auto *kit = findKit(kits, id);
        if (kit == nullptr)
            continue;

        const auto &names = channelNames[id];
        for (const auto &[name, value] : kit->factoryMicLevels)
        {
            const auto it = names.find(name);
            out.factoryMicLevels[it != names.end() ? it->second : name] = value;
        }
        for (const auto &[name, value] : kit->factoryMicPans)
        {
            const auto it = names.find(name);
            out.factoryMicPans[it != names.end() ? it->second : name] = value;
        }
        for (const auto &[name, value] : kit->factoryMicPhaseInvert)
        {
            const auto it = names.find(name);
            out.factoryMicPhaseInvert[it != names.end() ? it->second : name] = value;
        }

        if (id == "crocell")
        {
            for (const auto &[original, mapped] : names)
            {
                const auto micLower = lowerCopy(original);

                if (micLower.find("snarebottom") != std::string::npos
                    || micLower.find("snare_bottom") != std::string::npos)
                    out.factoryMicPhaseInvert[mapped] = true;

            }
        }

        if (id == "drs")
        {
            for (const auto &[original, mapped] : names)
            {
                const auto micLower = lowerCopy(original);
                if (micLower.find("kdrum_back") != std::string::npos)
                {
                    out.factoryMicLevels[mapped] = 1.80f;
                    out.factoryMicPhaseInvert[mapped] = true;
                }
                else if (micLower.find("kdrum_front") != std::string::npos)
                {
                    out.factoryMicLevels[mapped] = 0.35f;
                    out.factoryMicPhaseInvert[mapped] = false;
                }
            }
        }

        out.warnings.insert(out.warnings.end(), kit->warnings.begin(), kit->warnings.end());
    }

    return result;
}

BuildResult buildFromDisk(const Selection &selection)
{
    BuildResult result;

    const auto basePath = baseKitPath();
    if (basePath.empty())
    {
        result.error = "Naked Drums is not installed. Add Naked Drums.glorpkit to the kit folder.";
        return result;
    }

    std::map<std::string, DrumKit> loaded;
    loaded["naked"] = DrumKit::load(basePath);
    if (!loaded["naked"].error.empty() || loaded["naked"].instruments.empty())
    {
        result.error = loaded["naked"].error.empty() ? "Naked Drums could not be loaded." : loaded["naked"].error;
        return result;
    }

    for (const auto &id : selection.usedModules())
    {
        if (id == "naked" || loaded.count(id) != 0)
            continue;

        const auto path = moduleKitPath(id);
        if (path.empty())
        {
            result.error = "Shell module '" + moduleName(id) + "' is not installed.";
            return result;
        }

        auto kit = DrumKit::load(path);
        if (!kit.error.empty() || kit.instruments.empty())
        {
            result.error = kit.error.empty() ? ("Shell module '" + moduleName(id) + "' could not be loaded.")
                                             : kit.error;
            return result;
        }

        loaded[id] = std::move(kit);
    }

    std::vector<std::pair<std::string, const DrumKit *>> kits;
    kits.reserve(loaded.size());
    for (const auto &[id, kit] : loaded)
        kits.emplace_back(id, &kit);

    return build(kits, selection);
}

} // namespace factory
} // namespace glorp
