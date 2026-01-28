#pragma once

#include <glm/glm.hpp>
#include <limits>

struct BoundingBox {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    BoundingBox() = default;
    BoundingBox(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 getSize() const {
        return max - min;
    }

    float getRadius() const {
        return glm::length(getSize()) * 0.5f;
    }

    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void expand(const BoundingBox& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    void getCorners(glm::vec3 corners[8]) const {
        corners[0] = {min.x, min.y, min.z};
        corners[1] = {max.x, min.y, min.z};
        corners[2] = {min.x, max.y, min.z};
        corners[3] = {max.x, max.y, min.z};
        corners[4] = {min.x, min.y, max.z};
        corners[5] = {max.x, min.y, max.z};
        corners[6] = {min.x, max.y, max.z};
        corners[7] = {max.x, max.y, max.z};
    }

    BoundingBox transformed(const glm::mat4& transform) const {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {min.x, max.y, max.z},
            {max.x, max.y, max.z}
        };

        BoundingBox result;
        for (const auto& corner : corners) {
            glm::vec4 transformed = transform * glm::vec4(corner, 1.0f);
            result.expand(glm::vec3(transformed));
        }
        return result;
    }
};
