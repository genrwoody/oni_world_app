#pragma once

#include <string_view>

#include "Utils/Log.hpp"
#include "Utils/InplaceVector.hpp"
#include "ClusterLayout.hpp"
#include "TemplateContainer.hpp"

enum class MixingLevel : uint8_t {
    Disabled,
    Enabled,
    TryMixing = Enabled,
    GuranteeMixing
};

enum class MixingType : uint8_t { Dlc, World, Subworld };

struct MixingConfig {
    std::string path;
    MixingType type{MixingType::Subworld};
    MixingLevel level{};
    int minCount = 0;
    int maxCount = 3;
    const SubworldMixingSettings *setting = nullptr;

    bool operator<(const MixingConfig &other)
    {
        if (other.minCount != minCount) {
            return other.minCount < minCount;
        }
        return other.maxCount < maxCount;
    }
};

class SettingsCache
{
public:
    using WorldTraitArray = std::inplace_vector<const WorldTrait *, 4>;

public:
    // ComposableDictionary<std::vector<WeightedSimHash>> borders;
    DefaultSettings defaults;
    // LevelLayerSettings layers;
    // MobSettings mobs;
    // ComposableDictionary<River> rivers;
    // ComposableDictionary<Room> rooms;
    std::map<Range, Temperature> temperatures;

    // std::map<std::string, std::vector<ElementGradient>> biomes;
    std::map<std::string, ClusterLayout> clusters;
    std::map<std::string, FeatureSettings> features;
    // std::map<std::string, NoiseTree> noise;
    // std::map<std::string, WorldTrait> storytraits;
    std::map<std::string, SubworldMixingSettings> subworldMixing;
    std::map<std::string, SubWorld> subworlds;
    std::map<std::string, std::vector<SubWorld *>> orderedSubworlds;
    std::map<std::string, WorldTrait> traits;
    std::map<std::string, WorldMixingSettings> worldMixing;
    std::map<std::string, World> worlds;
    //std::map<std::string, DlcMixingSetting> dlcMixings;
    std::map<std::string, TemplateContainer> templates;

    std::set<Feature> traitFeatures;
    std::vector<MixingConfig> mixConfigs;

private:
    static Variant m_nil;
    int m_seed = 0;
    int m_dlcState = 0;
    ClusterLayout *m_cluster = nullptr;

public:
    int Seed() const { return m_seed; }
    bool LoadSettingsCache(const std::string_view &content);
    bool CoordinateChanged(const std::string &text);
    bool CoordinateChanged(int type, int seed, int mix);
    bool InitializeWorlds(std::vector<World *> &chosenWorlds);
    bool IsSpaceOutEnabled() const { return (m_dlcState & 1) == 1; }
    bool IsMiniCluster() const { return m_cluster->IsMiniCluster(); }
    WorldTraitArray GetRandomTraits(const World &world, int seed) const;
    void SetSeedWithTraits(const std::vector<World *> &worlds, int traitsFlag,
                           KRandom &random);
    void DoSubworldMixing(std::vector<World *> worlds);
    static uint32_t Base36ToBinary(const std::string_view &input);
    static std::string BinaryToBase36(uint32_t input);

    template<typename T>
    const T &GetDefaultData(const World &world, const std::string &key) const
    {
        if (world.defaultsOverrides.has_value()) {
            auto itr = world.defaultsOverrides.value().data.find(key);
            if (itr != world.defaultsOverrides.value().data.end()) {
                return std::get<T>(itr->second);
            }
        }
        auto itr = defaults.data.find(key);
        if (itr == defaults.data.end()) {
            LogE("Can not get value for '%s'.", key.c_str());
            return std::get<T>(m_nil);
        }
        return std::get<T>(itr->second);
    }

private:
    bool InitializeCluster(const std::string_view &coord);
    void ParseAndApplyMixingSettingsCode(uint32_t num);
};
