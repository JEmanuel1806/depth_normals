#include "PointCloud.h"

Point* PointCloud::getPointByID(int id)
{
	for (auto& point : points) {
		if (point.ID == id)
			return &point;
	}
	return nullptr;
	}

glm::vec3 PointCloud::getColorByID(int id)
{
	for (auto& point : points) {
		if (point.ID == id)
			return point.color;
	}
	return glm::vec3(0.0f); //if no result
}

glm::vec3 PointCloud::getNormalByID(int id)
{
	for (auto& point : points) {
		if (point.ID == id) {
			return point.normal;
		}
	}

	return glm::vec3(0.0f);


}
