#include "Point.h"


void Point::SetPosition(const glm::vec3& pos)
{
	m_position = pos;
}


void Point::SetNormal(const glm::vec3& norm)
{
	m_normal = norm;
}

glm::vec3 Point::GetPosition() const
{
	return m_position;
}

glm::vec3 Point::GetNormal() const
{
	return m_normal;
}
