#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>


constexpr float DEFAULT_SPEED = 2.5f;
constexpr float DEFAULT_SENSITIVITY = 0.1f;
constexpr float DEFAULT_ZOOM = 85.0f;


Camera::Camera(glm::vec3 vecPosition, glm::vec3 vecUp, float fYaw, float fPitch)
    : m_vecFront(glm::vec3(0.0f, 0.0f, -1.0f)),
    m_movementSpeed(DEFAULT_SPEED),
    m_mouseSensitivity(DEFAULT_SENSITIVITY),
    m_zoom(DEFAULT_ZOOM) {

    m_vecPosition = vecPosition;
    m_vecWorldUp = vecUp;
    m_yaw = fYaw;
    m_pitch = fPitch;

    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_vecPosition, m_vecPosition + m_vecFront, m_vecUp);
}


void Camera::ProcessKeyboard(CameraMovement direction, float fDeltaTime) {
   
    HasChanged = true; // Register Keyboard to recalculate

    float fVelocity = m_movementSpeed * fDeltaTime;

    switch (direction) {
    case CameraMovement::FORWARD:
        m_vecPosition += m_vecFront * fVelocity;
        break;
    case CameraMovement::BACKWARD:
        m_vecPosition -= m_vecFront * fVelocity;
        break;
    case CameraMovement::LEFT:
        m_vecPosition -= m_vecRight * fVelocity;
        break;
    case CameraMovement::RIGHT:
        m_vecPosition += m_vecRight * fVelocity;
        break;
    case CameraMovement::ROTATE_LEFT:
        m_yaw -= 50.0f * fDeltaTime;
        break;
    case CameraMovement::ROTATE_RIGHT:
        m_yaw += 50.0f * fDeltaTime;
        break;
    }

    UpdateCameraVectors();
}

void Camera::ProcessMouseMovement(float fXOffset, float fYOffset, bool bConstrainPitch) {
    
    HasChanged = true; // Register Mouse Movement to recalculate
    
    fXOffset *= m_mouseSensitivity;
    fYOffset *= m_mouseSensitivity;

    m_yaw += fXOffset;
    m_pitch += fYOffset;

    if (bConstrainPitch) {
        if (m_pitch > 89.0f)  m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void Camera::ProcessMousePan(float fXOffset, float fYOffset)
{

    HasChanged = true;

    float panSpeed = 0.005f; 

    glm::vec3 panRight = -m_vecRight * fXOffset * panSpeed;
    glm::vec3 panUp = m_vecUp * fYOffset * panSpeed;

    m_vecPosition += panRight + panUp;

}

void Camera::ProcessMouseScroll(float fYOffset) {

    HasChanged = true;

    m_zoom -= fYOffset;
    m_zoom = glm::clamp(m_zoom, 1.0f, 45.0f);
}

void Camera::UpdateCameraVectors() {
    glm::vec3 vecFront;
    vecFront.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    vecFront.y = sin(glm::radians(m_pitch));
    vecFront.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_vecFront = glm::normalize(vecFront);
    m_vecRight = glm::normalize(glm::cross(m_vecFront, m_vecWorldUp));
    m_vecUp = glm::normalize(glm::cross(m_vecRight, m_vecFront));
}
