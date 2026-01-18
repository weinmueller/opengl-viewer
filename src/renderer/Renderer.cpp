#include "Renderer.h"
#include <glad/gl.h>

Renderer::Renderer() {
}

void Renderer::init() {
    m_meshShader = std::make_unique<Shader>("shaders/mesh.vert", "shaders/mesh.frag");
}

void Renderer::render(const Scene& scene, const Camera& camera, float aspectRatio) {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_meshShader->use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    m_meshShader->setMat4("view", view);
    m_meshShader->setMat4("projection", projection);
    m_meshShader->setVec3("viewPos", camera.getPosition());

    m_meshShader->setVec3("light.direction", glm::normalize(m_light.direction));
    m_meshShader->setVec3("light.color", m_light.color);
    m_meshShader->setFloat("light.ambient", m_light.ambient);
    m_meshShader->setFloat("light.diffuse", m_light.diffuse);
    m_meshShader->setFloat("light.specular", m_light.specular);

    for (const auto& obj : scene.getObjects()) {
        if (!obj->isVisible() || !obj->getMesh()) continue;

        m_meshShader->setMat4("model", obj->getModelMatrix());
        m_meshShader->setVec3("objectColor", obj->getColor());

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(obj->getModelMatrix())));
        m_meshShader->setMat3("normalMatrix", normalMatrix);

        if (m_wireframe) {
            obj->drawWireframe();
        } else {
            obj->draw();
        }
    }
}
