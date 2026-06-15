#pragma once

#include "DefaultSettings.hpp"
#include "WorldGenClasses.hpp"

class SettingsCache;

struct SubWorld {
    std::string name;
    PointSelectionMethod selectMethod{};
    MinMax density;
    float avoidRadius{};
    SampleBehaviour sampleBehaviour{};
    bool doAvoidPoints = true;
    bool dontRelaxChildren = false;
    MinMax blobSize;
    std::string nameKey;
    std::string descriptionKey;
    std::string utilityKey;
    std::string biomeNoise;
    std::string overrideNoise;
    std::string densityNoise;
    std::string borderOverride;
    int borderOverridePriority{};
    MinMax borderSizeOverride;
    Range temperatureRange{};
    std::optional<Feature> centralFeature;
    std::vector<Feature> features;
    Override overrides;
    std::vector<std::string> tags;
    int minChildCount = 2;
    bool singleChildCount{};
    int extraBiomeChildren{};
    std::vector<WeightedBiome> biomes;
    std::map<std::string, int> featureTemplates;
    std::vector<TemplateSpawnRules> subworldTemplateRules;
    int iterations{};
    float minEnergy{};
    ZoneType zoneType{};
    std::vector<SampleDescriber> samplers;
    float pdWeight = 1.0f;

    void EnforceTemplateSpawnRuleSelfConsistency();
};

struct WorldTrait {
    std::string filePath;
    std::string name;
    std::string description;
    std::string colorHex;
    std::string icon;
    std::vector<std::string> forbiddenDLCIds;
    std::vector<std::string> exclusiveWith;
    std::vector<std::string> exclusiveWithTags;
    std::vector<std::string> traitTags;
    MinMax startingBasePositionHorizontalMod;
    MinMax startingBasePositionVerticalMod;
    std::vector<WeightedSubworldName> additionalSubworldFiles;
    std::vector<AllowedCellsFilter> additionalUnknownCellFilters;
    std::vector<TemplateSpawnRules> additionalWorldTemplateRules;
    std::map<std::string, int> globalFeatureMods;
    std::vector<std::string> removeWorldTemplateRulesById;
    std::vector<ElementBandModifier> elementBandModifiers;

    // ignore lines

    int index{};

    bool IsValid(const struct World &world) const;
    bool ForbiddenSpaceOut() const;
};

struct World {
    std::string name;
    std::string description;
    std::vector<std::string> nameTables;
    std::vector<std::string> overrideName;
    std::string asteroidIcon;
    float iconScale = 1.0f;
    std::vector<std::string> worldTags;
    std::string dlcIdFrom;
    std::vector<std::string> requiredDlcIds;
    std::vector<std::string> forbiddenDlcIds;
    bool disableWorldTraits{};
    std::vector<TraitRule> worldTraitRules;
    float worldTraitScale = 1.0f;
    Skip skip{};
    bool moduleInterior{};
    WorldCategory category{};
    Vector2f worldsize;
    int hiddenY{};
    std::optional<DefaultSettings> defaultsOverrides;
    LayoutMethod layoutMethod{};
    std::vector<WeightedSubworldName> subworldFiles;
    std::vector<AllowedCellsFilter> unknownCellsAllowedSubworlds;
    std::vector<SubworldMixingRule> subworldMixingRules;
    std::vector<ModifyLayoutTagsRule> modifyLayoutTags;
    std::string startSubworldName;
    std::string startingBaseTemplate;
    MinMax startingBasePositionHorizontal = {0.5f, 0.5f};
    MinMax startingBasePositionVertical = {0.5f, 0.5f};
    std::vector<TemplateSpawnRules> worldTemplateRules;
    std::vector<std::string> seasons;
    std::vector<std::string> fixedTraits;
    bool adjacentTemporalTear{};

    // ignore lines

    LocationType locationType{};
    MinMax startingPositionHorizontal2;
    MinMax startingPositionVertical2;
    std::vector<const Feature *> globalFeatures2;
    std::vector<const WeightedSubworldName *> subworldFiles2;
    std::vector<const AllowedCellsFilter *> unknownCellsAllowedSubworlds2;
    std::vector<const TemplateSpawnRules *> worldTemplateRules2;
    std::vector<WeightedSubworldName> mixingSubworlds;

    void ApplayMixings(std::vector<struct MixingConfig *> &mixings);
    void ApplayTraits(const WorldTrait &traits, const SettingsCache &settings);
    void ClearMixingsAndTraits();
};
