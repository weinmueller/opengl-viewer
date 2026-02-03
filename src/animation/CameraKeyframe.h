#pragma once

#include <glm/glm.hpp>

struct CameraKeyframe {
    float time{0.0f};           // Time in seconds
    glm::vec3 target{0.0f};     // Orbit center
    float distance{5.0f};       // Distance from target
    float yaw{0.0f};            // Rotation yaw (degrees)
    float pitch{30.0f};         // Rotation pitch (degrees)
    float fov{45.0f};           // Field of view (degrees)

    static CameraKeyframe lerp(const CameraKeyframe& a, const CameraKeyframe& b, float t) {
        CameraKeyframe result;
        result.time = glm::mix(a.time, b.time, t);
        result.target = glm::mix(a.target, b.target, t);
        result.distance = glm::mix(a.distance, b.distance, t);
        result.yaw = glm::mix(a.yaw, b.yaw, t);
        result.pitch = glm::mix(a.pitch, b.pitch, t);
        result.fov = glm::mix(a.fov, b.fov, t);
        return result;
    }
};
