#include "Voronoi.hpp"

inline int Voronoi::CompareByYThenX(const Site &s1, const Vector2f &s2)
{
    if (s1.y < s2.y) {
        return -1;
    }
    if (s1.y > s2.y) {
        return 1;
    }
    if (s1.x < s2.x) {
        return -1;
    }
    if (s1.x > s2.x) {
        return 1;
    }
    return 0;
}

inline const Site *Voronoi::FortunesAlgorithm_leftRegion(Halfedge &he)
{
    auto edge = he.edge;
    if (edge == nullptr) {
        return fortunesAlgorithm_bottomMostSite;
    }
    return edge->GetSite(he.leftRight);
}

inline const Site *Voronoi::FortunesAlgorithm_rightRegion(Halfedge &he)
{
    auto edge = he.edge;
    if (edge == nullptr) {
        return fortunesAlgorithm_bottomMostSite;
    }
    return edge->GetSite(OtherSide(he.leftRight));
}

void Voronoi::Create(const std::vector<Site> &points, const Rect &plotBounds)
{
    m_plotBounds = plotBounds;
    m_sites.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        m_sites[i] = &points[i];
    }
    sitesEdges.resize(m_sites.size() + 1);

    std::ranges::sort(m_sites, [](const Site *lhs, const Site *rhs) {
        return (*lhs) < (*rhs);
    });
    m_sitesBounds = GetSitesBounds();
    halfedgePool.emplace_back(nullptr, Side::LEFT);
    halfedgePool.emplace_back(nullptr, Side::RIGHT);
    leftEnd = &halfedgePool[0];
    rightEnd = &halfedgePool[1];
    leftEnd->edgeListRightNeighbor = rightEnd;
    rightEnd->edgeListLeftNeighbor = leftEnd;
    edgeList.resize(2 * (size_t)std::sqrt(m_sites.size() + 4));
    edgeList.front() = leftEnd;
    edgeList.back() = rightEnd;

    FortunesAlgorithm();
}

Rect Voronoi::GetSitesBounds()
{
    float num = std::numeric_limits<float>::max();
    float num2 = std::numeric_limits<float>::lowest();
    for (auto site : m_sites) {
        if (site->x < num) {
            num = site->x;
        }
        if (site->x > num2) {
            num2 = site->x;
        }
    }
    float y = m_sites.front()->y;
    float y2 = m_sites.back()->y;
    return Rect{num, y, num2 - num, y2 - y};
}

inline void Voronoi::ConnectHalfedges(Halfedge *left, Halfedge *right)
{
    right->edgeListLeftNeighbor = left;
    right->edgeListRightNeighbor = left->edgeListRightNeighbor;
    left->edgeListRightNeighbor->edgeListLeftNeighbor = right;
    left->edgeListRightNeighbor = right;
}

inline void Voronoi::DisconnectHalfedges(Halfedge *node)
{
    node->edgeListLeftNeighbor->edgeListRightNeighbor =
        node->edgeListRightNeighbor;
    node->edgeListRightNeighbor->edgeListLeftNeighbor =
        node->edgeListLeftNeighbor;
    node->edgeListRightNeighbor = (node->edgeListLeftNeighbor = nullptr);
}

inline Vector2f *Voronoi::IntersectHalfedges(Halfedge *first, Halfedge *second)
{
    Vector2f &vertex = vertexPool.emplace_back();
    if (!first->Intersect(*second, vertex)) {
        vertexPool.pop_back();
        return nullptr;
    }
    return &vertex;
}

Halfedge *Voronoi::EdgeListGetHash(int num)
{
    Halfedge *&halfedge = edgeList[num];
    if (halfedge != nullptr && halfedge->edgeListLeftNeighbor == nullptr &&
        halfedge->edgeListRightNeighbor == nullptr) {
        halfedge = nullptr;
    }
    return halfedge;
}

Halfedge *Voronoi::EdgeListLeftNeighbor(const Site &site)
{
    auto xmin = m_sitesBounds.x;
    auto deltax = m_sitesBounds.width;
    auto hashsize = (int)edgeList.size();

    int num = (int)((site.x - xmin) / deltax * hashsize);
    if (num < 0) {
        num = 0;
    }
    if (num >= hashsize) {
        num = hashsize - 1;
    }
    Halfedge *halfedge = EdgeListGetHash(num);
    if (halfedge == nullptr) {
        int num2 = 1;
        while ((halfedge = EdgeListGetHash(num - num2)) == nullptr &&
               (halfedge = EdgeListGetHash(num + num2)) == nullptr) {
            num2++;
        }
    }
    if (halfedge == leftEnd ||
        (halfedge != rightEnd && halfedge->IsLeftOf(site))) {
        do {
            halfedge = halfedge->edgeListRightNeighbor;
        } while (halfedge != rightEnd && halfedge->IsLeftOf(site));
        halfedge = halfedge->edgeListLeftNeighbor;
    } else {
        do {
            halfedge = halfedge->edgeListLeftNeighbor;
        } while (halfedge != leftEnd && !halfedge->IsLeftOf(site));
    }
    if (num > 0 && num < hashsize - 1) {
        edgeList[num] = halfedge;
    }
    return halfedge;
}

