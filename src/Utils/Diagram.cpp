#include "Diagram.hpp"

#include <numbers>
#include <algorithm>

#include "Voronoi.hpp"
#include "ConvexHull.hpp"

constexpr float EPSILON = std::numeric_limits<float>::denorm_min();

static Vector2f GetDualPoint(ConvexFace<Site> &face)
{
    if (!face.dualPoint.has_value()) {
        auto &Vertices = face.Vertices;
        Vector3d vec1(Vertices[0]->x, Vertices[0]->y, Vertices[0]->z);
        Vector3d vec2(Vertices[1]->x, Vertices[1]->y, Vertices[1]->z);
        Vector3d vec3(Vertices[2]->x, Vertices[2]->y, Vertices[2]->z);
        double num1 = vec1.y * (vec2.z - vec3.z) + vec2.y * (vec3.z - vec1.z) +
                      vec3.y * (vec1.z - vec2.z);
        double num2 = vec1.z * (vec2.x - vec3.x) + vec2.z * (vec3.x - vec1.x) +
                      vec3.z * (vec1.x - vec2.x);
        double num3 = vec1.x * (vec2.y - vec3.y) + vec2.x * (vec3.y - vec1.y) +
                      vec3.x * (vec1.y - vec2.y);
        num3 = -0.5 / num3;
        face.dualPoint.emplace((float)(num1 * num3), (float)(num2 * num3));
    }
    return face.dualPoint.value();
}

inline static double Det(Vector3d *m)
{
    return m[0].x * (m[1].y * m[2].z - m[2].y * m[1].z) -
           m[0].y * (m[1].x * m[2].z - m[2].x * m[1].z) +
           m[0].z * (m[1].x * m[2].y - m[2].x * m[1].y);
}

inline static double LengthSquared(double x, double y) { return x * x + y * y; }

static Vector2f GetCircumcenter(ConvexFace<Site> &face)
{
    if (!face.circumcenter.has_value()) {
        auto &Vertices = face.Vertices;
        Vector3d data[3];
        for (int i = 0; i < 3; i++) {
            data[i].x = Vertices[i]->x;
            data[i].y = Vertices[i]->y;
            data[i].z = 1.0;
        }
        double num = Det(data);
        double num2 = -1.0 / (2.0 * num);
        for (int j = 0; j < 3; j++) {
            data[j].x = LengthSquared(Vertices[j]->x, Vertices[j]->y);
        }
        double num3 = 0.0 - Det(data);
        for (int k = 0; k < 3; k++) {
            data[k].y = Vertices[k]->x;
        }
        double num4 = Det(data);
        face.circumcenter.emplace((float)(num2 * num3), (float)(num2 * num4));
    }
    return face.circumcenter.value();
}

static void PolyForRandomPoints(std::vector<Vector2f> &verts)
{
    if (verts.empty()) {
        return;
    }
    ConvexHull hull;
    auto hullResult = hull.Create2D(verts);
    auto &Points = hullResult.Points;
    double area = 0.0;
    int count = Points.size();
    for (int i = 0; i < count; i++) {
        int index = (i + 1) % count;
        auto &vector1 = *Points[i];
        auto &vector2 = *Points[index];
        area += vector1.x * vector2.y - vector2.x * vector1.y;
    }
    std::vector<Vector2f> result;
    if (area < 0.0) {
        for (auto itr = Points.rbegin(); itr != Points.rend(); ++itr) {
            result.emplace_back(**itr);
        }
    } else {
        for (auto itr = Points.begin(); itr != Points.end(); ++itr) {
            result.emplace_back(**itr);
        }
    }
    verts.swap(result);
}

static bool ContainsVert(const ConvexFace<Site> *face, const Site *target)
{
    if (face == nullptr || target == nullptr) {
        return false;
    }
    for (int i = 0; i < 3; i++) {
        if (face->Vertices[i] == target) {
            return true;
        }
    }
    return false;
}

static Edge GetEdge(const ConvexFace<Site> &face0,
                    const ConvexFace<Site> &face1)
{
    Edge edge;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (face0.Vertices[i] == face1.Vertices[j]) {
                if (edge.leftSite == nullptr) {
                    edge.leftSite = face0.Vertices[i];
                } else {
                    edge.rightSite = face0.Vertices[i];
                }
            }
        }
    }
    return edge;
}

static void TouchingFaces(Site *site, ConvexFace<Site> &startingFace,
                          std::vector<ConvexFace<Site> *> &result)
{
    result.clear();
    std::stack<ConvexFace<Site> *> stack;
    stack.push(&startingFace);
    while (!stack.empty()) {
        auto convexFaceExt = stack.top();
        stack.pop();
        if (!ContainsVert(convexFaceExt, site) ||
            std::ranges::find(result, convexFaceExt) != result.end()) {
            continue;
        }
        result.push_back(convexFaceExt);
        for (auto face : convexFaceExt->Adjacency) {
            if (ContainsVert(face, site)) {
                stack.push(face);
            }
        }
    }
}

