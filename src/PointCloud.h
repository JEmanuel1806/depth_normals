#pragma once

#include "Point.h"

#include <vector>

class PointCloud {
public:
	std::vector<Point> points;

	void addPoint(Point point) {
		points.push_back(point);
	}

	/*
	void removePoint(Point point) {
		points.erase(std::remove(points.begin(), points.end(), point), points.end());
	}
	*/

	int points_amount() {
		return points.size();
	}

	Point* getPointByID(int id);

	glm::vec3 getNormalByID(int id);
	glm::vec3 getColorByID(int id);

	
private:
	
};