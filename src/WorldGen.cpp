#include "WorldGen.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <unordered_set>
#include <queue>
#include <numeric>

#include "Utils/Voronoi.hpp"
#include "Utils/Diagram.hpp"
#include "Utils/PointGenerator.hpp"
#include "Utils/Log.hpp"

struct WeightedSubWorld {
    const SubWorld *subWorld;
    float weight;
    float overridePower;
    int minCount;
    int maxCount;
    int priority;

    WeightedSubWorld(const SubWorld *subworld,
                     const WeightedSubworldName &weightedWorld)
        : subWorld{subworld}
        , weight{weightedWorld.weight}
        , overridePower{weightedWorld.overridePower}
        , minCount{weightedWorld.minCount}
        , maxCount{weightedWorld.maxCount}
        , priority{weightedWorld.priority}
    {
    }
};

static void ApplySwapTags(std::vector<Site> &sites, KRandom &random);
extern void WriteToBinary(const std::vector<Site> &sites);
extern void WriteToBinary(const std::vector<Site *> &sites);

bool WorldGen::GenerateOverworld(std::vector<Site> &sites)
{
    KRandom random(m_seed);
    if (!GenerateSeedPoints(random, sites)) {
        return false;
    }
    bool usePD = m_world.layoutMethod == LayoutMethod::PowerTree;
    Polygon bounds(Rect(0.0f, 0.0f, m_world.worldsize.x, m_world.worldsize.y));
    Diagram diagram(bounds, sites);
    if (usePD) {
        diagram.ComputeNode();
        diagram.ComputeNodePD();
    } else {
        diagram.ComputeNode();
    }
    PropagateDistanceTags(sites);
    ConvertUnknownCells(sites, random);
    if (usePD) {
        diagram.ComputeNodePD();
    }
    int count = 0;
    for (int i = 0; i < (int)sites.size(); ++i) {
        count += GenerateChildren(sites[i], random, m_seed + i, usePD);
    }
    std::vector<Site *> allSites;
    allSites.reserve(count);
    if (!ForceLowestToLeaf(sites, allSites)) {
        return false;
    }
    random = KRandom(m_seed);
    ApplySwapTags(sites, random);
    DetermineTemplates(allSites, random);
    return true;
}

bool WorldGen::ForceLowestToLeaf(std::vector<Site> &sites,
                                 std::vector<Site *> &allSites)
{
    int index = 1;
    Site *startSite = nullptr;
    for (auto &site : sites) {
        for (auto &child : *site.children) {
            child.idx = index++;
            allSites.push_back(&child);
            if (startSite == nullptr && child.tags.contains("StartLocation")) {
                startSite = &child;
            }
        }
    }
    if (startSite == nullptr) {
        return false;
    }
    for (auto &site : *startSite->parent->children) {
        site.tags.insert("IgnoreCaveOverride");
    }
    for (auto neighbour : startSite->neighbours) {
        const_cast<Site *>(neighbour)->tags.insert("NearStartLocation");
    }
    return true;
}

static inline void SwitchNodes(Site &lhs, Site &rhs)
{
    std::swap(lhs.idx, rhs.idx);
    std::swap(lhs.x, rhs.x);
    std::swap(lhs.y, rhs.y);
    lhs.polygon.Swap(rhs.polygon);
}

static void ApplySwapTags(std::vector<Site> &sites, KRandom &random)
{
    std::vector<Site *> nodes;
    for (auto &site : sites) {
        if (site.tags.contains("CenteralFeature") ||
            !site.tags.contains("SwapLakesToBelow")) {
            continue;
        }
        nodes.clear();
        for (auto &child : *site.children) {
            if (!child.tags.contains("CenteralFeature")) {
                nodes.push_back(&child);
            }
        }
        ShuffleSeeded(nodes, random);
        std::queue<Site *> above;
        std::queue<Site *> below;
        for (auto node : nodes) {
            bool isWet = node->tags.contains("Wet");
            bool isAbove = node->y > site.y;
            if (isWet && isAbove) {
                above.push(node);
            } else if (!isWet && !isAbove) {
                below.push(node);
            }
        }
        while (!above.empty() && !below.empty()) {
            SwitchNodes(*above.front(), *below.front());
            above.pop();
            below.pop();
        }
    }
}

