#pragma once

#include "Camera.h"
#include "core/Shader.h"
#include "scene/Scene.h"
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

    Light& getLight() { return m_light; }
    const Light& getLight() const { return m_light; }

private:
    void initPickingFBO(int width, int height);
    void cleanupPickingFBO();

    std::unique_ptr<Shader> m_meshShader;
    std::unique_ptr<Shader> m_pickingShader;

    // Picking framebuffer
    GLuint m_pickingFBO{0};
    GLuint m_pickingTexture{0};
    GLuint m_pickingDepth{0};
    int m_pickingWidth{0};
    int m_pickingHeight{0};

    glm::vec3 m_clearColor{0.1f, 0.1f, 0.15f};
    Light m_light;
    bool m_wireframe{false};
};
