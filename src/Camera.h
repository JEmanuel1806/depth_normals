#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


constexpr unsigned int SCREEN_WIDTH = 1280;
constexpr unsigned int SCREEN_HEIGHT = 720;


enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    ROTATE_LEFT,
    ROTATE_RIGHT
};


class Camera {
public:
    Camera(glm::vec3 vecPosition = glm::vec3(0.0f),
        glm::vec3 vecUp = glm::vec3(0.0f, 1.0f, 0.0f),
        float fYaw = -90.0f,
        float fPitch = 0.0f);

    glm::mat4 GetViewMatrix() const;

    void ProcessKeyboard(CameraMovement direction, float fDeltaTime);
    void ProcessMouseMovement(float fXOffset, float fYOffset, bool bConstrainPitch = true);
    void ProcessMousePan(float fXOffset, float fYOffset);
    void ProcessMouseScroll(float fYOffset);

private:
    void UpdateCameraVectors();

public:
    glm::vec3 m_vecPosition;
    glm::vec3 m_vecFront;
    glm::vec3 m_vecUp;
    glm::vec3 m_vecRight;
    glm::vec3 m_vecWorldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;
};
