#pragma once

#include "BoundingBox.h"
#include <glm/glm.hpp>
#include <array>

// Represents a view frustum with 6 planes for culling
class Frustum {
public:
    enum Plane {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    Frustum() = default;

    // Extract frustum planes from view-projection matrix
    void update(const glm::mat4& viewProjection) {
        // Extract planes using Gribb/Hartmann method
        // Each plane is stored as (a, b, c, d) where ax + by + cz + d = 0

        // Left plane
        m_planes[Left] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][0],
            viewProjection[1][3] + viewProjection[1][0],
            viewProjection[2][3] + viewProjection[2][0],
            viewProjection[3][3] + viewProjection[3][0]
        );

        // Right plane
        m_planes[Right] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][0],
            viewProjection[1][3] - viewProjection[1][0],
            viewProjection[2][3] - viewProjection[2][0],
            viewProjection[3][3] - viewProjection[3][0]
        );

        // Bottom plane
        m_planes[Bottom] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][1],
            viewProjection[1][3] + viewProjection[1][1],
            viewProjection[2][3] + viewProjection[2][1],
            viewProjection[3][3] + viewProjection[3][1]
        );

        // Top plane
        m_planes[Top] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][1],
            viewProjection[1][3] - viewProjection[1][1],
            viewProjection[2][3] - viewProjection[2][1],
            viewProjection[3][3] - viewProjection[3][1]
        );

        // Near plane
        m_planes[Near] = glm::vec4(
            viewProjection[0][3] + viewProjection[0][2],
            viewProjection[1][3] + viewProjection[1][2],
            viewProjection[2][3] + viewProjection[2][2],
            viewProjection[3][3] + viewProjection[3][2]
        );

        // Far plane
        m_planes[Far] = glm::vec4(
            viewProjection[0][3] - viewProjection[0][2],
            viewProjection[1][3] - viewProjection[1][2],
            viewProjection[2][3] - viewProjection[2][2],
            viewProjection[3][3] - viewProjection[3][2]
        );

        // Normalize all planes
        for (auto& plane : m_planes) {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) {
                plane /= length;
            }
        }
    }

    // Test if an AABB is inside or intersecting the frustum
    // Returns true if the box should be rendered (inside or intersecting)
    bool isBoxVisible(const BoundingBox& box) const {
        for (const auto& plane : m_planes) {
            // Find the positive vertex (furthest along plane normal)
            glm::vec3 positive = box.min;
            if (plane.x >= 0) positive.x = box.max.x;
            if (plane.y >= 0) positive.y = box.max.y;
            if (plane.z >= 0) positive.z = box.max.z;

            // If positive vertex is outside, entire box is outside
            float distance = glm::dot(glm::vec3(plane), positive) + plane.w;
            if (distance < 0) {
                return false;
            }
        }
        return true;
    }

    // Test if a sphere is visible (useful for quick rejection)
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : m_planes) {
            float distance = glm::dot(glm::vec3(plane), center) + plane.w;
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<glm::vec4, Count> m_planes;
};