static bool RunFilterTagCommand(Site &site, const AllowedCellsFilter &filter)
{
    switch (filter.tagcommand) {
    case TagCommand::Default:
        return true;
    case TagCommand::AtTag:
        return std::ranges::contains(site.tags, filter.tag);
    case TagCommand::NotAtTag:
        return !std::ranges::contains(site.tags, filter.tag);
    case TagCommand::DistanceFromTag: {
        auto distance = site.minDistanceToTag[filter.tag];
        if (distance < filter.minDistance || filter.maxDistance < distance) {
            return false;
        } else {
            return true;
        }
    }
    default:
        return false;
    }
}

static HashSet<WeightedSubWorld *>
GetNameFilterSet(const AllowedCellsFilter &filter,
                 std::vector<WeightedSubWorld> &subworlds)
{
    HashSet<WeightedSubWorld *> hashSet;
    for (auto &name : filter.subworldNames) {
        for (auto &subworld : subworlds) {
            if (subworld.subWorld == nullptr)
                continue;
            if (subworld.subWorld->name == name) {
                hashSet.Append(&subworld);
            }
        }
    }
    return hashSet;
}

static HashSet<WeightedSubWorld *> GetZoneTypeFilterSet(
    const AllowedCellsFilter &filter,
    std::map<int, std::vector<WeightedSubWorld *>> &subworldsByZoneType)
{
    HashSet<WeightedSubWorld *> hashSet;
    for (auto &zoneType : filter.zoneTypes) {
        auto &subWorlds = subworldsByZoneType[(int)zoneType];
        for (auto &subworld : subWorlds) {
            if (subworld->subWorld == nullptr)
                continue;
            hashSet.Append(subworld);
        }
    }
    return hashSet;
}

static HashSet<WeightedSubWorld *> GetTemperatureFilterSet(
    const AllowedCellsFilter &filter,
    std::map<int, std::vector<WeightedSubWorld *>> subworldsByTemperature)
{
    HashSet<WeightedSubWorld *> hashSet;
    for (auto &tempRange : filter.temperatureRanges) {
        auto &subWorlds = subworldsByTemperature[(int)tempRange];
        for (auto &subworld : subWorlds) {
            if (subworld->subWorld == nullptr)
                continue;
            hashSet.Append(subworld);
        }
    }
    return hashSet;
}

static std::vector<WeightedSubWorld *>
Filter(Site &site, const World &world,
       std::vector<WeightedSubWorld> &allSubWorlds,
       std::map<int, std::vector<WeightedSubWorld *>> &subworldsByTemperature,
       std::map<int, std::vector<WeightedSubWorld *>> &subworldsByZoneType)
{
    HashSet<WeightedSubWorld *> hashSet;
    HashSet<WeightedSubWorld *> hashSet2;
    for (auto filter : world.unknownCellsAllowedSubworlds2) {
        hashSet2.Clear();
        bool useFilter = RunFilterTagCommand(site, *filter);
        if (useFilter && !filter->subworldNames.empty()) {
            hashSet2.UnionWith(GetNameFilterSet(*filter, allSubWorlds));
        }
        if (useFilter && !filter->temperatureRanges.empty()) {
            hashSet2.UnionWith(
                GetTemperatureFilterSet(*filter, subworldsByTemperature));
        }
        if (useFilter && !filter->zoneTypes.empty()) {
            hashSet2.UnionWith(
                GetZoneTypeFilterSet(*filter, subworldsByZoneType));
        }
        switch (filter->command) {
        case Command::Clear:
            if (useFilter) {
                hashSet.Clear();
            }
            break;
        case Command::Replace:
            if (hashSet2.Size() > 0) {
                hashSet.Clear();
                hashSet.UnionWith(hashSet2);
            }
            break;
        case Command::UnionWith:
            hashSet.UnionWith(hashSet2);
            break;
        case Command::ExceptWith:
            hashSet.ExceptWith(hashSet2);
            break;
        case Command::IntersectWith: {
            hashSet.IntersectWith(hashSet2);
            break;
        }
        case Command::SymmetricExceptWith: {
            hashSet.SymmetricExceptWith(hashSet2);
            break;
        }
        case Command::All:
        default:
            break;
        }
    }
    return hashSet.ToList();
}

