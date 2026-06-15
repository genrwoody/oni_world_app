#include "App.hpp"

#include <stack>
#include <ranges>
#include <algorithm>

#include <clipper.hpp>

#include "config.h"
#include "Setting/SettingsCache.hpp"
#include "Utils/Log.hpp"
#include "WorldGen.hpp"

namespace
{

bool GetZonePolygon(Site &site, Polygon &polygon)
{
    ZoneType zoneType = site.subworld->zoneType;
    ClipperLib::Clipper clipper;
    std::stack<const Site *> stack;
    stack.push(&site);
    while (!stack.empty()) {
        auto top = stack.top();
        stack.pop();
        if (top->visited) {
            continue;
        }
        ClipperLib::Path path;
        for (Vector2f point : top->polygon.Vertices) {
            point *= 10000.0f;
            path.emplace_back((int)point.x, (int)point.y);
        }
        clipper.AddPath(path, ClipperLib::ptSubject, true);
        top->visited = true;
        for (auto neighbour : top->neighbours) {
            if (neighbour->visited) {
                continue;
            }
            if (neighbour->subworld->zoneType != zoneType) {
                continue;
            }
            stack.push(neighbour);
        }
    }
    ClipperLib::PolyTree polytree;
    ClipperLib::Paths paths;
    clipper.Execute(ClipperLib::ctUnion, polytree, ClipperLib::pftEvenOdd);
    ClipperLib::PolyTreeToPaths(polytree, paths);
    if (!paths.empty()) {
        auto &path = paths[0];
        for (auto &item : path) {
            Vector2f point{(float)item.X, (float)item.Y};
            polygon.Vertices.emplace_back(point * 0.0001f);
        }
    }
    return paths.size() > 1;
}

void SetResultWorldInfo(int seed, World *world, Site &site)
{
    Vector2i starting = {site.x, site.y};
    Vector2i worldSize = world->worldsize;
    starting.y = worldSize.y - starting.y;
    int worldType = (world->locationType == LocationType::StartWorld) ? 0 : 1;
    jsExchangeData(RT_Starting, worldType, (size_t)&starting);
    jsExchangeData(RT_WorldSize, seed, (size_t)&worldSize);
}

void SetResultTraits(const SettingsCache::WorldTraitArray &traits)
{
    std::vector<int> result;
    result.reserve(traits.size());
    for (auto *item : traits) {
        result.push_back(item->index - 1);
    }
    jsExchangeData(RT_Trait, (uint32_t)result.size(), (size_t)result.data());
}

void SetResultGeysers(int seed, const WorldGen &worldGen)
{
    auto geysers = worldGen.GetGeysers(seed);
    std::vector<int> result;
    result.reserve(geysers.size() * 3);
    for (auto &item : geysers) {
        result.insert(result.end(), {item.z, item.x, item.y}); // z is type
    }
    jsExchangeData(RT_Geyser, (uint32_t)result.size(), (size_t)result.data());
}

void SetResultPolygons(World *world, std::vector<Site> &sites)
{
    std::vector<int> result;
    std::ranges::for_each(sites, [](Site &site) { site.visited = false; });
    for (auto &item : sites) {
        if (item.visited) {
            continue;
        }
        Polygon polygon;
        bool hasHole = GetZonePolygon(item, polygon);
        result.push_back(hasHole ? 1 : 0);
        result.push_back((int)item.subworld->zoneType);
        result.push_back((int)polygon.Vertices.size());
        for (auto &vex : polygon.Vertices) {
            result.push_back(vex.x);
            result.push_back(world->worldsize.y - vex.y);
        }
    }
    jsExchangeData(RT_Polygon, (uint32_t)result.size(), (size_t)result.data());
}

void SerializeSite(const Site &site, std::vector<uint32_t> &data)
{
    data.push_back(site.idx);
    data.push_back(*(uint32_t *)&site.x);
    data.push_back(*(uint32_t *)&site.y);
    int count = (int)site.polygon.Vertices.size();
    if (count != 0) {
        data.push_back(count);
        for (auto &point : site.polygon.Vertices) {
            data.push_back(*(uint32_t *)&point.x);
            data.push_back(*(uint32_t *)&point.y);
        }
    }
    if (site.children && !site.children->empty()) {
        for (auto &child : *site.children) {
            data.push_back(child.idx);
            data.push_back(*(uint32_t *)&child.x);
            data.push_back(*(uint32_t *)&child.y);
            int count2 = (int)child.polygon.Vertices.size();
            if (count2 != 0) {
                data.push_back(count2);
                for (auto &point : child.polygon.Vertices) {
                    data.push_back(*(uint32_t *)&point.x);
                    data.push_back(*(uint32_t *)&point.y);
                }
            }
        }
    }
}

} // namespace

