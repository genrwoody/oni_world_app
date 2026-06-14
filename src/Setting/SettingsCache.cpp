#include "SettingsCache.hpp"

#include <cstring>
#include <string_view>
#include <set>
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <regex>

#include <json/reader.h>
#include <miniz.h>

#include "JsonDeserializeGen.hpp"
#include "Utils/KRandom.hpp"
#include "Utils/Polygon.hpp"
#include "Utils/PointGenerator.hpp"
#include "Utils/SortHelper.hpp"

Variant SettingsCache::m_nil;

template<typename Ty>
inline bool string_view_to_number(const std::string_view& str, Ty& out)
{
    auto begin = str.data();
    auto end = str.data() + str.size();
    auto [ptr, err] = std::from_chars(begin, end, out);
    return err == std::errc{};
}

namespace Setting
{
template<>
bool deserialize(const Json::Value &json, std::map<Range, MinMax> &obj)
{
    for (auto itr = json.begin(); itr != json.end(); ++itr) {
        Range key;
        MinMax value;
        if (!Deserializer<Range>::deserialize(itr.name(), key) ||
            !Deserializer<MinMax>::deserialize(*itr, value)) {
            LogE("object std::map<Range, MinMax> parse failed.");
            return false;
        }
        obj[key] = value;
    }
    return true;
}
} // namespace Setting

template<typename T>
static bool LoadJsonFile(mz_zip_archive &zip, int index, T &result)
{
    size_t size;
    char *ptr = (char *)mz_zip_reader_extract_to_heap(&zip, index, &size, 0);
    if (!ptr) {
        LogE("can not open file %d.", index);
        return false;
    }
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value root;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader{builder.newCharReader()};
    auto ret = reader->parse(ptr, ptr + size, &root, &errs);
    mz_free(ptr);
    if (!ret) {
        LogE("parse %d failed, error: %s.", index, errs.c_str());
        return false;
    }
    if (!Setting::deserialize(root, result)) {
        LogE("deserialize failed, file: %d", index);
        return false;
    }
    return true;
}

// dlc/dlc2/templates/bases/ceresBase.json -> dlc2::bases/ceresBase
static std::string GenerateKey(const char *filename)
{
    const size_t MaxKeySize = 70;
    std::string key;
    key.reserve(MaxKeySize);
    unsigned offset = 0;
    if (strstr(filename, "dlc/expansion1") != nullptr) {
        key = "expansion1::";
        offset = 15;
    } else if (strstr(filename, "dlc/dlc2") != nullptr) {
        key = "dlc2::";
        offset = 9;
    } else if (strstr(filename, "dlc/dlc3") != nullptr) {
        key = "dlc3::";
        offset = 9;
    } else if (strstr(filename, "dlc/dlc4") != nullptr) {
        key = "dlc4::";
        offset = 9;
    }
    if (strstr(filename, "templates/") != nullptr) {
        offset += 10;
    } else {
        offset += 9; // worldgen/
    }
    key += filename + offset;
    key.resize(key.size() - 5);
    return key;
}