Edge *Voronoi::CreateBisectingEdge(const Site &site0, const Site &site1)
{
    auto &coord = site1;
    auto &coord2 = site0;
    double num = coord2.x - coord.x;
    double num2 = coord2.y - coord.y;
    auto num3 = ((num > 0.0f) ? num : (0.0f - num));
    auto num4 = ((num2 > 0.0f) ? num2 : (0.0f - num2));
    float num5 =
        coord.x * num + coord.y * num2 + (num * num + num2 * num2) * 0.5f;
    float num6;
    float num7;
    if (num3 > num4) {
        num6 = 1.0f;
        num7 = num2 / num;
        num5 /= num;
    } else {
        num7 = 1.0f;
        num6 = num / num2;
        num5 /= num2;
    }
    Edge &edge = edgePool.emplace_back();
    edge.leftSite = &site0;
    edge.rightSite = &site1;
    edge.a = num6;
    edge.b = num7;
    edge.c = num5;
    sitesEdges[site0.idx].push_back(&edge);
    sitesEdges[site1.idx].push_back(&edge);
    return &edge;
}

void Voronoi::FortunesAlgorithm()
{
    Vector2f s;
    int sites_cursor = 0;
    fortunesAlgorithm_bottomMostSite = m_sites[sites_cursor++];
    const Site *site = m_sites[sites_cursor++];
    while (true) {
        if (!halfedgePriorityQueue.empty()) {
            auto bottom = *halfedgePriorityQueue.begin();
            s.x = bottom->vertex->x;
            s.y = bottom->ystar;
        }
        Halfedge *halfedge;
        Halfedge *edgeListRightNeighbor;
        const Site *site2;
        Edge *edge;
        Halfedge *halfedge2;
        Vector2f *vertex;
        if (site != nullptr &&
            (halfedgePriorityQueue.empty() || CompareByYThenX(*site, s) < 0)) {
            halfedge = EdgeListLeftNeighbor(*site);
            edgeListRightNeighbor = halfedge->edgeListRightNeighbor;
            site2 = FortunesAlgorithm_rightRegion(*halfedge);
            edge = CreateBisectingEdge(*site2, *site);
            halfedge2 = &halfedgePool.emplace_back(edge, Side::LEFT);
            ConnectHalfedges(halfedge, halfedge2);
            vertex = IntersectHalfedges(halfedge, halfedge2);
            if (vertex != nullptr) {
                halfedgePriorityQueue.erase(halfedge);
                halfedge->vertex = vertex;
                halfedge->ystar = vertex->y + site->Distance(*vertex);
                halfedgePriorityQueue.insert(halfedge);
            }
            halfedge = halfedge2;
            halfedge2 = &halfedgePool.emplace_back(edge, Side::RIGHT);
            ConnectHalfedges(halfedge, halfedge2);
            vertex = IntersectHalfedges(halfedge2, edgeListRightNeighbor);
            if (vertex != nullptr) {
                halfedge2->vertex = vertex;
                halfedge2->ystar = vertex->y + site->Distance(*vertex);
                halfedgePriorityQueue.insert(halfedge2);
            }
            if (sites_cursor < (int)m_sites.size()) {
                site = m_sites[sites_cursor++];
            } else {
                site = nullptr;
            }
            continue;
        }
        if (halfedgePriorityQueue.empty()) {
            break;
        }
        halfedge = *halfedgePriorityQueue.begin();
        halfedgePriorityQueue.erase(halfedge);
        Halfedge *edgeListLeftNeighbor = halfedge->edgeListLeftNeighbor;
        edgeListRightNeighbor = halfedge->edgeListRightNeighbor;
        Halfedge *edgeListRightNeighbor2 =
            edgeListRightNeighbor->edgeListRightNeighbor;
        site2 = FortunesAlgorithm_leftRegion(*halfedge);
        const Site *site3 = FortunesAlgorithm_rightRegion(*edgeListRightNeighbor);
        Vector2f *vertex2 = halfedge->vertex;
        halfedge->edge->SetVertex(halfedge->leftRight, vertex2);
        edgeListRightNeighbor->edge->SetVertex(edgeListRightNeighbor->leftRight,
                                               vertex2);
        DisconnectHalfedges(halfedge);
        halfedgePriorityQueue.erase(edgeListRightNeighbor);
        DisconnectHalfedges(edgeListRightNeighbor);
        Side side = Side::LEFT;
        if (site2->y > site3->y) {
            const Site *site4 = site2;
            site2 = site3;
            site3 = site4;
            side = Side::RIGHT;
        }
        edge = CreateBisectingEdge(*site2, *site3);
        halfedge2 = &halfedgePool.emplace_back(edge, side);
        ConnectHalfedges(edgeListLeftNeighbor, halfedge2);
        edge->SetVertex(OtherSide(side), vertex2);
        vertex = IntersectHalfedges(edgeListLeftNeighbor, halfedge2);
        if (vertex != nullptr) {
            halfedgePriorityQueue.erase(edgeListLeftNeighbor);
            edgeListLeftNeighbor->vertex = vertex;
            edgeListLeftNeighbor->ystar = vertex->y + site2->Distance(*vertex);
            halfedgePriorityQueue.insert(edgeListLeftNeighbor);
        }
        vertex = IntersectHalfedges(halfedge2, edgeListRightNeighbor2);
        if (vertex != nullptr) {
            halfedge2->vertex = vertex;
            halfedge2->ystar = vertex->y + site2->Distance(*vertex);
            halfedgePriorityQueue.insert(halfedge2);
        }
    }
    for (auto &edge : edgePool) {
        edge.ClipVertices(m_plotBounds);
    }
}

