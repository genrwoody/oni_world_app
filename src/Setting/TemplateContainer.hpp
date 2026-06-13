#pragma once

#include "SampleDescriber.hpp"
#include "Utils/Polygon.hpp"

struct Prefab {
    std::string id;
    int location_x{};
    int location_y{};
};

struct TemplateInfo {
    Vector2f size;
    Vector2f min;
    int area{};

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
    std::vector<Prefab> otherEntities;
};
