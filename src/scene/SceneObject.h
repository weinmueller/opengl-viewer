#pragma once

#include "BoundingBox.h"
#include "mesh/Mesh.h"
#include "mesh/MeshData.h"
#include "lod/LODMesh.h"
#include "lod/LODLevel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>

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
    void subdivide(bool smooth = true, float creaseAngle = 180.0f);
    bool canSubdivide() const { return !m_meshData.empty(); }

    // Apply pre-computed subdivision result (for background threading)
    void applySubdividedMesh(MeshData&& data);

    // Get current mesh data (for background subdivision)
    const MeshData& getMeshData() const { return m_meshData; }

    // LOD support
    void applyLODLevels(std::vector<LODLevel>&& levels);
    LODMesh& getLODMesh() { return m_lodMesh; }
    const LODMesh& getLODMesh() const { return m_lodMesh; }
    bool hasLOD() const { return m_lodMesh.hasLOD(); }

    // Get mesh for rendering based on screen size (uses LOD if available)
    Mesh* getMeshForRendering(float screenSize);

    // Get current LOD index (-1 if no LOD)
    int getCurrentLODIndex() const { return m_lodMesh.hasLOD() ? m_lodMesh.getCurrentLODIndex() : -1; }

    // Check if LOD needs regeneration (after subdivision)
    bool needsLODRegeneration() const { return m_needsLODRegeneration; }
    void clearLODRegenerationFlag() { m_needsLODRegeneration = false; }

    const std::string& getName() const { return m_name; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }
    const glm::vec3& getColor() const { return m_color; }

    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }
    const glm::mat3& getNormalMatrix() const { return m_normalMatrix; }
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
    LODMesh m_lodMesh;

    glm::vec3 m_position{0.0f};
    glm::vec3 m_rotation{0.0f};
    glm::vec3 m_scale{1.0f};
    glm::vec3 m_color{0.8f, 0.8f, 0.8f};

    glm::mat4 m_modelMatrix{1.0f};
    glm::mat3 m_normalMatrix{1.0f};
    BoundingBox m_localBounds;
    BoundingBox m_worldBounds;
    bool m_visible{true};
    bool m_selected{false};
    bool m_needsLODRegeneration{false};
};
