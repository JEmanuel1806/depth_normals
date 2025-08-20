#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Point {
public:

    int m_pointID;
    glm::vec3 _padVec;
    glm::vec3 m_position;
    float _padA;
    glm::vec3 m_color;
    float _padB;
    glm::vec3 m_normal;
    float _padC;

    Point()
        : m_pointID(-1),
        m_position(0.0f),
        m_color(1.0f),
        m_normal(0.0f, 0.0f, 0.0f)
    {}

    void SetPosition(const glm::vec3& pos);

    glm::vec3 GetPosition() const;

    void SetNormal(const glm::vec3& norm);

    glm::vec3 GetNormal() const;

};
