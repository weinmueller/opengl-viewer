#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance = 5.0f);

    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);

    void setTarget(const glm::vec3& target) { m_target = target; updateViewMatrix(); }
    void setDistance(float distance) { m_distance = glm::clamp(distance, m_minDistance, m_maxDistance); updateViewMatrix(); }

    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    glm::vec3 getPosition() const;
    const glm::vec3& getTarget() const { return m_target; }

    void setFOV(float fov) { m_fov = fov; }
    void setNearFar(float near, float far) { m_near = near; m_far = far; }
    void setOrbitSensitivity(float sensitivity) { m_orbitSensitivity = sensitivity; }
    void setPanSensitivity(float sensitivity) { m_panSensitivity = sensitivity; }
    void setZoomSensitivity(float sensitivity) { m_zoomSensitivity = sensitivity; }

private:
    void updateViewMatrix();

    glm::vec3 m_target;
    float m_distance;
    float m_yaw;
    float m_pitch;

    float m_fov;
    float m_near;
    float m_far;
    float m_minDistance;
    float m_maxDistance;
    float m_minPitch;
    float m_maxPitch;

    float m_orbitSensitivity;
    float m_panSensitivity;
    float m_zoomSensitivity;

    glm::mat4 m_viewMatrix;
};
