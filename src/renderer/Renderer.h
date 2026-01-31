#pragma once

#include "Camera.h"
#include "core/Shader.h"
#include "core/Texture.h"
#include "scene/Scene.h"
#include "scene/Frustum.h"
#include "scene/BoundingBox.h"
#include "util/TextRenderer.h"
#include "ui/HelpOverlay.h"
#include "ui/ProgressOverlay.h"
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <memory>

class SubdivisionManager;
class LODManager;
class MultiPatchManager;

struct Light {
    glm::vec3 direction{-0.5f, -1.0f, -0.3f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float ambient{0.2f};
    float diffuse{0.8f};
    float specular{0.5f};
};

struct RimLight {
    glm::vec3 color{0.6f, 0.7f, 0.9f};  // Slight blue tint
    float strength{0.4f};
};

struct Background {
    glm::vec3 topColor{0.15f, 0.18f, 0.25f};     // Dark blue-gray
    glm::vec3 bottomColor{0.05f, 0.05f, 0.08f};  // Near black
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init(int width, int height, const std::string& defaultTexturePath = "assets/textures/default_grid.png");
    void render(const Scene& scene, const Camera& camera, float aspectRatio);
    void resize(int width, int height);

    // Picking - returns object index or -1 if nothing picked
    int pick(const Scene& scene, const Camera& camera, float aspectRatio, int mouseX, int mouseY);

    void setClearColor(const glm::vec3& color) { m_clearColor = color; }
    const glm::vec3& getClearColor() const { return m_clearColor; }
    void setWireframe(bool wireframe) { m_wireframe = wireframe; }
    bool isWireframe() const { return m_wireframe; }
    void toggleWireframe() { m_wireframe = !m_wireframe; }

    void setBackfaceCulling(bool enabled) { m_backfaceCulling = enabled; }
    bool isBackfaceCulling() const { return m_backfaceCulling; }
    void toggleBackfaceCulling() { m_backfaceCulling = !m_backfaceCulling; }

    void toggleHelpOverlay() { m_helpOverlay.toggle(); }
    bool isHelpVisible() const { return m_helpOverlay.isVisible(); }

    void setSubdivisionManager(SubdivisionManager* manager) { m_subdivisionManager = manager; }
    void setLODManager(LODManager* manager) { m_lodManager = manager; }
    void setMultiPatchManager(MultiPatchManager* manager) { m_multipatchManager = manager; }

    // LOD controls
    void setLODEnabled(bool enabled) { m_lodEnabled = enabled; }
    bool isLODEnabled() const { return m_lodEnabled; }
    void toggleLOD() { m_lodEnabled = !m_lodEnabled; }

    void setLODDebugColors(bool enabled) { m_lodDebugColors = enabled; }
    bool isLODDebugColors() const { return m_lodDebugColors; }
    void toggleLODDebugColors() { m_lodDebugColors = !m_lodDebugColors; }

    void setTexturesEnabled(bool enabled) { m_texturesEnabled = enabled; }
    bool isTexturesEnabled() const { return m_texturesEnabled; }
    void toggleTextures() { m_texturesEnabled = !m_texturesEnabled; }

    // Solution visualization controls
    void setSolutionVisualization(bool enabled) { m_showSolution = enabled; }
    bool isSolutionVisualization() const { return m_showSolution; }
    void toggleSolutionVisualization() { m_showSolution = !m_showSolution; }

    // Triangle stats for LOD display
    uint32_t getRenderedTriangles() const { return m_renderedTriangles; }
    uint32_t getOriginalTriangles() const { return m_originalTriangles; }
    float getLODSavingsPercent() const {
        if (m_originalTriangles == 0) return 0.0f;
        return 100.0f * (1.0f - static_cast<float>(m_renderedTriangles) / static_cast<float>(m_originalTriangles));
    }

    Light& getLight() { return m_light; }
    const Light& getLight() const { return m_light; }

    RimLight& getRimLight() { return m_rimLight; }
    const RimLight& getRimLight() const { return m_rimLight; }

    Background& getBackground() { return m_background; }
    const Background& getBackground() const { return m_background; }

    void setFrustumCulling(bool enabled) { m_frustumCulling = enabled; }
    bool isFrustumCulling() const { return m_frustumCulling; }
    void toggleFrustumCulling() { m_frustumCulling = !m_frustumCulling; }

    // Get last frame's culling stats
    int getVisibleObjects() const { return m_visibleObjects; }
    int getCulledObjects() const { return m_culledObjects; }

    // Check if a bounding box is visible in the current frustum
    bool isVisible(const BoundingBox& box) const { return m_frustum.isBoxVisible(box); }

private:
    void renderBackground();
    void initPickingFBO(int width, int height);
    void cleanupPickingFBO();

    std::unique_ptr<Shader> m_meshShader;
    std::unique_ptr<Shader> m_pickingShader;
    std::unique_ptr<Shader> m_backgroundShader;
    TextRenderer m_textRenderer;

    // Default texture for untextured objects
    std::unique_ptr<Texture> m_defaultTexture;

    // Background quad VAO/VBO
    GLuint m_backgroundVAO{0};
    GLuint m_backgroundVBO{0};

    // Picking framebuffer
    GLuint m_pickingFBO{0};
    GLuint m_pickingTexture{0};
    GLuint m_pickingDepth{0};
    int m_pickingWidth{0};
    int m_pickingHeight{0};

    glm::vec3 m_clearColor{0.1f, 0.1f, 0.15f};
    Light m_light;
    RimLight m_rimLight;
    Background m_background;
    bool m_wireframe{false};
    bool m_backfaceCulling{true};
    bool m_frustumCulling{true};

    Frustum m_frustum;
    int m_visibleObjects{0};
    int m_culledObjects{0};

    HelpOverlay m_helpOverlay;
    ProgressOverlay m_progressOverlay;
    SubdivisionManager* m_subdivisionManager{nullptr};
    LODManager* m_lodManager{nullptr};
    MultiPatchManager* m_multipatchManager{nullptr};

    bool m_lodEnabled{true};
    bool m_lodDebugColors{false};
    bool m_texturesEnabled{true};
    bool m_showSolution{false};

    // Triangle count stats
    uint32_t m_renderedTriangles{0};
    uint32_t m_originalTriangles{0};
};
