#include "Mesh.h"

Mesh::Mesh() {
    // Both buffer sets start empty
}

Mesh::~Mesh() {
    cleanupBufferSet(m_buffers[0]);
    cleanupBufferSet(m_buffers[1]);
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_buffers{std::move(other.m_buffers[0]), std::move(other.m_buffers[1])}
    , m_writeIndex(other.m_writeIndex)
    , m_readIndex(other.m_readIndex)
    , m_vertexCount(other.m_vertexCount)
    , m_indexCount(other.m_indexCount)
    , m_minBounds(other.m_minBounds)
    , m_maxBounds(other.m_maxBounds)
    , m_pendingMinBounds(other.m_pendingMinBounds)
    , m_pendingMaxBounds(other.m_pendingMaxBounds)
    , m_pendingVertexCount(other.m_pendingVertexCount)
    , m_pendingIndexCount(other.m_pendingIndexCount)
{
    other.m_buffers[0] = BufferSet{};
    other.m_buffers[1] = BufferSet{};
    other.m_writeIndex = 0;
    other.m_readIndex = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanupBufferSet(m_buffers[0]);
        cleanupBufferSet(m_buffers[1]);

        m_buffers[0] = std::move(other.m_buffers[0]);
        m_buffers[1] = std::move(other.m_buffers[1]);
        m_writeIndex = other.m_writeIndex;
        m_readIndex = other.m_readIndex;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        m_minBounds = other.m_minBounds;
        m_maxBounds = other.m_maxBounds;
        m_pendingMinBounds = other.m_pendingMinBounds;
        m_pendingMaxBounds = other.m_pendingMaxBounds;
        m_pendingVertexCount = other.m_pendingVertexCount;
        m_pendingIndexCount = other.m_pendingIndexCount;

        other.m_buffers[0] = BufferSet{};
        other.m_buffers[1] = BufferSet{};
        other.m_writeIndex = 0;
        other.m_readIndex = 0;
    }
    return *this;
}

void Mesh::cleanupBufferSet(BufferSet& buf) {
    if (buf.fence) {
        glDeleteSync(buf.fence);
        buf.fence = nullptr;
    }
    if (buf.vao) {
        glDeleteVertexArrays(1, &buf.vao);
        buf.vao = 0;
    }
    if (buf.vbo) {
        glDeleteBuffers(1, &buf.vbo);
        buf.vbo = 0;
    }
    if (buf.ebo) {
        glDeleteBuffers(1, &buf.ebo);
        buf.ebo = 0;
    }
    buf.indexCount = 0;
}

void Mesh::setupVertexAttributes(BufferSet& buf) {
    glVertexArrayVertexBuffer(buf.vao, 0, buf.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(buf.vao, buf.ebo);

    // Position attribute
    glEnableVertexArrayAttrib(buf.vao, 0);
    glVertexArrayAttribFormat(buf.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(buf.vao, 0, 0);

    // Normal attribute
    glEnableVertexArrayAttrib(buf.vao, 1);
    glVertexArrayAttribFormat(buf.vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(buf.vao, 1, 0);

    // TexCoord attribute
    glEnableVertexArrayAttrib(buf.vao, 2);
    glVertexArrayAttribFormat(buf.vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
    glVertexArrayAttribBinding(buf.vao, 2, 0);

    // Solution value attribute (for Poisson visualization)
    glEnableVertexArrayAttrib(buf.vao, 3);
    glVertexArrayAttribFormat(buf.vao, 3, 1, GL_FLOAT, GL_FALSE, offsetof(Vertex, solutionValue));
    glVertexArrayAttribBinding(buf.vao, 3, 0);
}

void Mesh::upload(const MeshData& data) {
    // For synchronous upload, we upload to the read buffer directly
    cleanupBufferSet(m_buffers[m_readIndex]);

    if (data.empty()) return;

    BufferSet& buf = m_buffers[m_readIndex];

    glCreateVertexArrays(1, &buf.vao);
    glCreateBuffers(1, &buf.vbo);
    glCreateBuffers(1, &buf.ebo);

    glNamedBufferStorage(buf.vbo, data.vertices.size() * sizeof(Vertex),
                         data.vertices.data(), 0);
    glNamedBufferStorage(buf.ebo, data.indices.size() * sizeof(uint32_t),
                         data.indices.data(), 0);

    setupVertexAttributes(buf);

    buf.indexCount = static_cast<uint32_t>(data.indices.size());
    m_vertexCount = static_cast<uint32_t>(data.vertices.size());
    m_indexCount = buf.indexCount;
    m_minBounds = data.minBounds;
    m_maxBounds = data.maxBounds;

    // Ensure write index matches read index (no pending upload)
    m_writeIndex = m_readIndex;
}

void Mesh::uploadAsync(const MeshData& data) {
    if (data.empty()) return;

    // Select the write buffer (the one not currently being rendered)
    int writeIdx = (m_readIndex + 1) % 2;
    BufferSet& buf = m_buffers[writeIdx];

    // If there's a previous pending upload on this buffer, wait briefly for it
    if (buf.fence) {
        // Wait up to 5ms — if GPU is still busy, we'll just recreate the buffer
        glClientWaitSync(buf.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 5000000ULL); // 5ms in nanoseconds
        glDeleteSync(buf.fence);
        buf.fence = nullptr;
    }

    // Clean up old resources
    cleanupBufferSet(buf);

    // Create new buffers
    glCreateVertexArrays(1, &buf.vao);
    glCreateBuffers(1, &buf.vbo);
    glCreateBuffers(1, &buf.ebo);

    glNamedBufferStorage(buf.vbo, data.vertices.size() * sizeof(Vertex),
                         data.vertices.data(), 0);
    glNamedBufferStorage(buf.ebo, data.indices.size() * sizeof(uint32_t),
                         data.indices.data(), 0);

    setupVertexAttributes(buf);

    buf.indexCount = static_cast<uint32_t>(data.indices.size());

    // Insert fence to track GPU completion
    buf.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // Store pending state
    m_pendingMinBounds = data.minBounds;
    m_pendingMaxBounds = data.maxBounds;
    m_pendingVertexCount = static_cast<uint32_t>(data.vertices.size());
    m_pendingIndexCount = buf.indexCount;

    // Mark write buffer as pending
    m_writeIndex = writeIdx;
}

bool Mesh::swapBuffers() {
    if (m_writeIndex == m_readIndex) {
        // No pending upload
        return false;
    }

    BufferSet& buf = m_buffers[m_writeIndex];

    // Check if GPU finished uploading
    if (buf.fence) {
        GLenum result = glClientWaitSync(buf.fence, 0, 0);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
            glDeleteSync(buf.fence);
            buf.fence = nullptr;

            // Swap: make write buffer the new read buffer
            m_readIndex = m_writeIndex;
            m_minBounds = m_pendingMinBounds;
            m_maxBounds = m_pendingMaxBounds;
            m_vertexCount = m_pendingVertexCount;
            m_indexCount = m_pendingIndexCount;

            return true;
        }
    }

    return false;
}

void Mesh::draw() const {
    const BufferSet& buf = m_buffers[m_readIndex];
    if (!buf.vao) return;

    glBindVertexArray(buf.vao);
    glDrawElements(GL_TRIANGLES, buf.indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawWireframe() const {
    if (!m_buffers[m_readIndex].vao) return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