std::deque<Edge *> Voronoi::ReorderEdges(int index, std::deque<Side> &sides)
{
    auto &edges = sitesEdges[index];
    int count = edges.size();
    std::vector<bool> barray(count);
    std::deque<Edge *> list;
    int num = 0;
    int num2 = 0;
    Edge *edge = edges[num2];
    list.push_back(edge);
    sides.push_back(Side::LEFT);

    auto *coord2 = edge->leftVertex;
    auto *coord4 = edge->rightVertex;
    barray[num2] = true;
    num++;
    while (num < count) {
        for (num2 = 1; num2 < count; num2++) {
            if (!barray[num2]) {
                edge = edges[num2];
                auto *coord6 = edge->leftVertex;
                auto *coord8 = edge->rightVertex;
                if (coord6 == coord4) {
                    coord4 = coord8;
                    sides.push_back(Side::LEFT);
                    list.push_back(edge);
                    barray[num2] = true;
                } else if (coord8 == coord2) {
                    coord2 = coord6;
                    sides.push_front(Side::LEFT);
                    list.push_front(edge);
                    barray[num2] = true;
                } else if (coord6 == coord2) {
                    coord2 = coord8;
                    sides.push_front(Side::RIGHT);
                    list.push_front(edge);
                    barray[num2] = true;
                } else if (coord8 == coord4) {
                    coord4 = coord6;
                    sides.push_back(Side::RIGHT);
                    list.push_back(edge);
                    barray[num2] = true;
                }
                if (barray[num2]) {
                    num++;
                }
            }
        }
    }
    return list;
}

