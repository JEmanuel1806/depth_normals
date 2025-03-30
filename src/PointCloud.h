#pragma once

#include "Point.h"
#include <vector>

class PointCloud {
public:

    void AddPoint(const Point& point) {
        m_points.push_back(point);
    }

    int PointsAmount() const {
        return static_cast<int>(m_points.size());
    }

    Point* GetPointByID(int id);
    glm::vec3 GetNormalByID(int id);
    glm::vec3 GetColorByID(int id);

public:
    bool m_hasNormals = false;
    std::vector<Point> m_points;
};
