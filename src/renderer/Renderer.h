#pragma once

#include "Camera.h"
#include "core/Shader.h"
#include "scene/Scene.h"
#include "ui/HelpOverlay.h"
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <memory>

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

    void init(int width, int height);
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

    Light& getLight() { return m_light; }
    const Light& getLight() const { return m_light; }

    RimLight& getRimLight() { return m_rimLight; }
    const RimLight& getRimLight() const { return m_rimLight; }

    Background& getBackground() { return m_background; }
    const Background& getBackground() const { return m_background; }

private:
    void renderBackground();
    void initPickingFBO(int width, int height);
    void cleanupPickingFBO();

    std::unique_ptr<Shader> m_meshShader;
    std::unique_ptr<Shader> m_pickingShader;
    std::unique_ptr<Shader> m_backgroundShader;

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

    HelpOverlay m_helpOverlay;
};