static void GenerateNeighbors(Site *site, const ConvexFace<Site> &startFace)
{
    std::vector<const Site *> &neighbors = site->neighbours;
    std::vector<const ConvexFace<Site> *> list2;
    std::stack<const ConvexFace<Site> *> stack;
    stack.push(&startFace);
    while (!stack.empty()) {
        auto convexFaceExt = stack.top();
        stack.pop();
        list2.push_back(convexFaceExt);
        for (auto face : convexFaceExt->Adjacency) {
            if (ContainsVert(face, site) &&
                std::ranges::find(list2, face) == list2.end()) {
                auto edge = GetEdge(*convexFaceExt, *face);
                auto dualSite3d =
                    ((edge.leftSite == site) ? edge.rightSite : edge.leftSite);
                neighbors.push_back(dualSite3d);
                stack.push(face);
            }
        }
    }
}

static void FilterNeighbours(Site &home)
{
    auto itr = home.neighbours.begin();
    while (itr != home.neighbours.end()) {
        auto site = *itr;
        if (site->dummy || !site->polygon.SimpleSharesEdge(home.polygon)) {
            itr = home.neighbours.erase(itr);
        } else {
            ++itr;
        }
    }
}

Diagram::Diagram(Polygon &bounds, std::vector<Site> &sites)
    : m_bounds{bounds}
    , m_sites{sites}
{
    for (auto &site : m_sites) {
        m_weightSum += site.weight;
    }
}

bool Diagram::MakeVD(const Rect &bounds)
{
    Voronoi voronoi;
    voronoi.Create(m_sites, bounds);
    for (auto &site : m_sites) {
        if (site.dummy) {
            continue;
        }
        voronoi.BuildRegion(site);
        voronoi.FindNeighborSites(site);
        site.polygon.Intersect(m_bounds);
    }
    for (auto &site : m_sites) {
        site.UpdatePosition();
    }
    return true;
}

bool Diagram::ComputeNode()
{
    if (m_sites.size() == 1) {
        m_sites[0].polygon = m_bounds;
        m_sites[0].UpdatePosition();
        return true;
    }
    size_t originSize = m_sites.size();
    Rect bounds = m_bounds.Bounds();
    auto minCorner = bounds.MinCorner();
    auto maxCorner = bounds.MaxCorner();
    auto center = bounds.Center();
    auto index = (uint16_t)(originSize + 1);
    m_sites.emplace_back(index++, minCorner.x - 500.0f, center.y);
    m_sites.back().dummy = true;
    m_sites.emplace_back(index++, maxCorner.x + 500.0f, center.y);
    m_sites.back().dummy = true;
    m_sites.emplace_back(index++, center.x, minCorner.y - 500.0f);
    m_sites.back().dummy = true;
    m_sites.emplace_back(index++, center.x, maxCorner.y + 500.0f);
    m_sites.back().dummy = true;
    bounds.x -= 500.0f;
    bounds.y -= 500.0f;
    bounds.width += 500.0f;
    bounds.height += 500.0f;
    MakeVD(bounds);
    m_sites.resize(originSize);
    for (auto &site : m_sites) {
        FilterNeighbours(site);
    }
    return true;
}

bool Diagram::ComputeVD()
{
    ConvexHull hull;
    auto hullResult = hull.CreateDelaunay(m_sites);
    std::vector<ConvexFace<Site> *> roundFaces;
    for (auto &cell : hullResult.Faces) {
        GetCircumcenter(cell);
        for (auto site : cell.Vertices) {
            if (site->dummy || site->visited) {
                continue;
            }
            site->visited = true;
            site->polygon.Clear();
            TouchingFaces(site, cell, roundFaces);
            for (auto item : roundFaces) {
                auto center = GetCircumcenter(*item);
                site->polygon.Vertices.emplace_back(center);
            }
            PolyForRandomPoints(site->polygon.Vertices);
            site->polygon.Intersect(m_bounds);
        }
    }
    for (auto &site : m_sites) {
        site.neighbours.clear();
        site.UpdatePosition();
    }
    return true;
}

bool Diagram::ComputePD()
{
    ConvexHull hull;
    auto hullResult= hull.Create(m_sites);
    std::vector<ConvexFace<Site> *> roundFaces;
    for (auto &face : hullResult.Faces) {
        if (face.Normal[2] >= -EPSILON) {
            continue;
        }
        for (auto site : face.Vertices) {
            if (site->dummy || site->visited) {
                continue;
            }
            site->visited = true;
            site->neighbours.clear();
            site->polygon.Clear();
            TouchingFaces(site, face, roundFaces);
            GenerateNeighbors(site, face);
            for (auto item : roundFaces) {
                auto center = GetDualPoint(*item);
                site->polygon.Vertices.emplace_back(center);
            }
            PolyForRandomPoints(site->polygon.Vertices);
            site->polygon.Intersect(m_bounds);
        }
    }
    for (auto &site : m_sites) {
        site.UpdatePosition();
    }
    return true;
}

