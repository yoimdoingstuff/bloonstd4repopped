#pragma once

#include <vector>
#include <string>
#include <cmath>

namespace btd4 {

struct Point2D {
    float x{0.0f};
    float y{0.0f};
};

struct MapRect {
    float x{0.0f};
    float y{0.0f};
    float w{0.0f};
    float h{0.0f};

    bool contains(float px, float py) const {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }

    bool intersectsCircle(float cx, float cy, float radius) const {
        // Find closest point on rect to circle center
        float closestX = std::fmax(x, std::fmin(cx, x + w));
        float closestY = std::fmax(y, std::fmin(cy, y + h));
        float dx = cx - closestX;
        float dy = cy - closestY;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};

class Path {
public:
    Path() = default;
    explicit Path(std::vector<Point2D> waypoints);

    void addWaypoint(float x, float y);
    void clear();

    const std::vector<Point2D>& waypoints() const { return m_waypoints; }
    float totalLength() const { return m_totalLength; }
    size_t waypointCount() const { return m_waypoints.size(); }

    Point2D getPositionAtDistance(float distance) const;
    bool isAtEnd(float distance) const;

    void recalculate();

private:
    std::vector<Point2D> m_waypoints;
    std::vector<float> m_segmentLengths;
    std::vector<float> m_cumulativeLengths;
    float m_totalLength{0.0f};
};

class Map {
public:
    Map() = default;
    explicit Map(std::string name);

    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    void addPath(const Path& path);
    const std::vector<Path>& paths() const { return m_paths; }
    std::vector<Path>& paths() { return m_paths; }

    void addBuildableRegion(const MapRect& rect);
    void addBlockedRegion(const MapRect& rect);

    const std::vector<MapRect>& buildableRegions() const { return m_buildableRegions; }
    const std::vector<MapRect>& blockedRegions() const { return m_blockedRegions; }

    bool canPlaceTower(float x, float y, float footprintRadius) const;
    bool validate() const;

private:
    std::string m_name{"Default Map"};
    std::vector<Path> m_paths;
    std::vector<MapRect> m_buildableRegions;
    std::vector<MapRect> m_blockedRegions;
};

} // namespace btd4
