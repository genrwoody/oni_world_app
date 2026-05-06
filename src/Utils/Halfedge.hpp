#pragma once

#include <string>
#include <deque>
#include <map>
#include <set>
#include <memory>

#include "Polygon.hpp"

struct Site;
struct Edge;
struct Halfedge;
struct SubWorld;
struct Feature;

enum class Side { LEFT, RIGHT };

inline Side OtherSide(Side side)
{
    return side == Side::LEFT ? Side::RIGHT : Side::LEFT;
}

struct Site {
    bool dummy = false;
    mutable bool visited = false;
    int idx = 0;
    float x = 0;
    float y = 0;
    float z = 0;
    float weight = 1.0f;
    float currentWeight = 0.0f;
    float previousWeightAdaption = 0.0f;
    std::vector<const Site *> neighbours;
    Polygon polygon;

    Site *parent = nullptr;
    const SubWorld *subworld = nullptr;
    const Feature *globalFeature = nullptr;
    std::unique_ptr<std::vector<Site>> children;
    std::set<std::string> tags;
    std::map<std::string, int> minDistanceToTag;
    std::string templateTag;

    Site() = default;

    Site(int idx_, float x_, float y_, float weight_ = 1.0f)
        : idx(idx_)
        , x(x_)
        , y(y_)
        , weight(weight_)
        , currentWeight(weight_)
    {
        z = (double)x * x + (double)y * y - currentWeight;
    }

    Site(int idx_, const Vector2f &position_, float weight_ = 1.0f)
        : Site(idx_, position_.x, position_.y, weight_)
    {
    }

    Site(const Site &) = delete;            // uncopyable
    Site &operator=(const Site &) = delete; // uncopyable

    Site(Site &&) = default;

    inline bool operator<(const Site &site) const
    {
        if (y < site.y) {
            return true;
        }
        if (y > site.y) {
            return false;
        }
        return x < site.x;
    }

    float Distance(const Vector2f &rhs) const
    {
        double dx = rhs.x - x;
        double dy = rhs.y - y;
        return (float)std::sqrt(dx * dx + dy * dy);
    }

    void UpdatePosition()
    {
        if (!dummy) {
            auto &centroid = polygon.Centroid();
            x = centroid.x;
            y = centroid.y;
            z = (double)x * x + (double)y * y - currentWeight;
        }
    }

    float DistanceSquared(const Site &rhs) const
    {
        float dx = x - rhs.x;
        float dy = y - rhs.y;
        return dx * dx + dy * dy;
    }
};

struct Edge {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;

    bool visible = false;

    Vector2f *leftVertex = nullptr;
    Vector2f *rightVertex = nullptr;

    Vector2f clippedEnds[2];

    const Site *leftSite = nullptr;
    const Site *rightSite = nullptr;

    void SetVertex(Side leftRight, Vector2f *v)
    {
        if (leftRight == Side::LEFT) {
            leftVertex = v;
        } else {
            rightVertex = v;
        }
    }

    const Site *GetSite(Side leftRight) const
    {
        return leftRight == Side::LEFT ? leftSite : rightSite;
    }

    void ClipVertices(const Rect &bounds);
};

struct Halfedge {
    Halfedge *edgeListLeftNeighbor = nullptr;

    Halfedge *edgeListRightNeighbor = nullptr;

    Edge *edge = nullptr;

    Side leftRight;

    Vector2f *vertex = nullptr;

    float ystar = 0.0f;

    Halfedge(Edge *edge_, Side leftRight_)
        : edge{edge_}
        , leftRight{leftRight_}
    {
    }

    bool operator<(const Halfedge &rhs) const
    {
        if (ystar < rhs.ystar)
            return true;
        if (ystar == rhs.ystar)
            return vertex->x < rhs.vertex->x;
        return false;
    }

    bool IsLeftOf(const Site &point) const;

    bool Intersect(const Halfedge &halfedge1, Vector2f &result) const;
};