bool Diagram::ComputePowerDiagram()
{
    for (int i = 0; i <= 500; i++) {
        UpdateWeights();
        ComputePD();
        float max = 0.0f;
        for (auto &site : m_sites) {
            if (!site.dummy) {
                float area1 = site.polygon.Area();
                float area2 = site.weight / m_weightSum * m_bounds.Area();
                float ratio = std::abs(area1 - area2) / area2;
                if (max < ratio) {
                    max = ratio;
                }
            }
        }
        if (max < 0.2f) {
            break;
        }
    }
    return true;
}

void Diagram::UpdateWeights()
{
    float min = 0.0f;
    int externCount = m_bounds.Vertices.size() * 2;
    auto sites = m_sites | std::views::take(m_sites.size() - externCount);
    for (auto &site : sites) {
        site.visited = false;
        if (site.currentWeight < 1.0f) {
            site.currentWeight = 1.0f;
        }
        float area1 = site.polygon.Area();
        float area2 = site.weight / (double)m_weightSum * m_bounds.Area();
        float ratio = area2 / area1;
        if ((ratio > 1.1f && site.previousWeightAdaption < 0.9f) ||
            (ratio < 0.9f && site.previousWeightAdaption > 1.1f)) {
            ratio = std::sqrt(ratio);
        }
        if (ratio < 1.1f && ratio > 0.9f && site.currentWeight != 1.0f) {
            ratio = std::sqrt(ratio);
        }
        if (site.currentWeight < 10.0f) {
            ratio *= ratio;
        }
        if (site.currentWeight > 10.0f) {
            ratio = std::sqrt(ratio);
        }
        site.previousWeightAdaption = ratio;
        site.currentWeight *= ratio;
        if (site.currentWeight < 1.0f) {
            float radius1 = std::sqrt(area1 / std::numbers::pi_v<float>);
            float radius2 = std::sqrt(area2 / std::numbers::pi_v<float>);
            float diff = std::sqrt(site.currentWeight) - (radius1 - radius2);
            if (diff < 0.0f) {
                site.currentWeight = 0.0f - diff * diff;
                if (site.currentWeight < min) {
                    min = site.currentWeight;
                }
            }
        }
    }
    if (min < 0.0f) {
        min = -min;
        for (auto &site : sites) {
            site.currentWeight += min + 1.0;
        }
    }
    min = 1.0f;
    for (auto &site : sites) {
        if (site.neighbours.empty()) {
            float dist = site.x * site.x + site.y * site.y;
            float factor = dist / (std::abs(site.currentWeight) + 1.0f);
            if (factor < min) {
                min = factor;
            }
        }
        for (auto &neighbour : site.neighbours) {
            float diff = site.currentWeight - neighbour->currentWeight;
            float dist = site.DistanceSquared(*neighbour);
            float factor = dist / (std::abs(diff) + 1.0f);
            if (factor < min) {
                min = factor;
            }
        }
    }
    for (auto &site : sites) {
        site.currentWeight *= min;
        site.UpdatePosition();
    }
}

bool Diagram::ComputeNodePD()
{
    size_t originSize = m_sites.size();
    auto &centroid = m_bounds.Centroid();
    auto index = (uint16_t)(originSize + 1);
    m_weightSum = 0.0f;
    for (auto &site : m_sites) {
        site.visited = false;
        site.currentWeight = site.weight;
        site.previousWeightAdaption = 0.0f;
        m_weightSum += site.weight;
    }
    int j = 1;
    for (int i = 0; i < (int)m_bounds.Vertices.size(); j++, i++) {
        if (j == (int)m_bounds.Vertices.size())
            j = 0;
        auto &point1 = m_bounds.Vertices[i];
        auto &point2 = m_bounds.Vertices[j];
        auto point3 = (point2 - point1) * 0.5f + point2;
        auto extPoint1 = point1 + (point1 - centroid).Normalized() * 1000.0f;
        auto extPoint2 = point2 + (point3 - centroid).Normalized() * 1000.0f;
        m_sites.emplace_back(index++, extPoint1, EPSILON);
        m_sites.back().dummy = true;
        m_sites.emplace_back(index++, extPoint2, EPSILON);
        m_sites.back().dummy = true;
    }
    ComputeVD();
    ComputePowerDiagram();
    m_sites.resize(originSize);
    for (auto &site : m_sites) {
        FilterNeighbours(site);
    }
    return true;
}
