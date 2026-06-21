#pragma once

#include <memory>

#include "Setting/SettingsCache.hpp"
#include "Utils/KRandom.hpp"
#include "Utils/Diagram.hpp"

struct TemplateSpawner {
    Vector2f position;
    const TemplateContainer *container;
};

class WorldGen
{
private:
    int m_seed;
    const SettingsCache &m_settings;
    const World &m_world;
    WorldPlacement *m_placement = nullptr;
    std::vector<TemplateSpawner> m_templates;

public:
    WorldGen(World &world, SettingsCache &settings, int seed)
        : m_seed{seed}
        , m_settings{settings}
        , m_world{world}
    {
    }

    bool GenerateOverworld(std::vector<Site> &sites);

    std::vector<Vector3i> GetGeysers(int seed) const;

private:
    template<typename T>
    const T &GetDefaultData(const std::string &key)
    {
        return m_settings.GetDefaultData<T>(m_world, key);
    }
    bool GenerateSeedPoints(KRandom &random, std::vector<Site> &sites);
    void PropagateDistanceTags(std::vector<Site> &sites) const;
    void ConvertUnknownCells(std::vector<Site> &allSites, KRandom &random);
    size_t GenerateChildren(Site &site, KRandom &random, int seed, bool usePD);
    void SetFeatureBiome(Site &site, KRandom &random, const Feature *feature);
    bool DetermineTemplates(std::vector<Site *> &sites, KRandom &random);
    static bool ForceLowestToLeaf(std::vector<Site> &sites,
                                  std::vector<Site *> &allSites);
};

// clang-format off
inline std::string ZoneTypeToString(ZoneType zone)
{
    const char *dict[] = {
        "FrozenWastes", "CrystalCaverns",    "BoggyMarsh",     "Sandstone",
        "ToxicJungle",  "MagmaCore",         "OilField",       "Space",
        "Ocean",        "Rust",              "Forest",         "Radioactive",
        "Swamp",        "Wasteland",         "RocketInterior", "Metallic",
        "Barren",       "Moo",               "IceCaves",       "CarrotQuarry",
        "SugarWoods",   "PrehistoricGarden", "PrehistoricRaptor", "PrehistoricWetlands",
        "KelpForest",   "Reef",              "Abyss",          "Beach"};
    static_assert((int)ZoneType::MaxZoneType == std::size(dict));
    return dict[(int)zone];
}
// clang-format on

inline std::string TempRangeToString(Range range)
{
    const char *dict[] = {
        "ExtremelyCold", "VeryVeryCold", "VeryCold", "Cold",        "Chilly",
        "Cool",          "Mild",         "Room",     "HumanWarm",   "HumanHot",
        "SomewhatHot",   "Hot",          "VeryHot",  "ExtremelyHot"};
    static_assert((int)Range::MaxRange == std::size(dict));
    return dict[(int)range];
}
