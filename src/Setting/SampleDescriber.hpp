#pragma once

#include <optional>
#include <string>

#include "SimHashes.hpp"
#include "Utils/KRandom.hpp"

struct MinMax {
    float min{};
    float max{};

    // ignore lines

    void Add(const MinMax &other)
    {
        min += other.min;
        max += other.max;
    }

    float GetRandomValue(KRandom &random) const
    {
        return random.Next(min, max);
    }
};

struct Override {
    float massOverride{};
    std::optional<float> massMultiplier{};
    float temperatureOverride{};
    float temperatureMultiplier{};
    std::string diseaseOverride;
    int diseaseAmountOverride{};

    // ignore lines

    void ModMultiplyMass(float mult)
    {
        if (!massMultiplier.has_value()) {
            massMultiplier = mult;
        } else {
            massMultiplier.value() *= mult;
        }
    }
};

struct SampleDescriber {
    std::string name;
    PointSelectionMethod selectMethod{};
    MinMax density;
    float avoidRadius{};
    SampleBehaviour sampleBehaviour{};
    bool doAvoidPoints = true;
    bool dontRelaxChildren = false;
    MinMax blobSize;
};
