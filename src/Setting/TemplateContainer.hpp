#pragma once

#include "SampleDescriber.hpp"
#include "Utils/Polygon.hpp"

struct Cell {
    SimHashes element{};
    float mass{};
    float temperature{};
    std::string diseaseName;
    int diseaseCount{};
    int location_x{};
    int location_y{};
    bool preventFoWReveal{};
};

struct Rottable {
    float rotAmount{};
};

struct StorageItem {
    std::string id;
    SimHashes element{};
    float units{};
    bool isOre{};
    float temperature{};
    std::string diseaseName;
    int diseaseCount{};
    Rottable rottable;
};

struct TemplateAmountValue {
    std::string id;
    float value{};
};

struct Prefab {
    std::string id;
    int location_x{};
    int location_y{};
    SimHashes element{};
    float temperature{};
    float units{};
    std::string diseaseName;
    int diseaseCount{};
    Orientation rotationOrientation{};
    std::vector<StorageItem> storage;
    Type type{};
    std::string facadeId;
    int connections{};
    Rottable rottable;
    std::vector<TemplateAmountValue> amounts;
    std::vector<TemplateAmountValue> other_values;
};

struct TemplateInfo {
    Vector2f size;
    Vector2f min;
    int area{};
    std::vector<std::string> discover_tags;

    // ignore lines

    Rect GetBounds(Vector2f &position, int padding) const
    {
        Rect result;
        result.x = (int)position.x + (int)min.x - padding;
        result.y = (int)position.y + (int)min.y - padding;
        result.width = (int)size.x + padding * 2;
        result.height = (int)size.y + padding * 2;
        return result;
    }
};

struct TemplateContainer {
    std::string name;
    TemplateInfo info;
    std::vector<Cell> cells;
    std::vector<Prefab> buildings;
    std::vector<Prefab> pickupables;
    std::vector<Prefab> elementalOres;
    std::vector<Prefab> otherEntities;
};
