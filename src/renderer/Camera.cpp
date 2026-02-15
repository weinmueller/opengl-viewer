#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera(float distance)
    : m_target(0.0f, 0.0f, 0.0f)
    , m_distance(distance)
    , m_yaw(0.0f)
    , m_pitch(30.0f)
    , m_fov(45.0f)
    , m_near(0.1f)
    , m_far(1000.0f)
    , m_minDistance(0.1f)
    , m_maxDistance(500.0f)
    , m_minPitch(-89.0f)
    , m_maxPitch(89.0f)
    , m_orbitSensitivity(0.3f)
    , m_panSensitivity(0.005f)
    , m_zoomSensitivity(0.5f)
{
    updateViewMatrix();
}

void Camera::orbit(float deltaX, float deltaY) {
    m_yaw += deltaX * m_orbitSensitivity;
    m_pitch += deltaY * m_orbitSensitivity;

    m_pitch = glm::clamp(m_pitch, m_minPitch, m_maxPitch);

    if (m_yaw > 360.0f) m_yaw -= 360.0f;
    if (m_yaw < 0.0f) m_yaw += 360.0f;

    updateViewMatrix();
}

void Camera::pan(float deltaX, float deltaY) {
    // Extract right and up vectors from the view matrix to avoid
    // degenerate cross product when looking nearly straight up/down
    glm::vec3 right(m_viewMatrix[0][0], m_viewMatrix[1][0], m_viewMatrix[2][0]);
    glm::vec3 up(m_viewMatrix[0][1], m_viewMatrix[1][1], m_viewMatrix[2][1]);

    float panScale = m_distance * m_panSensitivity;
    m_target += right * (-deltaX * panScale);
    m_target += up * (deltaY * panScale);

    updateViewMatrix();
}

void Camera::zoom(float delta) {
    m_distance -= delta * m_zoomSensitivity * m_distance * 0.1f;
    m_distance = glm::clamp(m_distance, m_minDistance, m_maxDistance);
    updateViewMatrix();
}

glm::vec3 Camera::getPosition() const {
    float pitchRad = glm::radians(m_pitch);
    float yawRad = glm::radians(m_yaw);

    glm::vec3 offset;
    offset.x = m_distance * cos(pitchRad) * sin(yawRad);
    offset.y = m_distance * sin(pitchRad);
    offset.z = m_distance * cos(pitchRad) * cos(yawRad);

    return m_target + offset;
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_near, m_far);
}

void Camera::updateViewMatrix() {
    glm::vec3 position = getPosition();
    m_viewMatrix = glm::lookAt(position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::setYawPitch(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = glm::clamp(pitch, m_minPitch, m_maxPitch);

    if (m_yaw > 360.0f) m_yaw -= 360.0f;
    if (m_yaw < 0.0f) m_yaw += 360.0f;

    updateViewMatrix();
}