namespace BoundsCheck
{
static constexpr int TOP = 1;
static constexpr int BOTTOM = 2;
static constexpr int LEFT = 4;
static constexpr int RIGHT = 8;

static int Check(Vector2f point, Rect bounds)
{
    int num = 0;
    if (point.x == bounds.x) {
        num |= LEFT;
    }
    if (point.x == bounds.x + bounds.width) {
        num |= RIGHT;
    }
    if (point.y == bounds.y) {
        num |= TOP;
    }
    if (point.y == bounds.y + bounds.height) {
        num |= BOTTOM;
    }
    return num;
}

static void AppendBoundsPoint(std::vector<Vector2f> &points, const Rect &bounds,
                              Vector2f &vector, Vector2f &value)
{
    float bounds_xMax = bounds.x + bounds.width;
    float bounds_yMax = bounds.y + bounds.height;

    int num = BoundsCheck::Check(vector, bounds);
    int num2 = BoundsCheck::Check(value, bounds);
    if ((num & BoundsCheck::RIGHT) != 0) {
        float xMax = bounds_xMax;
        if ((num2 & BoundsCheck::BOTTOM) != 0) {
            float yMax = bounds_yMax;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::TOP) != 0) {
            float yMax = bounds.y;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::LEFT) != 0) {
            float yMax =
                ((!(vector.y - bounds.y + value.y - bounds.y < bounds.height))
                     ? bounds_yMax
                     : bounds.y);
            points.emplace_back(xMax, yMax);
            points.emplace_back(bounds.x, yMax);
        }
    } else if ((num & BoundsCheck::LEFT) != 0) {
        float xMax = bounds.x;
        if ((num2 & BoundsCheck::BOTTOM) != 0) {
            float yMax = bounds_yMax;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::TOP) != 0) {
            float yMax = bounds.y;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::RIGHT) != 0) {
            float yMax =
                ((!(vector.y - bounds.y + value.y - bounds.y < bounds.height))
                     ? bounds_yMax
                     : bounds.y);
            points.emplace_back(xMax, yMax);
            points.emplace_back(bounds_xMax, yMax);
        }
    } else if ((num & BoundsCheck::TOP) != 0) {
        float yMax = bounds.y;
        if ((num2 & BoundsCheck::RIGHT) != 0) {
            float xMax = bounds_xMax;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::LEFT) != 0) {
            float xMax = bounds.x;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::BOTTOM) != 0) {
            float xMax =
                ((!(vector.x - bounds.x + value.x - bounds.x < bounds.width))
                     ? bounds_xMax
                     : bounds.x);
            points.emplace_back(xMax, yMax);
            points.emplace_back(xMax, bounds_yMax);
        }
    } else if ((num & BoundsCheck::BOTTOM) != 0) {
        float yMax = bounds_yMax;
        if ((num2 & BoundsCheck::RIGHT) != 0) {
            float xMax = bounds_xMax;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::LEFT) != 0) {
            float xMax = bounds.x;
            points.emplace_back(xMax, yMax);
        } else if ((num2 & BoundsCheck::TOP) != 0) {
            float xMax =
                ((!(vector.x - bounds.x + value.x - bounds.x < bounds.width))
                     ? bounds_xMax
                     : bounds.x);
            points.emplace_back(xMax, yMax);
            points.emplace_back(xMax, bounds.y);
        }
    }
}
} // namespace BoundsCheck

static void ConnectEnds(std::vector<Vector2f> &points, Edge *edge, Side side,
                        const Rect &bounds, bool closingUp)
{
    Vector2f &vector = points.back();
    Vector2f &value = edge->clippedEnds[(int)side];
    if (vector.Distance(value) >= 0.005f) {
        if (vector.x != value.x && vector.y != value.y) {
            BoundsCheck::AppendBoundsPoint(points, bounds, vector, value);
        }
        if (closingUp) {
            return;
        }
        points.emplace_back(value);
    }
    Vector2f &value2 = edge->clippedEnds[(int)OtherSide(side)];
    if (points[0].Distance(value2) >= 0.005f) {
        points.emplace_back(value2);
    }
}

void Voronoi::BuildRegion(Site &site)
{
    std::deque<Side> sides;
    std::deque<Edge *> reordered = ReorderEdges(site.idx, sides);

    site.polygon.Clear();
    std::vector<Vector2f> &list = site.polygon.Vertices;
    int count = reordered.size();
    int start = std::numeric_limits<int>::min();
    for (int i = 0; i < count; i++) {
        auto edge = reordered[i];
        auto side = sides[i];
        if (!edge->visible) {
            continue;
        }
        if (list.empty()) {
            start = i;
            list.push_back(edge->clippedEnds[(int)side]);
            list.push_back(edge->clippedEnds[(int)OtherSide(side)]);
            continue;
        }
        ConnectEnds(list, edge, side, m_plotBounds, false);
    }
    if (start >= 0) {
        ConnectEnds(list, reordered[start], sides[start], m_plotBounds, true);
    }
    float area = 0.0f;
    size_t j = list.size() - 1;
    for (size_t i = 0; i < list.size(); j = i++) {
        auto &vec1 = list[j];
        auto &vec2 = list[i];
        area += vec1.x * vec2.y - vec2.x * vec1.y;
    }
    if (area < 0.0f) {
        for (size_t i = 0, k = list.size() - 1; i < k; ++i, --k) {
            std::swap(list[i], list[k]);
        }
    }
}

void Voronoi::FindNeighborSites(Site &site)
{
    auto &edges = sitesEdges[site.idx];
    site.neighbours.clear();
    for (auto &edge : edges) {
        if (&site == edge->leftSite) {
            site.neighbours.push_back(edge->rightSite);
        }
        if (&site == edge->rightSite) {
            site.neighbours.push_back(edge->leftSite);
        }
    }
}
