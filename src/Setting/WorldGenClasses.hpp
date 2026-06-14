#pragma once

#include "DefaultSettings.hpp"
#include "Utils/Vector2f.hpp"

struct Feature {
    std::string type;
    std::vector<std::string> tags;
    std::vector<std::string> excludesTags;

    // ignore lines

    bool operator<(const Feature &rhs) const { return type < rhs.type; }
};

struct WeightedBiome {
    std::string name;
    float weight{};
    std::vector<std::string> tags;
};

struct AllowedCellsFilter {
    TagCommand tagcommand{};
    std::string tag;
    int minDistance{};
    int maxDistance{};
    Command command = Command::Replace;
    std::vector<Range> temperatureRanges;
    std::vector<ZoneType> zoneTypes;
    std::vector<std::string> subworldNames;
    int sortOrder{};
    bool ignoreIfMissingTag{};

    // ignore lines

    bool hasBackup{false};
    std::vector<std::string> subworldNames2;

    bool operator<(const AllowedCellsFilter &rhs) const
    {
        return sortOrder < rhs.sortOrder;
    }

    void Backup()
    {
        if (!hasBackup) {
            if (subworldNames2.empty()) {
                subworldNames2 = subworldNames;
            }
            hasBackup = true;
        }
    }

    void Restore()
    {
        if (hasBackup) {
            subworldNames = subworldNames2;
            hasBackup = false;
        }
    }
};

struct TemplateSpawnRules {
    std::string ruleId;
    std::vector<std::string> names;
    ListRule listRule{};
    int someCount{};
    int moreCount{};
    Vector2f range;
    int times = 1;
    float priority{};
    bool allowDuplicates{};
    bool allowExtremeTemperatureOverlap{};
    bool allowNearStart{};
    bool useRelaxedFiltering{};
    Vector2f overrideOffset;
    Vector2f overridePlacement = {-1.0f, -1.0f};
    std::vector<AllowedCellsFilter> allowedCellsFilter;
};

struct WeightedSubworldName {
    std::string name;
    std::string overrideName;
    float overridePower{};
    float weight = 1.0f;
    int minCount{};
    int maxCount = INT_MAX;
    int priority{};
};

struct WorldMixing {
    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;
    std::vector<TemplateSpawnRules> additionalWorldTemplateRules;
    std::vector<AllowedCellsFilter> additionalUnknownCellFilters;
    std::vector<WeightedSubworldName> additionalSubworldFiles;
    std::vector<std::string> additionalSeasons;
};

struct WorldMixingSettings {
    std::string name;
    std::string description;
    std::string icon;
    std::vector<std::string> forbiddenClusterTags;
    std::string world;
};

struct TraitRule {
    int min = 0;
    int max = 0;
    std::vector<std::string> requiredTags;
    std::vector<std::string> specificTraits;
    std::vector<std::string> forbiddenTags;
    std::vector<std::string> forbiddenTraits;
};

struct SubworldMixingRule {
    std::string name;
    int minCount{};
    int maxCount = INT_MAX;
    std::vector<std::string> forbiddenTags;
    std::vector<std::string> requiredTags;
};

struct ElementBandModifier {
    std::string element;
    float massMultiplier = 1.0f;
    float bandMultiplier = 1.0f;
};

struct MobReference {
    std::string type;
    MinMax count;
};

struct ElementChoiceGroup {
    Selection selectionMethod{};
    std::vector<WeightedSimHash> choices;
};

struct FeatureSettings {
    Shape shape{};
    std::vector<int> borders;
    MinMax blobSize;
    std::string forceBiome;
    std::vector<std::string> biomeTags;
    std::vector<MobReference> internalMobs;
    std::vector<std::string> tags;
    std::map<std::string, ElementChoiceGroup> ElementChoiceGroups;
    //
    std::vector<std::string> excludeTags;
};

struct SubworldMixingSettings {
    std::string name;
    std::string description;
    std::string icon;
    std::vector<std::string> forbiddenClusterTags;
    WeightedSubworldName subworld;
    std::vector<std::string> mixingTags;
    std::vector<TemplateSpawnRules> additionalWorldTemplateRules;
};

struct ModifyLayoutTagsRule {
    std::vector<std::string> addTags;
    std::vector<std::string> removeTags;
    std::vector<AllowedCellsFilter> allowedCellsFilter;
};
