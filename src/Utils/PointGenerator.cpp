#include "PointGenerator.hpp"

#include <cmath>
#include <numbers>
#include <fstream>

#include "Setting/SettingsCache.hpp"

struct Settings {
    Vector2f TopLeft;
    Vector2f LowerRight;
    Vector2f Center;
    Vector2f Dimensions;
    float MinDistance{};
    float CellSize{};
    int GridWidth{};
    int GridHeight{};
    KRandom *Random{};
};

struct State {
    std::vector<Vector2f> Grid;
    std::vector<Vector2f> ActivePoints;
    std::vector<Vector2f> Points;
};

// constexpr int DefaultPointsPerIteration = 30;
constexpr float SquareRootTwo = std::numbers::sqrt2_v<float>;

inline static Vector2f Denormalize(const Vector2f &point,
                                   const Settings &settings)
{
    auto &origin = settings.TopLeft;
    auto &cellSize = settings.CellSize;
    return {(float)(int)((double)(point.x - origin.x) / cellSize),
            (float)(int)((double)(point.y - origin.y) / cellSize)};
}

static Vector2f GenerateRandomAround(const Vector2f &center, Settings &settings)
{
    KRandom &random = *settings.Random;
    float num0 = random.NextSingle();
    double num2 = settings.MinDistance + settings.MinDistance * (double)num0;
    float num1 = random.NextSingle();
    double num3 = std::numbers::pi_v<float> * 2.0f * num1;
    float num4 = num2 * (float)std::sin(num3);
    float num5 = num2 * (float)std::cos(num3);
    return {center.x + num4, center.y + num5};
}

static void AddFirstPoint(Settings &settings, State &state)
{
    KRandom &random = *settings.Random;
    float num = std::min(settings.Dimensions.x, settings.Dimensions.y) / 2.0f;
    Vector2f vector{random.NextSingle(), random.NextSingle()};
    auto vector2 = settings.Center + vector * num;
    auto vector3 = Denormalize(vector2, settings);
    int index = (int)vector3.x * settings.GridHeight + (int)vector3.y;
    state.Grid[index] = vector2;
    state.ActivePoints.push_back(vector2);
    state.Points.push_back(vector2);
}

static bool AddNextPoint(const Vector2f &point, Settings &settings,
                         State &state)
{
    bool result = false;
    Vector2f vector = GenerateRandomAround(point, settings);
    if (vector.x >= settings.TopLeft.x && vector.x < settings.LowerRight.x &&
        vector.y > settings.TopLeft.y && vector.y < settings.LowerRight.y) {
        Vector2f range = Denormalize(vector, settings);
        bool flag = false;
        auto left = std::max(0.0f, range.x - 2.0f);
        auto right = std::min((float)settings.GridWidth, range.x + 3.0f);
        for (int i = (int)left; (float)i < right; i++) {
            if (flag) {
                break;
            }
            auto top = std::max(0.0f, range.y - 2.0f);
            auto bottom = std::min((float)settings.GridHeight, range.y + 3.0f);
            for (int j = (int)top; (float)j < bottom; j++) {
                if (flag) {
                    break;
                }
                auto &item = state.Grid[i * settings.GridHeight + j];
                if (item.x != 0.0f && item.y != 0.0f &&
                    item.Distance(vector) < settings.MinDistance) {
                    flag = true;
                }
            }
        }
        if (!flag) {
            result = true;
            state.ActivePoints.push_back(vector);
            state.Points.push_back(vector);
            int index = (int)range.x * settings.GridHeight + (int)range.y;
            state.Grid[index] = vector;
        }
    }
    return result;
}

static std::vector<Vector2f>
PoissonDiskSample(KRandom &random, Vector2f topLeft, Vector2f lowerRight,
                  float minimumDistance, int pointsPerIteration)
{
    Settings settings;
    settings.TopLeft = topLeft;
    settings.LowerRight = lowerRight;
    settings.Center = (topLeft + lowerRight) / 2.0f;
    settings.Dimensions = lowerRight - topLeft;
    settings.MinDistance = minimumDistance;
    settings.CellSize = minimumDistance / SquareRootTwo;
    settings.GridWidth = (int)(settings.Dimensions.x / settings.CellSize) + 1;
    settings.GridHeight = (int)(settings.Dimensions.y / settings.CellSize) + 1;
    settings.Random = &random;
    State state;
    state.Grid.resize(settings.GridWidth * settings.GridHeight);
    state.Points.reserve(state.Grid.size());
    state.ActivePoints.reserve(state.Grid.size());
    AddFirstPoint(settings, state);
    while (!state.ActivePoints.empty()) {
        int count = (int)state.ActivePoints.size() - 1;
        int index = random.Next(0, count);
        Vector2f point = state.ActivePoints[index];
        bool flag = false;
        for (int i = 0; i < pointsPerIteration; i++) {
            flag = AddNextPoint(point, settings, state) || flag;
        }
        if (!flag) {
            state.ActivePoints.erase(state.ActivePoints.begin() + index);
        }
    }
    return state.Points;
}

static std::vector<Vector2f> UniformHexSample(float width, float height,
                                              float density,
                                              const Vector2f &centroid)
{
    std::vector<Vector2f> result;
    auto w_2 = width / 2.0;
    auto h_2 = height / 2.0;
    int count = (int)std::sqrt(std::floor(width * height / density));
    result.reserve(count * count);
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            float x = -w_2 + (0.5 + i) / count * width;
            float y = -h_2 + (0.25 + 0.5 * (i % 2) + j) / count * height;
            result.emplace_back(centroid.x + x, centroid.y + y);
        }
    }
    return result;
}

std::vector<Vector2f> GetRandomPoints(const Polygon &boundingArea,
                                      float density, float avoidRadius,
                                      const std::vector<Vector2f> &avoidPoints,
                                      SampleBehaviour behaviour,
                                      bool testInsideBounds, KRandom &random,
                                      bool doShuffle, bool testAvoidPoints)
{
    std::vector<Vector2f> points;
    switch (behaviour) {
    case SampleBehaviour::UniformHex: {
        auto &bounds = boundingArea.Bounds();
        points = UniformHexSample(bounds.width, bounds.height, density,
                                  boundingArea.Centroid());
        break;
    }
    case SampleBehaviour::PoissonDisk: {
        auto &bounds = boundingArea.Bounds();
        Vector2f minPoint{bounds.x, bounds.y};
        Vector2f maxPoint{bounds.x + bounds.width, bounds.y + bounds.height};
        points = PoissonDiskSample(random, minPoint, maxPoint, density, 20);
        break;
    }
    default:
        LogE("Unsupport sampler behaviour: %d", (int)behaviour);
        return points;
    }
    double squaredAvoidRadius = avoidRadius * avoidRadius;
    auto first = points.begin();
    for (auto itr = points.begin(); itr != points.end(); ++itr) {
        if (testInsideBounds && !boundingArea.Contains(*itr)) {
            continue;
        }
        bool flag = false;
        if (testAvoidPoints && !avoidPoints.empty()) {
            for (auto &avoidPoint : avoidPoints) {
                if (avoidPoint.DistanceSquared(*itr) < squaredAvoidRadius) {
                    flag = true;
                    break;
                }
            }
        }
        if (!flag) {
            if (first != itr) {
                *first = std::move(*itr);
            }
            ++first;
        }
    }
    points.erase(first, points.end());
    if (doShuffle) {
        ShuffleSeeded(points, random);
    }
    return points;
}
