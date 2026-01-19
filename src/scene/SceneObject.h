#pragma once

#include "BoundingBox.h"
#include "mesh/Mesh.h"
#include "mesh/MeshData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

class SceneObject {
public:
    SceneObject(const std::string& name = "Object");

    void setMesh(std::shared_ptr<Mesh> mesh);
    void setMeshData(const MeshData& data);
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& eulerAngles);
    void setScale(const glm::vec3& scale);
    void setColor(const glm::vec3& color) { m_color = color; }

    // Subdivision
    void subdivide(bool smooth = true, float creaseAngle = 30.0f);
    bool canSubdivide() const { return !m_meshData.empty(); }

    const std::string& getName() const { return m_name; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }
    const glm::vec3& getColor() const { return m_color; }

    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }
    const BoundingBox& getWorldBounds() const { return m_worldBounds; }
    Mesh* getMesh() const { return m_mesh.get(); }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

    void draw() const;
    void drawWireframe() const;

    // Call each frame to check for completed async GPU uploads
    void update();

private:
    void updateModelMatrix();
    void updateWorldBounds();

    std::string m_name;
    std::shared_ptr<Mesh> m_mesh;
    MeshData m_meshData;

    glm::vec3 m_position{0.0f};
    glm::vec3 m_rotation{0.0f};
    glm::vec3 m_scale{1.0f};
    glm::vec3 m_color{0.8f, 0.8f, 0.8f};

    glm::mat4 m_modelMatrix{1.0f};
    BoundingBox m_localBounds;
    BoundingBox m_worldBounds;
    bool m_visible{true};
    bool m_selected{false};
};