static void ApplySubworldToNode(Site &site, const SubWorld &subWorld,
                                float overridePower = -1.0f)
{
    site.subworld = &subWorld;
    site.weight = ((overridePower > 0.0f) ? overridePower : subWorld.pdWeight);
    for (auto &tag : subWorld.tags) {
        site.tags.insert(tag);
    }
    site.tags.insert(subWorld.name);
    site.tags.insert(ZoneTypeToString(subWorld.zoneType));
    site.tags.insert(TempRangeToString(subWorld.temperatureRange));
}

void WorldGen::ConvertUnknownCells(std::vector<Site> &sites, KRandom &random)
{
    std::vector<int> indices(sites.size() - 1);
    std::iota(indices.begin(), indices.end(), 1);
    ShuffleSeeded(indices, random);
    std::vector<WeightedSubWorld> subworldsForWorld;
    const std::vector<SubWorld *> *subworldList = nullptr;
    if (m_settings.IsSpaceOutEnabled()) {
        auto itr = m_settings.orderedSubworlds.find("SPACEOUT");
        if (itr != m_settings.orderedSubworlds.end()) {
            subworldList = &itr->second;
        }
    } else {
        auto itr = m_settings.orderedSubworlds.find("VANILLA");
        if (itr != m_settings.orderedSubworlds.end()) {
            subworldList = &itr->second;
        }
    }
    if (subworldList == nullptr) {
        return;
    }
    for (auto item : *subworldList) {
        for (auto subworld : m_world.subworldFiles2) {
            if (item->name == subworld->name) {
                subworldsForWorld.emplace_back(item, *subworld);
            }
        }
    }
    std::map<int, std::vector<WeightedSubWorld *>> dict1;
    for (int i = 0; i <= (int)Range::ExtremelyHot; ++i) {
        auto &list = dict1[i];
        for (auto &subworld : subworldsForWorld) {
            if (subworld.subWorld->temperatureRange == (Range)i) {
                list.push_back(&subworld);
            }
        }
    }
    std::map<int, std::vector<WeightedSubWorld *>> dict2;
    for (int i = 0; i <= (int)ZoneType::SugarWoods; ++i) {
        auto &list = dict2[i];
        for (auto &subworld : subworldsForWorld) {
            if (subworld.subWorld->zoneType == (ZoneType)i) {
                list.push_back(&subworld);
            }
        }
    }
    for (auto index : indices) {
        auto &site = sites[index];
        auto list2 = Filter(site, m_world, subworldsForWorld, dict1, dict2);
        std::vector<WeightedSubWorld *> list3;
        for (auto &item : list2) {
            if (item->minCount > 0) {
                list3.push_back(item);
            }
        }
        WeightedSubWorld *weightedSubWorld;
        if (!list3.empty()) {
            weightedSubWorld = list3[0];
            int priority = weightedSubWorld->priority;
            for (auto &item2 : list3) {
                if (item2->priority > priority ||
                    (item2->priority == priority &&
                     item2->minCount > weightedSubWorld->minCount)) {
                    weightedSubWorld = item2;
                    priority = item2->priority;
                }
            }
            weightedSubWorld->minCount--;
        } else {
            weightedSubWorld = WeightedRandom_Choose(list2, random);
        }
        if (weightedSubWorld != nullptr &&
            weightedSubWorld->subWorld != nullptr) {
            ApplySubworldToNode(site, *weightedSubWorld->subWorld,
                                weightedSubWorld->overridePower);
            weightedSubWorld->maxCount--;
            if (weightedSubWorld->maxCount <= 0) {
                weightedSubWorld->subWorld = nullptr;
            }
        }
    }
    auto &globalFeatures = m_world.globalFeatures2;
    std::vector<Site *> list2;
    list2.reserve(sites.size());
    for (auto &site : sites) {
        if (!site.tags.contains("NoGlobalFeatureSpawning")) {
            list2.push_back(&site);
        }
    }
    ShuffleSeeded(list2, random);
    for (size_t i = 0; i < list2.size() && i < globalFeatures.size(); ++i) {
        list2[i]->globalFeature = globalFeatures[i];
    }
}

