#include "PointCloud.h"

Point* PointCloud::GetPointByID(int id)
{
	for (auto& point : m_points) {
		if (point.m_pointID == id)
			return &point;
	}
	return nullptr;
	}

glm::vec3 PointCloud::GetColorByID(int id)
{
	for (auto& point : m_points) {
		if (point.m_pointID == id)
			return point.m_color;
	}
	return glm::vec3(0.0f); //if no result
}

glm::vec3 PointCloud::GetNormalByID(int id)
{
	for (auto& point : m_points) {
		if (point.m_pointID == id) {
			return point.m_normal;
		}
	}

	return glm::vec3(0.0f);


}
