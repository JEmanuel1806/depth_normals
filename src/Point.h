#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Point {
public:

	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
	int ID;

	Point()
		: ID(-1), 
		position(0.0f), normal(0.0f, 0.0f, 1.0f), color(1.0f) {}

	void setPosition(glm::vec3 pos);
	glm::vec3 getPosition();

	void setNormal(glm::vec3 norm);
	glm::vec3 getNormal();

private:

};