static void
TagTopAndBottomSites(float mapHeight, std::vector<Site> &sites,
                     std::map<std::string, std::vector<Site *>> &sitesWithTags)
{
    auto minY = 5.0f;
    auto maxY = mapHeight - 5.0f;
    auto &surfaceSites = sitesWithTags["AtSurface"];
    auto &depthsSites = sitesWithTags["AtDepths"];
    for (auto &site : sites) {
        auto &bounds = site.polygon.Bounds();
        if (bounds.y < maxY && maxY < bounds.y + bounds.height) {
            site.tags.emplace("AtSurface");
            surfaceSites.emplace_back(&site);
        }
        if (bounds.y < minY && minY < bounds.y + bounds.height) {
            site.tags.emplace("AtDepths");
            depthsSites.emplace_back(&site);
        }
    }
}

static void
TagEdgeSites(float mapWidth, std::vector<Site> &sites,
             std::map<std::string, std::vector<Site *>> &sitesWithTags)
{
    auto minX = 5.0f;
    auto maxX = mapWidth - 5.0f;
    auto &edgeSites = sitesWithTags["AtEdge"];
    auto &leftSites = sitesWithTags["AtLeft"];
    auto &rightSites = sitesWithTags["AtRight"];
    for (auto &site : sites) {
        auto &bounds = site.polygon.Bounds();
        if (bounds.x < minX && minX < bounds.x + bounds.width) {
            site.tags.emplace("AtEdge");
            site.tags.emplace("AtLeft");
            edgeSites.emplace_back(&site);
            leftSites.emplace_back(&site);
        }
        if (bounds.x < maxX && maxX < bounds.x + bounds.width) {
            site.tags.emplace("AtEdge");
            site.tags.emplace("AtRight");
            edgeSites.emplace_back(&site);
            rightSites.emplace_back(&site);
        }
    }
}

void WorldGen::PropagateDistanceTags(std::vector<Site> &sites) const
{
    std::map<std::string, std::vector<Site *>> sitesWithTags;

    sites[0].tags.emplace("AtStart");
    sitesWithTags["AtStart"].emplace_back(&sites[0]);

    TagTopAndBottomSites(m_world.worldsize.y, sites, sitesWithTags);
    TagEdgeSites(m_world.worldsize.x, sites, sitesWithTags);

    const char *tags[] = {"AtSurface", "AtDepths", "AtEdge",
                          "AtStart",   "AtLeft",   "AtRight"};
    for (auto &tag : tags) {
        for (auto &site : sites) {
            site.visited = false;
        }
        auto &sitesWithTag = sitesWithTags[tag];
        std::queue<const Site *> neighbours;
        for (auto *site : sitesWithTag) {
            site->visited = true;
            site->minDistanceToTag.emplace(tag, 0);
            neighbours.push(site);
        }
        int distance = 0;
        const Site *site = nullptr;
        const Site *end = nullptr;
        while (!neighbours.empty()) {
            if (site == end) {
                distance++;
                end = neighbours.back();
            }
            site = neighbours.front();
            neighbours.pop();
            for (const auto *constNeighbour : site->neighbours) {
                Site *neighbour = const_cast<Site *>(constNeighbour);
                if (!neighbour->visited) {
                    neighbour->visited = true;
                    neighbour->minDistanceToTag.emplace(tag, distance);
                    neighbours.push(neighbour);
                }
            }
        }
    }
}