// for debug
void WriteToBinary(const std::vector<Site> &sites)
{
    static int index = 10;
    std::vector<uint32_t> data;
    for (auto &site : sites) {
        SerializeSite(site, data);
    }
    jsExchangeData(index++, (uint32_t)data.size(), (uint32_t)data.data());
}

// for debug
void WriteToBinary(const std::vector<Site *> &sites)
{
    static int index = 10;
    std::vector<uint32_t> data;
    for (auto *site : sites) {
        SerializeSite(*site, data);
    }
    jsExchangeData(index++, (uint32_t)data.size(), (uint32_t)data.data());
}

App::App()
    : m_settings{std::make_unique<SettingsCache>()}
    , m_random{std::make_unique<KRandom>(0)}
{
}

App::~App() {}

void App::Initialize(int seed)
{
    uint32_t count = SETTING_ASSET_FILESIZE;
    auto data = std::make_unique<char[]>(count);
    jsExchangeData(RT_Resource, count, (size_t)data.get());
    std::string_view content(data.get(), count);
    m_settings->LoadSettingsCache(content);
    m_random = std::make_unique<KRandom>(seed);
}

bool App::Generate(int type, int seed, uint64_t mix, int traitsFlag)
{
    if (!m_settings->CoordinateChanged(type, seed, mix)) {
        return false;
    }
    return GenerateInternal(traitsFlag);
}

bool App::Generate(const std::string &code, int traitsFlag)
{
    if (!m_settings->CoordinateChanged(code)) {
        return false;
    }
    return GenerateInternal(traitsFlag);
}

bool App::GenerateInternal(int traitsFlag)
{
    std::vector<World *> worlds;
    if (!m_settings->InitializeWorlds(worlds)) {
        return false;
    }
    if (traitsFlag != 0) { // roll seed for preset traits
        m_settings->SetSeedWithTraits(worlds, traitsFlag, *m_random);
    }
    int seed = m_settings->Seed();
    std::vector<SettingsCache::WorldTraitArray> allWorldTraits;
    allWorldTraits.reserve(worlds.size());
    for (size_t i = 0; i < worlds.size(); ++i) {
        auto world = worlds[i];
        world->ClearMixingsAndTraits();
        auto traits = m_settings->GetRandomTraits(*world, seed + i);
        for (auto trait : traits) {
            world->ApplayTraits(*trait, *m_settings);
        }
        allWorldTraits.emplace_back(traits);
    }
    m_settings->DoSubworldMixing(worlds);
    bool genWarpWorld = m_settings->IsMiniCluster();
    for (size_t i = 0; i < worlds.size(); ++i) {
        auto world = worlds[i];
        if (world->locationType == LocationType::Cluster) {
            continue;
        } else if (world->locationType == LocationType::StartWorld) {
            // go on;
        } else if (!world->startingBaseTemplate.contains("::bases/warpworld")) {
            continue; // other inner cluster
        } else if (!genWarpWorld) {
            continue;
        }
        WorldGen worldGen(*world, *m_settings, seed + i);
        std::vector<Site> sites;
        if (!worldGen.GenerateOverworld(sites)) {
            LogE("generate overworld failed.");
            return false;
        }
        SetResultWorldInfo(seed, world, sites[0]);
        SetResultTraits(allWorldTraits[i]);
        SetResultGeysers(seed + (int)worlds.size() - 1, worldGen);
        SetResultPolygons(world, sites);
    }
    return true;
}
