#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::vec3 minBounds{std::numeric_limits<float>::max()};
    glm::vec3 maxBounds{std::numeric_limits<float>::lowest()};

    void calculateBounds() {
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }

    glm::vec3 getCenter() const {
        return (minBounds + maxBounds) * 0.5f;
    }

    float getBoundingRadius() const {
        return glm::length(maxBounds - minBounds) * 0.5f;
    }

    void clear() {
        vertices.clear();
        indices.clear();
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    }

    bool empty() const {
        return vertices.empty();
    }
};
