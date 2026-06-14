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

// for temperatures.json
struct Temperature {
    float min{};
    float max{};
};
