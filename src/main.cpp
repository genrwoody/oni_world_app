#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <stack>
#include <ranges>
#include <algorithm>

#include <clipper.hpp>

#include "config.h"
#include "Setting/SettingsCache.hpp"
#include "WorldGen.hpp"

// I defined only one function for exchanging data between c++ and js,
// it get resource from js and set result to js.
extern "C" void jsExchangeData(uint32_t type, uint32_t count, size_t data);

enum ResultType {
    RT_Starting,
    RT_Trait,
    RT_Geyser,
    RT_Polygon,
    RT_WorldSize,
    RT_Resource
};

// for debug
void WriteToBinary(const std::vector<Site> &sites)
{
    static int index = 10;
    std::vector<uint32_t> data;
    for (auto &site : sites) {
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
    jsExchangeData(index++, (uint32_t)data.size(), (uint32_t)data.data());
}

class App
{
private:
    SettingsCache m_settings;
    KRandom m_random{0};

    App() = default;

public:
    static App *Instance()
    {
        static App inst;
        return &inst;
    }

    void Initialize(int seed)
    {
        uint32_t count = SETTING_ASSET_FILESIZE;
        auto data = std::make_unique<char[]>(count);
        jsExchangeData(RT_Resource, count, (size_t)data.get());
        std::string_view content(data.get(), count);
        m_settings.LoadSettingsCache(content);
        m_random = KRandom(seed);
    }

    bool Generate(const std::string &code, int traits);
    void SetResultWorldInfo(int seed, World *world, std::vector<Site> &sites);
    void SetResultTraits(const SettingsCache::WorldTraitArray &traits);
    void SetResultGeysers(int seed, const WorldGen &worldGen);
    void SetResultPolygons(World *world, std::vector<Site> &sites);
    // union sites with the same zone type. if result has hole return true.
    static bool GetZonePolygon(Site &site, Polygon &polygon);
};

bool App::Generate(const std::string &code, int traitsFlag)
{
    if (!m_settings.CoordinateChanged(code, m_settings)) {
        LogE("parse seed code %s failed.", code.c_str());
        return false;
    }
    std::vector<World *> worlds;
    for (auto &worldPlacement : m_settings.cluster->worldPlacements) {
        auto itr = m_settings.worlds.find(worldPlacement.world);
        if (itr == m_settings.worlds.end()) {
            LogE("world %s was wrong.", worldPlacement.world.c_str());
            return false;
        }
        itr->second.locationType = worldPlacement.locationType;
        worlds.push_back(&itr->second);
    }
    if (worlds.size() == 1) {
        worlds[0]->locationType = LocationType::StartWorld;
    }
    if (traitsFlag != 0) { // roll seed for preset traits
        m_settings.SetSeedWithTraits(worlds, traitsFlag, m_random);
    }
    int seed = m_settings.Seed();
    std::vector<SettingsCache::WorldTraitArray> allWorldTraits;
    for (size_t i = 0; i < worlds.size(); ++i) {
        auto world = worlds[i];
        world->ClearMixingsAndTraits();
        auto traits = m_settings.GetRandomTraits(*world, seed + i);
        for (auto trait : traits) {
            if (trait) {
                world->ApplayTraits(*trait, m_settings);
            }
        }
        allWorldTraits.emplace_back(traits);
    }
    m_settings.DoSubworldMixing(worlds);
    bool genWarpWorld = code.find("M-") == 0;
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
        WorldGen worldGen(*world, m_settings, seed + i);
        std::vector<Site> sites;
        if (!worldGen.GenerateOverworld(sites)) {
            LogE("generate overworld failed.");
            return false;
        }
        SetResultWorldInfo(seed, world, sites);
        SetResultTraits(allWorldTraits[i]);
        SetResultGeysers(seed, worldGen);
        SetResultPolygons(world, sites);
    }
    return true;
}

void App::SetResultWorldInfo(int seed, World *world, std::vector<Site> &sites)
{
    Vector2i starting = {sites[0].x, sites[0].y};
    Vector2i worldSize = world->worldsize;
    starting.y = worldSize.y - starting.y;
    int worldType = (world->locationType == LocationType::StartWorld) ? 0 : 1;
    jsExchangeData(RT_Starting, worldType, (size_t)&starting);
    jsExchangeData(RT_WorldSize, seed, (size_t)&worldSize);
}

void App::SetResultTraits(const SettingsCache::WorldTraitArray &traits)
{
    std::vector<int> result;
    result.reserve(traits.size());
    for (auto *item : traits) {
        uint32_t index = 0;
        for (auto &pair : m_settings.traits) {
            if (item == &pair.second) {
                result.push_back(index);
                break;
            } else {
                index++;
            }
        }
    }
    jsExchangeData(RT_Trait, (uint32_t)result.size(), (size_t)result.data());
}

void App::SetResultGeysers(int seed, const WorldGen &worldGen)
{
    seed += (int)m_settings.cluster->worldPlacements.size() - 1;
    auto geysers = worldGen.GetGeysers(seed);
    std::vector<int> result;
    result.reserve(geysers.size() * 3);
    for (auto &item : geysers) {
        result.insert(result.end(), {item.z, item.x, item.y}); // z is type
    }
    jsExchangeData(RT_Geyser, (uint32_t)result.size(), (size_t)result.data());
}

