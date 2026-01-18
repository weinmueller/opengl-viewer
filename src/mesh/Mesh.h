#pragma once

#include <glad/gl.h>
#include "MeshData.h"
#include <memory>

class Mesh {
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const MeshData& data);
    void draw() const;
    void drawWireframe() const;

    bool isValid() const { return m_vao != 0; }
    uint32_t getVertexCount() const { return m_vertexCount; }
    uint32_t getIndexCount() const { return m_indexCount; }

    const glm::vec3& getMinBounds() const { return m_minBounds; }
    const glm::vec3& getMaxBounds() const { return m_maxBounds; }
    glm::vec3 getCenter() const { return (m_minBounds + m_maxBounds) * 0.5f; }
    float getBoundingRadius() const { return glm::length(m_maxBounds - m_minBounds) * 0.5f; }

private:
    void cleanup();

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    glm::vec3 m_minBounds;
    glm::vec3 m_maxBounds;
};
