#pragma once

#include "World.hpp"

struct WorldPlacement {
    std::string world;
    WorldMixing worldMixing;
    MinMax allowedRings = {0, 9999};
    int buffer = 2;
    LocationType locationType{};
    int x{};
    int y{};
    int width{};
    int height{};
    bool startWorld{};
    int hiddenY{};

    // ignore lines

    void SetSize(Vector2f size)
    {
        width = (int)size.x;
        height = (int)size.y;
    }

    void SetPosition(Vector2f pos)
    {
        x = (int)pos.x;
        y = (int)pos.y;
    }

    bool IsMixingPlacement() const
    {
        if (worldMixing.requiredTags.size() == 0) {
            return worldMixing.forbiddenTags.size() != 0;
        }
        return true;
    }
};

struct ClusterLayout {
    std::vector<WorldPlacement> worldPlacements;
    std::vector<std::string> requiredDlcIds;
    std::vector<std::string> forbiddenDlcIds;
    bool disableStoryTraits{};
    int fixedCoordinate{};
    ClusterCategory clusterCategory{};
    int startWorldIndex{};
    int width{};
    int height{};
    int numRings{};
    std::string coordinatePrefix;
    std::vector<std::string> clusterTags;

    // ignore lines

    bool IsMiniCluster() const
    {
        return coordinatePrefix.starts_with("M-");
    }
};