void App::SetResultPolygons(World *world, std::vector<Site> &sites)
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

bool App::GetZonePolygon(Site &site, Polygon &polygon)
{
    ZoneType zoneType = site.subworld->zoneType;
    ClipperLib::Clipper clipper;
    std::stack<Site *> stack;
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

extern "C" void EMSCRIPTEN_KEEPALIVE app_init(int seed)
{
    App::Instance()->Initialize(seed);
}

extern "C" bool EMSCRIPTEN_KEEPALIVE app_generate(int type, int seed, int mix)
{
    const char *worlds[] = {
        "SNDST-A-",  "OCAN-A-",    "S-FRZ-",     "LUSH-A-",    "FRST-A-",
        "VOLCA-",    "BAD-A-",     "HTFST-A-",   "OASIS-A-",   "CER-A-",
        "CERS-A-",   "PRE-A-",     "PRES-A-",    "V-SNDST-C-", "V-OCAN-C-",
        "V-SWMP-C-", "V-SFRZ-C-",  "V-LUSH-C-",  "V-FRST-C-",  "V-VOLCA-C-",
        "V-BAD-C-",  "V-HTFST-C-", "V-OASIS-C-", "V-CER-C-",   "V-CERS-C-",
        "V-PRE-C-",  "V-PRES-C-",  "SNDST-C-",   "PRE-C-",     "CER-C-",
        "FRST-C-",   "SWMP-C-",    "M-SWMP-C-",  "M-BAD-C-",   "M-FRZ-C-",
        "M-FLIP-C-", "M-RAD-C-",   "M-CERS-C-"};
    if (type < 0 || (int)std::size(worlds) <= type) {
        return false;
    }
    std::string code = worlds[type];
    int traits = 0;
    if (seed < 0) {
        traits = -seed;
        seed = 0;
    }
    code += std::to_string(seed);
    code += "-0-D3-";
    code += SettingsCache::BinaryToBase36(mix);
    LogI("generate with code: %s", code.c_str());
    return App::Instance()->Generate(code, traits);
}

#ifndef EMSCRIPTEN

int main()
{
    int type, seed, mixing;
    app_init(time(nullptr));
    while (true) {
        std::cout << "input type, seed, mixing: ";
        std::cin >> type >> seed >> mixing;
        if (seed == 0) {
            break;
        }
        if (!app_generate(type, seed, mixing)) {
            LogE("generate failed.");
        }
    }
    return 0;
}

void jsExchangeData(uint32_t type, uint32_t count, size_t data)
{
    const char *geysers[] = {
        "低温蒸汽喷孔", "蒸汽喷孔",     "清水泉",       "低温泥浆泉",
        "污水泉",       "低温盐泥泉",   "盐水泉",       "小型火山",
        "火山",         "二氧化碳泉",   "二氧化碳喷孔", "氢气喷孔",
        "高温污氧喷孔", "含菌污氧喷孔", "氯气喷孔",     "天然气喷孔",
        "铜火山",       "铁火山",       "金火山",       "铝火山",
        "钴火山",       "渗油裂缝",     "液硫泉",       "冷氯喷孔",
        "钨火山",       "铌火山",       "打印舱",       "储油石",
        "输出端",       "输入端",       "传送器",       "低温箱"};
    const char *traits[] = {
        "坠毁的卫星群", "冰封之友",   "不规则的原油区",   "繁茂核心",
        "金属洞穴",     "放射性地壳", "地下海洋",         "火山活跃",
        "大型石块",     "中型石块",   "混合型石块",       "小型石块",
        "被圈闭的原油", "冰冻核心",   "活跃性地质",       "晶洞",
        "休眠性地质",   "大型冰川",   "不规则的原油区",   "岩浆通道",
        "金属贫瘠",     "金属富足",   "备选的打印舱位置", "粘液菌团",
        "地下海洋",     "火山活跃"};
    switch (type) {
    default:
        break;
    case RT_Starting:
        break;
    case RT_Trait: {
        auto ptr = (uint32_t *)data;
        auto end = ptr + count;
        while (ptr < end) {
            auto index = *ptr++;
            LogI("%s", traits[index]);
        }
        break;
    }
    case RT_Geyser: {
        auto ptr = (uint32_t *)data;
        auto end = ptr + count;
        while (ptr < end) {
            auto index = *ptr++;
            auto x = *ptr++;
            auto y = *ptr++;
            LogI("%s: %d, %d", geysers[index], x, y);
        }
        break;
    }
    case RT_Polygon:
        break;
    case RT_Resource: {
        auto ptr = (char *)data;
        *ptr = 'E';
        std::ifstream fstm(SETTING_ASSET_FILEPATH, std::ios::binary);
        if (fstm.is_open()) {
            auto size = fstm.seekg(0, std::ios::end).tellg();
            if (size == count) {
                fstm.seekg(0).read(ptr, count);
            } else {
                LogE("wrong count.");
            }
        } else {
            LogE("can not open file.");
        }
        break;
    }
    }
}

#endif