bool SettingsCache::LoadSettingsCache(const std::string_view &content)
{
    if (!defaults.data.empty()) {
        return false;
    }
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, content.data(), content.size(), 0)) {
        LogE("wrong content");
        return false;
    }
    size_t mixConfigsCount = 0;
    std::map<std::string, std::vector<std::string>> Asubworlds;
    auto count = mz_zip_reader_get_num_files(&zip);
    for (unsigned i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
            LogE("Failed to get file stat for file index: %d", i);
            continue;
        }
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            continue;
        }
        if (strstr(stat.m_filename, "Asubworlds.json") != nullptr) {
            LoadJsonFile(zip, i, Asubworlds);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/defaults.json") != nullptr) {
            LoadJsonFile(zip, i, defaults);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/temperatures.json") != nullptr) {
            LoadJsonFile(zip, i, temperatures);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/clusters/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            LoadJsonFile(zip, i, clusters[key]);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/features/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            LoadJsonFile(zip, i, features[key]);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/subworldMixing/") != nullptr) {
            mixConfigsCount++;
            std::string key = GenerateKey(stat.m_filename);
            LoadJsonFile(zip, i, subworldMixing[key]);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/subworlds/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            SubWorld &subworld = subworlds[key];
            LoadJsonFile(zip, i, subworld);
            subworld.name = key;
            subworld.EnforceTemplateSpawnRuleSelfConsistency();
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/traits/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            WorldTrait &trait = traits[key];
            LoadJsonFile(zip, i, trait);
            trait.filePath = key;
            for (auto &item : trait.globalFeatureMods) {
                traitFeatures.emplace(item.first);
            }
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/worldMixing/") != nullptr) {
            mixConfigsCount++;
            std::string key = GenerateKey(stat.m_filename);
            LoadJsonFile(zip, i, worldMixing[key]);
            continue;
        }
        if (strstr(stat.m_filename, "worldgen/worlds/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            LoadJsonFile(zip, i, worlds[key]);
            continue;
        }
        if (strstr(stat.m_filename, "templates/") != nullptr) {
            std::string key = GenerateKey(stat.m_filename);
            TemplateContainer &templt = templates[key];
            LoadJsonFile(zip, i, templt);
            templt.name = key;
            continue;
        }
        LogE("unknown file: %s", stat.m_filename);
    }
    mz_zip_end(&zip);
    for (auto &pair : Asubworlds) {
        auto &list = orderedSubworlds[pair.first];
        list.reserve(pair.second.size());
        for (auto &name : pair.second) {
            auto itr = subworlds.find(name);
            if (itr != subworlds.end()) {
                list.push_back(&itr->second);
            }
        }
    }
    mixConfigs.reserve(mixConfigsCount + 3);
    mixConfigs = {
        {"DLC2_ID", MixingType::Dlc},
        {"dlc2::subworldMixing/IceCavesMixingSettings"},
        {"dlc2::subworldMixing/CarrotQuarryMixingSettings"},
        {"dlc2::subworldMixing/SugarWoodsMixingSettings"},
        {"dlc2::worldMixing/CeresMixingSettings", MixingType::World},
        {"DLC3_ID", MixingType::Dlc},
        {"DLC4_ID", MixingType::Dlc},
        {"dlc4::subworldMixing/GardenMixingSettings"},
        {"dlc4::subworldMixing/RaptorMixingSettings"},
        {"dlc4::subworldMixing/WetlandsMixingSettings"},
        {"dlc4::worldMixing/PrehistoricMixingSettings", MixingType::World},
    };
    return true;
}

uint32_t SettingsCache::Base36ToBinary(const std::string_view &input)
{
    uint8_t dict[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  // 0-9
                      0,  0,  0,  0,  0,  0,  0,              // 3A-40
                      10, 11, 12, 13, 14, 15, 16, 17, 18, 19, // A-J
                      20, 21, 22, 23, 24, 25, 26, 27, 28, 29, // K-T
                      30, 31, 32, 33, 34, 35};                // U-Z
    uint32_t result = 0;
    for (auto itr = input.rbegin(); itr != input.rend(); ++itr) {
        result *= 36;
        result += dict[*itr - '0'];
    }
    return result;
}

