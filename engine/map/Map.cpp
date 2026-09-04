#include "Map.hpp"
#include <algorithm>

namespace btd4 {

Path::Path(std::vector<Point2D> waypoints)
    : m_waypoints(std::move(waypoints)) {
    recalculate();
}

void Path::addWaypoint(float x, float y) {
    m_waypoints.push_back({x, y});
    recalculate();
}

void Path::clear() {
    m_waypoints.clear();
    m_segmentLengths.clear();
    m_cumulativeLengths.clear();
    m_totalLength = 0.0f;
}

void Path::recalculate() {
    m_segmentLengths.clear();
    m_cumulativeLengths.clear();
    m_totalLength = 0.0f;

    if (m_waypoints.size() < 2) {
        return;
    }

    m_cumulativeLengths.push_back(0.0f);
    for (size_t i = 0; i + 1 < m_waypoints.size(); ++i) {
        float dx = m_waypoints[i + 1].x - m_waypoints[i].x;
        float dy = m_waypoints[i + 1].y - m_waypoints[i].y;
        float len = std::sqrt(dx * dx + dy * dy);
        m_segmentLengths.push_back(len);
        m_totalLength += len;
        m_cumulativeLengths.push_back(m_totalLength);
    }
}

Point2D Path::getPositionAtDistance(float distance) const {
    if (m_waypoints.empty()) {
        return {0.0f, 0.0f};
    }
    if (m_waypoints.size() == 1 || distance <= 0.0f) {
        return m_waypoints.front();
    }
    if (distance >= m_totalLength) {
        return m_waypoints.back();
    }

    // Binary search or linear search segment
    for (size_t i = 0; i < m_segmentLengths.size(); ++i) {
        float segStartDist = m_cumulativeLengths[i];
        float segLen = m_segmentLengths[i];
        if (distance <= segStartDist + segLen) {
            if (segLen <= 0.0001f) {
                return m_waypoints[i];
            }
            float t = (distance - segStartDist) / segLen;
            float px = m_waypoints[i].x + t * (m_waypoints[i + 1].x - m_waypoints[i].x);
            float py = m_waypoints[i].y + t * (m_waypoints[i + 1].y - m_waypoints[i].y);
            return {px, py};
        }
    }

    return m_waypoints.back();
}

bool Path::isAtEnd(float distance) const {
    return distance >= m_totalLength;
}

Map::Map(std::string name)
    : m_name(std::move(name)) {
}

void Map::addPath(const Path& path) {
    m_paths.push_back(path);
}

void Map::addBuildableRegion(const MapRect& rect) {
    m_buildableRegions.push_back(rect);
}

void Map::addBlockedRegion(const MapRect& rect) {
    m_blockedRegions.push_back(rect);
}

bool Map::canPlaceTower(float x, float y, float footprintRadius) const {
    // If buildable regions are defined, the center must be inside at least one
    if (!m_buildableRegions.empty()) {
        bool inBuildable = false;
        for (const auto& rect : m_buildableRegions) {
            if (rect.contains(x, y)) {
                inBuildable = true;
                break;
            }
        }
        if (!inBuildable) {
            return false;
        }
    }

    // Must not intersect any blocked region
    for (const auto& rect : m_blockedRegions) {
        if (rect.intersectsCircle(x, y, footprintRadius)) {
            return false;
        }
    }

    // Must not be on top of a path (check distance to each path segment)
    for (const auto& path : m_paths) {
        const auto& wps = path.waypoints();
        for (size_t i = 0; i + 1 < wps.size(); ++i) {
            float x1 = wps[i].x;
            float y1 = wps[i].y;
            float x2 = wps[i + 1].x;
            float y2 = wps[i + 1].y;

            float dx = x2 - x1;
            float dy = y2 - y1;
            float l2 = dx * dx + dy * dy;
            float t = (l2 > 0.0001f) ? std::clamp(((x - x1) * dx + (y - y1) * dy) / l2, 0.0f, 1.0f) : 0.0f;

            float projX = x1 + t * dx;
            float projY = y1 + t * dy;
            float distSq = (x - projX) * (x - projX) + (y - projY) * (y - projY);

            // 12.0f default track buffer radius
            float minAllowedDist = footprintRadius + 8.0f;
            if (distSq < (minAllowedDist * minAllowedDist)) {
                return false;
            }
        }
    }

    return true;
}

bool Map::validate() const {
    if (m_paths.empty()) {
        return false;
    }
    for (const auto& path : m_paths) {
        if (path.waypointCount() < 2 || path.totalLength() <= 0.0f) {
            return false;
        }
    }
    return true;
}

} // namespace btd4