bool WorldGen::GenerateSeedPoints(KRandom &random, std::vector<Site> &sites)
{
    auto mapWidth = m_world.worldsize.x;
    auto mapHeight = m_world.worldsize.y;
    Polygon poly(Rect(0.0f, 0.0f, mapWidth, mapHeight));
    auto densityMin = GetDefaultData<float>("OverworldDensityMin");
    auto densityMax = GetDefaultData<float>("OverworldDensityMax");
    auto density = random.Next(densityMin, densityMax);
    auto avoidRadius = GetDefaultData<float>("OverworldAvoidRadius");
    auto startX = m_world.startingPositionHorizontal2.GetRandomValue(random);
    auto startY = m_world.startingPositionVertical2.GetRandomValue(random);
    std::vector<Vector2f> position;
    position.emplace_back(startX * mapWidth, startY * mapHeight);
    auto &sampler = GetDefaultData<std::string>("OverworldSampleBehaviour");
    auto enumSampler = sampler == "UniformHex" ? SampleBehaviour::UniformHex
                                               : SampleBehaviour::PoissonDisk;
    auto points = GetRandomPoints(poly, density, avoidRadius, position,
                                  enumSampler, false, random, false, true);
    auto subworldFile = std::ranges::find_if(
        m_world.subworldFiles2, [this](const WeightedSubworldName *x) {
            return x->name == m_world.startSubworldName;
        });
    auto subworld = m_settings.subworlds.find(m_world.startSubworldName);
    if (subworld == m_settings.subworlds.end()) {
        LogE("start subworld %s wrong.", m_world.startSubworldName.c_str());
        return false;
    }
    float overridePower = -1.0f;
    if (subworldFile != m_world.subworldFiles2.end() &&
        (*subworldFile)->overridePower > 0.0f) {
        overridePower = (*subworldFile)->overridePower;
    }
    int index = 1;
    sites.reserve(points.size() + 10); // reserve with dummy sites;
    sites.emplace_back(index++, position[0]);
    ApplySubworldToNode(sites.back(), subworld->second, overridePower);
    for (auto &point : points) {
        sites.emplace_back(index++, point);
    }
    return true;
}

void WorldGen::SetFeatureBiome(Site &site, KRandom &random,
                               const Feature *feature)
{
    bool flag = false;
    if (feature != nullptr) {
        auto itr = m_settings.features.find(feature->type);
        if (itr != m_settings.features.end()) {
            auto &feature2 = itr->second;
            auto &biomes = itr->second.biomeTags;
            site.tags.insert(feature2.tags.begin(), feature2.tags.end());
            site.tags.insert(feature->tags.begin(), feature->tags.end());
            for (auto &tag : feature->excludesTags) {
                site.tags.erase(tag);
            }
            site.tags.insert("Feature");
            site.tags.insert(feature->type);
            if (!feature2.forceBiome.empty()) {
                site.tags.insert(feature2.forceBiome);
                flag = true;
            }
            site.tags.insert(biomes.begin(), biomes.end());
        } else {
            LogE("unknown feature: %s", feature->type.c_str());
        }
    }
    if (!flag && !site.subworld->biomes.empty()) {
        std::vector<const WeightedBiome *> biomes;
        for (auto &biome : site.subworld->biomes) {
            biomes.push_back(&biome);
        }
        auto biome = WeightedRandom_Choose(biomes, random);
        site.tags.insert(biome->name);
        site.tags.insert(biome->tags.begin(), biome->tags.end());
        flag = true;
    }
    if (!flag) {
        site.tags.insert("UNKNOWN");
    }
}

size_t WorldGen::GenerateChildren(Site &site, KRandom &externRrandom, int seed,
                                  bool usePD)
{
    KRandom random(seed);
    auto &subworld = *site.subworld;
    int index = 1;
    site.children = std::make_unique<std::vector<Site>>();

    float density = subworld.density.GetRandomValue(random);
    int minPointCount = subworld.features.size() + subworld.extraBiomeChildren;
    if (site.globalFeature != nullptr) {
        minPointCount++;
    }
    if (minPointCount < subworld.minChildCount) {
        minPointCount = subworld.minChildCount;
    }
    int maxPointCount = std::numeric_limits<int>::max();
    if (subworld.singleChildCount) {
        minPointCount = 1;
        maxPointCount = 1;
    }
    auto &boundingArea = site.polygon;
    float avoidRadius = subworld.avoidRadius;
    std::vector<Vector2f> position;
    if (subworld.centralFeature.has_value()) {
        position.push_back(site.polygon.Centroid());
        site.children->emplace_back(index++, site.polygon.Centroid());
        auto &child = site.children->back();
        auto &feature = subworld.centralFeature.value();
        child.subworld = site.subworld;
        child.tags = site.tags;
        child.tags.insert("CenteralFeature");
        child.parent = &site;
        SetFeatureBiome(child, externRrandom, &feature);
    }
    std::vector<Vector2f> points;
    for (int i = 0; i < 10; ++i) {
        points = GetRandomPoints(boundingArea, density, avoidRadius, position,
                                 subworld.sampleBehaviour, true, random, true,
                                 subworld.doAvoidPoints);
        if (minPointCount <= (int)points.size()) {
            break;
        } else if (maxPointCount < (int)points.size()) {
            points.resize(maxPointCount);
            break;
        } else {
            density *= 0.8f;
            avoidRadius *= 0.8f;
        }
    }
    for (auto &sampler : subworld.samplers) {
        position.insert(position.end(), points.begin(), points.end());
        density = sampler.density.GetRandomValue(random);
        auto rndPoints = GetRandomPoints(
            boundingArea, density, sampler.avoidRadius, position,
            sampler.sampleBehaviour, true, random, true, sampler.doAvoidPoints);
        points.insert(points.end(), rndPoints.begin(), rndPoints.end());
    }
    if (points.size() > 200) {
        points.resize(200);
    }
    for (size_t i = 0; i < points.size(); ++i) {
        const Feature *feature = nullptr;
        if (i < subworld.features.size()) {
            feature = &subworld.features[i];
        }
        if (i == subworld.features.size()) {
            feature = site.globalFeature;
        }
        site.children->emplace_back(index++, points[i]);
        auto &child = site.children->back();
        child.subworld = site.subworld;
        child.tags = site.tags;
        child.parent = &site;
        SetFeatureBiome(child, externRrandom, feature);
    }
    Diagram diagram(site.polygon, *site.children);
    diagram.ComputeNode();
    if (!subworld.dontRelaxChildren) {
        if (usePD) {
            diagram.ComputeNodePD();
        } else {
            diagram.ComputeNode();
        }
    }
    return (int)site.children->size();
}

