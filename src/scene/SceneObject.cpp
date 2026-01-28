#include "SceneObject.h"
#include "geometry/Subdivision.h"
#include <iostream>

SceneObject::SceneObject(const std::string& name)
    : m_name(name)
{
    updateModelMatrix();
}

void SceneObject::setMesh(std::unique_ptr<Mesh> mesh) {
    m_mesh = std::move(mesh);
    if (m_mesh) {
        m_localBounds = BoundingBox(m_mesh->getMinBounds(), m_mesh->getMaxBounds());
        updateWorldBounds();
    }
}

void SceneObject::setMeshData(const MeshData& data) {
    m_meshData = data;

    // Also upload to GPU
    m_mesh = std::make_unique<Mesh>();
    m_mesh->upload(m_meshData);

    m_localBounds = BoundingBox(m_mesh->getMinBounds(), m_mesh->getMaxBounds());
    updateWorldBounds();

    // Load texture if specified
    if (!m_meshData.texturePath.empty()) {
        m_texture = std::make_unique<Texture>();
        if (!m_texture->load(m_meshData.texturePath)) {
            m_texture.reset();
        }
    }
}

void SceneObject::subdivide(bool smooth, float creaseAngle) {
    if (m_meshData.empty()) {
        return;
    }

    // Apply subdivision
    if (smooth) {
        m_meshData = Subdivision::loopSubdivide(m_meshData, creaseAngle);
    } else {
        m_meshData = Subdivision::midpointSubdivide(m_meshData);
    }

    // Re-upload to GPU
    m_mesh = std::make_unique<Mesh>();
    m_mesh->upload(m_meshData);

    m_localBounds = BoundingBox(m_mesh->getMinBounds(), m_mesh->getMaxBounds());
    updateWorldBounds();
}

void SceneObject::applySubdividedMesh(MeshData&& data) {
    m_meshData = std::move(data);

    // Use async upload for double-buffering (GPU upload on main thread)
    if (!m_mesh) {
        m_mesh = std::make_unique<Mesh>();
    }
    m_mesh->uploadAsync(m_meshData);

    // Update bounds immediately from mesh data
    m_localBounds = BoundingBox(m_meshData.minBounds, m_meshData.maxBounds);
    updateWorldBounds();

    // Clear existing LOD since mesh has changed - needs regeneration
    m_lodMesh.clear();
    m_needsLODRegeneration = true;
}

void SceneObject::setPosition(const glm::vec3& position) {
    m_position = position;
    updateModelMatrix();
    updateWorldBounds();
}

void SceneObject::setRotation(const glm::vec3& eulerAngles) {
    m_rotation = eulerAngles;
    updateModelMatrix();
    updateWorldBounds();
}

void SceneObject::setScale(const glm::vec3& scale) {
    m_scale = scale;
    updateModelMatrix();
    updateWorldBounds();
}

void SceneObject::updateModelMatrix() {
    m_modelMatrix = glm::mat4(1.0f);
    m_modelMatrix = glm::translate(m_modelMatrix, m_position);
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
    m_modelMatrix = glm::scale(m_modelMatrix, m_scale);

    // Cache normal matrix (expensive inverse only when transform changes)
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_modelMatrix)));
}

void SceneObject::updateWorldBounds() {
    if (m_localBounds.isValid()) {
        m_worldBounds = m_localBounds.transformed(m_modelMatrix);
    }
}

void SceneObject::update() {
    if (m_mesh && m_mesh->swapBuffers()) {
        // Buffer swap occurred, update bounds
        m_localBounds = BoundingBox(m_mesh->getMinBounds(), m_mesh->getMaxBounds());
        updateWorldBounds();
    }
}

void SceneObject::draw() const {
    if (m_visible && m_mesh) {
        m_mesh->draw();
    }
}

void SceneObject::drawWireframe() const {
    if (m_visible && m_mesh) {
        m_mesh->drawWireframe();
    }
}

void SceneObject::applyLODLevels(std::vector<LODLevel>&& levels) {
    m_lodMesh.setLevels(std::move(levels));
}

Mesh* SceneObject::getMeshForRendering(float screenSize) {
    if (m_lodMesh.hasLOD()) {
        return m_lodMesh.selectLOD(screenSize);
    }
    return m_mesh.get();
}
