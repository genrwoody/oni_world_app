#pragma once

#include "SampleDescriber.hpp"

struct BaseLocation {
    int left{};
    int right{};
    int top{};
    int bottom{};
};

struct StartingWorldElementSetting {
    std::string element;
    float amount{};
};

// for defaults.json
struct DefaultSettings {
    BaseLocation baseData;
    std::map<std::string, Variant> data;
    std::vector<std::string> defaultMoveTags;
    std::vector<std::string> overworldAddTags;
    std::vector<StartingWorldElementSetting> startingWorldElements;
};

// for border.json and features
struct WeightedSimHash {
    std::string element;
    float weight{};
    Override overrides;
};

struct LevelLayer {
    std::vector<std::string> content;
    float bandSize{};
    float maxValue{};
};

// for layers.json
struct LevelLayerSettings {
    std::vector<LevelLayer> LevelLayers;
};

struct Mob {
    std::string name;
    PointSelectionMethod selectMethod{};
    MinMax density;
    float avoidRadius{};
    SampleBehaviour sampleBehaviour{};
    bool doAvoidPoints = true;
    bool dontRelaxChildren = false;
    MinMax blobSize;
    std::string prefabName;
    int width{};
    int height{};
    int paddingX{};
    Location location{};
};

// for mobs.json
struct MobSettings {
    ComposableDictionary<Mob> MobLookupTable;
};

// for rivers.json
struct River {
    std::string element;
    std::string backgroundElement;
    float widthCenter{};
    float widthBorder{};
    float temperature{};
    float maxMass{};
    float flowIn{};
    float flowOut{};
};

struct WeightedMob {
    float weight{};
    std::string tag;
};

// for rooms.json
struct Room {
    std::string name;
    PointSelectionMethod selectMethod{};
    MinMax density;
    float avoidRadius{};
    SampleBehaviour sampleBehaviour{};
    bool doAvoidPoints = true;
    bool dontRelaxChildren = false;
    MinMax blobSize;
    Shape shape{};
    Selection mobselection{};
    std::vector<WeightedMob> mobs;
};

// for temperatures.json
struct Temperature {
    float min{};
    float max{};
};
