#include "SceneObject.h"

SceneObject::SceneObject(const std::string& name)
    : m_name(name)
{
    updateModelMatrix();
}

void SceneObject::setMesh(std::shared_ptr<Mesh> mesh) {
    m_mesh = mesh;
    if (m_mesh) {
        m_localBounds = BoundingBox(m_mesh->getMinBounds(), m_mesh->getMaxBounds());
        updateWorldBounds();
    }
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
}

void SceneObject::updateWorldBounds() {
    if (m_localBounds.isValid()) {
        m_worldBounds = m_localBounds.transformed(m_modelMatrix);
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