std::string SettingsCache::BinaryToBase36(uint32_t input)
{
    if (input == 0) {
        return "0";
    }
    char dict[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string result;
    while (input > 0) {
        result.push_back(dict[input % 36]);
        input /= 36;
    }
    return result;
}

bool SettingsCache::CoordinateChanged(int type, int seed, int mix)
{
    const char *clusterPrefix[] = {
        "SNDST-A",  "OCAN-A",    "S-FRZ",     "LUSH-A",    "FRST-A",
        "VOLCA",    "BAD-A",     "HTFST-A",   "OASIS-A",   "CER-A",
        "CERS-A",   "PRE-A",     "PRES-A",    "V-SNDST-C", "V-OCAN-C",
        "V-SWMP-C", "V-SFRZ-C",  "V-LUSH-C",  "V-FRST-C",  "V-VOLCA-C",
        "V-BAD-C",  "V-HTFST-C", "V-OASIS-C", "V-CER-C",   "V-CERS-C",
        "V-PRE-C",  "V-PRES-C",  "SNDST-C",   "PRE-C",     "CER-C",
        "FRST-C",   "SWMP-C",    "M-SWMP-C",  "M-BAD-C",   "M-FRZ-C",
        "M-FLIP-C", "M-RAD-C",   "M-CERS-C"};
    if (type < 0 || (int)std::size(clusterPrefix) <= type) {
        return false;
    }
    m_seed = seed;
    ParseAndApplyMixingSettingsCode((uint32_t)mix);
    return InitializeCluster(clusterPrefix[type]);
}

bool SettingsCache::CoordinateChanged(const std::string &text)
{
    constexpr size_t MaxGroupCount = 6;
    std::regex regex("(.+)-(\\d+)-(.+)-(.+)-(.+)");
    std::smatch match;
    std::inplace_vector<std::string_view, MaxGroupCount> codes;
    if (std::regex_match(text.begin(), text.end(), match, regex)) {
        for (auto &item : match) {
            codes.push_back(std::string_view{item.first, item.second});
        }
    }
    if (codes.size() != MaxGroupCount) {
        LogE("parse seed code %s failed.", text.c_str());
        return false;
    }
    if (!string_view_to_number(codes[2], m_seed)) {
        LogE("can not convert seed code to number.");
        return false;
    }
    uint32_t mix = Base36ToBinary(codes[5]);
    ParseAndApplyMixingSettingsCode(mix);
    return InitializeCluster(codes[1]);
}

bool SettingsCache::InitializeCluster(const std::string_view &coord)
{
    if (m_seed > 0x7FFF0000) { // avoid overflow.
        LogE("the seed is too large.");
        return false;
    }
    m_cluster = nullptr;
    for (auto &pair : clusters) {
        if (pair.second.coordinatePrefix == coord) {
            m_cluster = &pair.second;
            break;
        }
    }
    if (m_cluster == nullptr) {
        LogE("cluster %*s was wrong.", (int)coord.size(), coord.data());
        return false;
    }
    m_dlcState = 0;
    for (auto &id : m_cluster->requiredDlcIds) {
        if (id == "EXPANSION1_ID") {
            m_dlcState |= 1;
        } else if (id == "DLC2_ID") {
            m_dlcState |= 2;
        } else if (id == "DLC3_ID") {
            m_dlcState |= 4;
        } else if (id == "DLC4_ID") {
            m_dlcState |= 8;
        }
    }
    if (coord.contains("CER")) {
        mixConfigs[0].level = mixConfigs[1].level = mixConfigs[2].level =
            mixConfigs[3].level = mixConfigs[4].level = MixingLevel::Disabled;
    } else if (coord.contains("PRE")) {
        mixConfigs[6].level = mixConfigs[7].level = mixConfigs[8].level =
            mixConfigs[9].level = mixConfigs[10].level = MixingLevel::Disabled;
    }
    return true;
}

bool SettingsCache::InitializeWorlds(std::vector<World *> &chosenWorlds)
{
    chosenWorlds.clear();
    chosenWorlds.reserve(m_cluster->worldPlacements.size());
    for (auto &worldPlacement : m_cluster->worldPlacements) {
        auto itr = worlds.find(worldPlacement.world);
        if (itr == worlds.end()) {
            LogE("world %s was wrong.", worldPlacement.world.c_str());
            return false;
        }
        itr->second.locationType = worldPlacement.locationType;
        chosenWorlds.push_back(&itr->second);
    }
    if (worlds.size() == 1) {
        chosenWorlds[0]->locationType = LocationType::StartWorld;
    }
    return true;
}

void SettingsCache::ParseAndApplyMixingSettingsCode(uint32_t num)
{
    for (auto itr = mixConfigs.rbegin(); itr != mixConfigs.rend(); ++itr) {
        itr->level = (MixingLevel)(num % 5);
        itr->minCount = itr->level == MixingLevel::GuranteeMixing ? 1 : 0;
        itr->maxCount = 3;
        num /= 5;
    }
}

SettingsCache::WorldTraitArray
SettingsCache::GetRandomTraits(const World &world, int seed) const
{
    if (seed == 0 || world.disableWorldTraits ||
        world.worldTraitRules.empty()) {
        return {};
    }
    KRandom kRandom(seed);
    std::vector<const WorldTrait *> total;
    total.reserve(traits.size());
    for (auto &pair : traits) {
        if (pair.first[0] == 't') {
            if (IsSpaceOutEnabled() && pair.second.ForbiddenSpaceOut()) {
                continue;
            }
            total.push_back(&pair.second);
        }
    }
    for (auto &pair : traits) {
        if (pair.first[0] == 'e' && IsSpaceOutEnabled()) {
            total.push_back(&pair.second);
        }
    }
    WorldTraitArray result;
    std::set<std::string> except;
    std::vector<const WorldTrait *> filtered;
    filtered.reserve(total.size());
    for (auto &rule : world.worldTraitRules) {
        for (auto &specificTrait : rule.specificTraits) {
            auto itr = traits.find(specificTrait);
            if (itr != traits.end()) {
                result.push_back(&itr->second);
            }
        }
        filtered.clear();
        for (auto &trait : total) {
            if (!rule.requiredTags.empty() &&
                !std::ranges::all_of(
                    trait->traitTags, [&rule](const std::string &elem) {
                        return std::ranges::contains(rule.requiredTags, elem);
                    })) {
                continue;
            }
            if (!rule.forbiddenTags.empty() &&
                std::ranges::any_of(
                    trait->traitTags, [&rule](const std::string &elem) {
                        return std::ranges::contains(rule.forbiddenTags, elem);
                    })) {
                continue;
            }
            if (std::ranges::contains(rule.forbiddenTraits, trait->filePath)) {
                continue;
            }
            if (trait->IsValid(world)) {
                filtered.push_back(trait);
            }
        }
        int num = kRandom.Next(rule.min, std::max(rule.min, rule.max + 1));
        int count = (int)result.size();
        while ((int)result.size() < count + num && filtered.size() > 0) {
            int index = kRandom.Next((int)filtered.size());
            auto *worldTrait = filtered[index];
            filtered.erase(filtered.begin() + index);
            bool flag = false;
            for (auto &exclusiveId : worldTrait->exclusiveWith) {
                if (std::ranges::contains(
                        result, exclusiveId,
                        [](const WorldTrait *trait) -> const std::string & {
                            return trait->filePath;
                        })) {
                    flag = true;
                    break;
                }
            }
            for (auto &exclusiveWithTag : worldTrait->exclusiveWithTags) {
                if (std::ranges::contains(except, exclusiveWithTag)) {
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                result.push_back(worldTrait);
                for (auto &exclusiveWithTag2 : worldTrait->exclusiveWithTags) {
                    except.emplace(exclusiveWithTag2);
                }
                auto itr = std::remove(total.begin(), total.end(), worldTrait);
                total.erase(itr, total.end());
            }
        }
        if ((int)result.size() != count + num) {
            LogI("TraitRule on %s tried to generate %d but only generated %d",
                 world.name.c_str(), num, (int)result.size() - count);
        }
    }
    return result;
}

void SettingsCache::SetSeedWithTraits(const std::vector<World *> &asteroids,
                                      int traitsFlag, KRandom &random)
{
    constexpr int MaxTryTimes = 1000;
    std::inplace_vector<const WorldTrait *, 4> presets;
    int index = 0;
    for (auto &pair : traits) {
        if ((traitsFlag >> index & 1) == 1) {
            presets.push_back(&pair.second);
        }
        ++index;
    }
    if (presets.empty()) {
        m_seed = random.Next();
        return;
    }
    index = 0;
    World *world = asteroids[index];
    for (size_t i = 0; i < asteroids.size(); ++i) {
        world = asteroids[i];
        if (world->locationType == LocationType::StartWorld) {
            index = i;
            break;
        }
    }
    size_t maxCount = 0;
    int maxCountSeed = 0;
    for (int i = 0; i < MaxTryTimes; ++i) {
        int seed = random.Next();
        auto worldTraits = GetRandomTraits(*world, seed + index);
        size_t count = 0;
        for (auto *preset : presets) {
            if (std::ranges::contains(worldTraits, preset)) {
                ++count;
            }
        }
        if (count == presets.size()) {
            m_seed = seed;
            return;
        } else if (maxCount < count) {
            maxCount = count;
            maxCountSeed = seed;
        }
    }
    m_seed = maxCountSeed;
    LogI("can not find seed for preset traits");
}

void SettingsCache::DoSubworldMixing(std::vector<World *> asteroids)
{
    std::vector<MixingConfig *> filtered;
    filtered.reserve(mixConfigs.size());
    for (auto &config : mixConfigs) {
        if (config.level == MixingLevel::Disabled ||
            config.type != MixingType::Subworld) {
            continue;
        }
        auto itr = subworldMixing.find(config.path);
        if (itr == subworldMixing.end()) {
            continue;
        }
        bool forbidden = false;
        for (auto &tag : itr->second.forbiddenClusterTags) {
            auto find = std::ranges::find(m_cluster->clusterTags, tag);
            if (find != m_cluster->clusterTags.end()) {
                forbidden = true;
                break;
            }
        }
        if (!forbidden) {
            config.setting = &(itr->second);
            filtered.push_back(&config);
        }
    }
    KRandom random(m_seed);
    ShuffleSeeded(asteroids, random);
    ArraySortHelper::Sort(
        asteroids, 0, (int)asteroids.size(), [](World *a, World *b) {
            const int dict[] = {3, 1, 2};
            return dict[(int)a->locationType] < dict[(int)b->locationType];
        });
    for (auto world : asteroids) {
        ShuffleSeeded(filtered, random);
        world->ApplayMixings(filtered);
    }
}
