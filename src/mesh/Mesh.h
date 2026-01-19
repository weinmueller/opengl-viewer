#pragma once

#include <glad/gl.h>
#include "MeshData.h"
#include <memory>
#include <atomic>

class Mesh {
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Synchronous upload (blocks until complete) - existing behavior
    void upload(const MeshData& data);

    // Asynchronous upload for double-buffering
    void uploadAsync(const MeshData& data);

    // Call each frame to swap buffers when GPU upload is complete
    // Returns true if a swap occurred
    bool swapBuffers();

    // Check if there's a pending async upload
    bool hasPendingUpload() const { return m_writeIndex != m_readIndex; }

    void draw() const;
    void drawWireframe() const;

    bool isValid() const { return m_buffers[m_readIndex].vao != 0; }
    uint32_t getVertexCount() const { return m_vertexCount; }
    uint32_t getIndexCount() const { return m_indexCount; }

    const glm::vec3& getMinBounds() const { return m_minBounds; }
    const glm::vec3& getMaxBounds() const { return m_maxBounds; }
    glm::vec3 getCenter() const { return (m_minBounds + m_maxBounds) * 0.5f; }
    float getBoundingRadius() const { return glm::length(m_maxBounds - m_minBounds) * 0.5f; }

private:
    // Double-buffered GPU resources
    struct BufferSet {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsync fence = nullptr;
        uint32_t indexCount = 0;
    };

    void cleanupBufferSet(BufferSet& buf);
    void setupVertexAttributes(BufferSet& buf);

    BufferSet m_buffers[2];
    int m_writeIndex = 0;
    int m_readIndex = 0;

    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    glm::vec3 m_minBounds{0.0f};
    glm::vec3 m_maxBounds{0.0f};

    // Pending bounds from async upload
    glm::vec3 m_pendingMinBounds{0.0f};
    glm::vec3 m_pendingMaxBounds{0.0f};
    uint32_t m_pendingVertexCount = 0;
    uint32_t m_pendingIndexCount = 0;
};
