#include "Point.h"



void Point::setPosition(glm::vec3 pos)
{
    position = pos;
}

glm::vec3 Point::getPosition() {
    return position;
}

void Point::setNormal(glm::vec3 norm) {
    normal = norm;
}

glm::vec3 Point::getNormal() {
    return normal;
}
