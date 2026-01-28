#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string texturePath;

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
        texturePath.clear();
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    }

    bool empty() const {
        return vertices.empty();
    }

    // Recalculate vertex normals from face geometry
    void recalculateNormals() {
        // Reset all normals to zero
        for (auto& vertex : vertices) {
            vertex.normal = glm::vec3(0.0f);
        }

        // Accumulate face normals to vertices
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3 v0 = vertices[i0].position;
            glm::vec3 v1 = vertices[i1].position;
            glm::vec3 v2 = vertices[i2].position;

            // Cross product gives area-weighted normal
            glm::vec3 faceNormal = glm::cross(v1 - v0, v2 - v0);
            vertices[i0].normal += faceNormal;
            vertices[i1].normal += faceNormal;
            vertices[i2].normal += faceNormal;
        }

        // Normalize all vertex normals
        for (auto& vertex : vertices) {
            float len = glm::length(vertex.normal);
            if (len > 1e-10f) {
                vertex.normal /= len;
            }
        }
    }
};