std::vector<Vector3i> WorldGen::GetGeysers(int globalWorldSeed) const
{
    const char *configs[] = {
        "steam",           "hot_steam",       "hot_water",
        "slush_water",     "filthy_water",    "slush_salt_water",
        "salt_water",      "small_volcano",   "big_volcano",
        "liquid_co2",      "hot_co2",         "hot_hydrogen",
        "hot_po2",         "slimy_po2",       "chlorine_gas",
        "methane",         "molten_copper",   "molten_iron",
        "molten_gold",     "molten_aluminum", "molten_cobalt",
        "oil_drip",        "liquid_sulfur",   "chlorine_gas_cool",
        "molten_tungsten", "molten_niobium",
    };
    std::vector<Vector3i> result;
    result.reserve(m_templates.size());
    int count = m_settings.IsSpaceOutEnabled() ? 23 : 20;
    for (auto &templt : m_templates) {
        const std::string &name = templt.container->name;
        Vector2<int> pos{templt.position};
        pos.y = (int)m_world.worldsize.y - pos.y;
        if (name == "geysers/generic") {
            int seed = globalWorldSeed + pos.x + (int)templt.position.y;
            int index = KRandom(seed).Next(0, count);
            if (!m_settings.IsSpaceOutEnabled() && index == 19) {
                index = 21;
            }
            result.emplace_back(pos.x, pos.y, index);
        } else if (name.starts_with("poi/oil/")) {
            result.emplace_back(pos.x, pos.y, std::size(configs) + 1);
        } else if (name.starts_with("expansion1::poi/warp/receiver")) {
            result.emplace_back(pos.x, pos.y, std::size(configs) + 2);
        } else if (name.starts_with("expansion1::poi/warp/sender")) {
            result.emplace_back(pos.x, pos.y, std::size(configs) + 3);
        } else if (name.starts_with("expansion1::poi/warp/teleporter")) {
            result.emplace_back(pos.x, pos.y, std::size(configs) + 4);
        } else if (name.starts_with("expansion1::poi/traits/cryopod")) {
            result.emplace_back(pos.x, pos.y, std::size(configs) + 5);
        } else if (!templt.container->otherEntities.empty()) {
            for (auto &item : templt.container->otherEntities) {
                if (item.id.find("GeyserGeneric_") == item.id.npos) {
                    continue;
                }
                std::string geyser = item.id.substr(14);
                for (int index = 0; index < (int)std::size(configs); ++index) {
                    if (geyser == configs[index]) {
                        pos.x += item.location_x;
                        pos.y -= item.location_y;
                        result.emplace_back(pos.x, pos.y, index);
                        break;
                    }
                }
            }
        }
    }
    return result;
}
