#include "Mesh.h"

Mesh::Mesh()
    : m_vao(0), m_vbo(0), m_ebo(0)
    , m_vertexCount(0), m_indexCount(0)
    , m_minBounds(0.0f), m_maxBounds(0.0f)
{
}

Mesh::~Mesh() {
    cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo)
    , m_vertexCount(other.m_vertexCount), m_indexCount(other.m_indexCount)
    , m_minBounds(other.m_minBounds), m_maxBounds(other.m_maxBounds)
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        m_minBounds = other.m_minBounds;
        m_maxBounds = other.m_maxBounds;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
    }
    return *this;
}

void Mesh::upload(const MeshData& data) {
    cleanup();

    if (data.empty()) return;

    m_vertexCount = static_cast<uint32_t>(data.vertices.size());
    m_indexCount = static_cast<uint32_t>(data.indices.size());
    m_minBounds = data.minBounds;
    m_maxBounds = data.maxBounds;

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);

    glNamedBufferStorage(m_vbo, data.vertices.size() * sizeof(Vertex),
                         data.vertices.data(), 0);
    glNamedBufferStorage(m_ebo, data.indices.size() * sizeof(uint32_t),
                         data.indices.data(), 0);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(m_vao, m_ebo);

    // Position attribute
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(m_vao, 0, 0);

    // Normal attribute
    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(m_vao, 1, 0);

    // TexCoord attribute
    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
    glVertexArrayAttribBinding(m_vao, 2, 0);
}

void Mesh::draw() const {
    if (!isValid()) return;

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawWireframe() const {
    if (!isValid()) return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Mesh::cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
}
