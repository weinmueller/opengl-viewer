#include "Renderer.h"

Renderer::Renderer() {
}

Renderer::~Renderer() {
    cleanupPickingFBO();
    if (m_backgroundVAO) {
        glDeleteVertexArrays(1, &m_backgroundVAO);
    }
    if (m_backgroundVBO) {
        glDeleteBuffers(1, &m_backgroundVBO);
    }
}

void Renderer::init(int width, int height) {
    m_meshShader = std::make_unique<Shader>("shaders/mesh.vert", "shaders/mesh.frag");
    m_pickingShader = std::make_unique<Shader>("shaders/picking.vert", "shaders/picking.frag");
    m_backgroundShader = std::make_unique<Shader>("shaders/background.vert", "shaders/background.frag");

    // Create full-screen quad for background
    float quadVertices[] = {
        // positions (NDC)
        -1.0f,  1.0f,  // top-left
        -1.0f, -1.0f,  // bottom-left
         1.0f, -1.0f,  // bottom-right

        -1.0f,  1.0f,  // top-left
         1.0f, -1.0f,  // bottom-right
         1.0f,  1.0f,  // top-right
    };

    glCreateVertexArrays(1, &m_backgroundVAO);
    glCreateBuffers(1, &m_backgroundVBO);

    glNamedBufferStorage(m_backgroundVBO, sizeof(quadVertices), quadVertices, 0);

    glVertexArrayVertexBuffer(m_backgroundVAO, 0, m_backgroundVBO, 0, 2 * sizeof(float));
    glEnableVertexArrayAttrib(m_backgroundVAO, 0);
    glVertexArrayAttribFormat(m_backgroundVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_backgroundVAO, 0, 0);

    initPickingFBO(width, height);
    m_helpOverlay.init();
}

void Renderer::initPickingFBO(int width, int height) {
    cleanupPickingFBO();

    m_pickingWidth = width;
    m_pickingHeight = height;

    // Create framebuffer
    glCreateFramebuffers(1, &m_pickingFBO);

    // Create color texture
    glCreateTextures(GL_TEXTURE_2D, 1, &m_pickingTexture);
    glTextureStorage2D(m_pickingTexture, 1, GL_RGB8, width, height);
    glTextureParameteri(m_pickingTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_pickingTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Create depth renderbuffer
    glCreateRenderbuffers(1, &m_pickingDepth);
    glNamedRenderbufferStorage(m_pickingDepth, GL_DEPTH_COMPONENT24, width, height);

    // Attach to framebuffer
    glNamedFramebufferTexture(m_pickingFBO, GL_COLOR_ATTACHMENT0, m_pickingTexture, 0);
    glNamedFramebufferRenderbuffer(m_pickingFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pickingDepth);

    // Check framebuffer status
    GLenum status = glCheckNamedFramebufferStatus(m_pickingFBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // Handle error
    }
}

void Renderer::cleanupPickingFBO() {
    if (m_pickingFBO) {
        glDeleteFramebuffers(1, &m_pickingFBO);
        m_pickingFBO = 0;
    }
    if (m_pickingTexture) {
        glDeleteTextures(1, &m_pickingTexture);
        m_pickingTexture = 0;
    }
    if (m_pickingDepth) {
        glDeleteRenderbuffers(1, &m_pickingDepth);
        m_pickingDepth = 0;
    }
}

void Renderer::resize(int width, int height) {
    if (width > 0 && height > 0) {
        initPickingFBO(width, height);
    }
}

void Renderer::render(const Scene& scene, const Camera& camera, float aspectRatio) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render gradient background first
    renderBackground();

    // Apply backface culling setting
    if (m_backfaceCulling) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    m_meshShader->use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    m_meshShader->setMat4("view", view);
    m_meshShader->setMat4("projection", projection);
    m_meshShader->setVec3("viewPos", camera.getPosition());

    // Main light
    m_meshShader->setVec3("light.direction", glm::normalize(m_light.direction));
    m_meshShader->setVec3("light.color", m_light.color);
    m_meshShader->setFloat("light.ambient", m_light.ambient);
    m_meshShader->setFloat("light.diffuse", m_light.diffuse);
    m_meshShader->setFloat("light.specular", m_light.specular);

    // Rim lighting
    m_meshShader->setFloat("rimStrength", m_rimLight.strength);
    m_meshShader->setVec3("rimColor", m_rimLight.color);

    size_t index = 0;
    for (const auto& obj : scene.getObjects()) {
        if (!obj->isVisible() || !obj->getMesh()) {
            ++index;
            continue;
        }

        m_meshShader->setMat4("model", obj->getModelMatrix());

        // Highlight selected objects
        glm::vec3 color = obj->getColor();
        if (obj->isSelected()) {
            color = glm::mix(color, glm::vec3(1.0f, 0.5f, 0.0f), 0.5f);
        }
        m_meshShader->setVec3("objectColor", color);

        m_meshShader->setMat3("normalMatrix", obj->getNormalMatrix());

        if (m_wireframe) {
            obj->drawWireframe();
        } else {
            obj->draw();
        }
        ++index;
    }

    // Render help overlay on top
    m_helpOverlay.render(m_pickingWidth, m_pickingHeight);
}

int Renderer::pick(const Scene& scene, const Camera& camera, float aspectRatio, int mouseX, int mouseY) {
    // Bind picking framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickingFBO);
    glViewport(0, 0, m_pickingWidth, m_pickingHeight);

    // Clear with background color (ID 0 = no object)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pickingShader->use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    m_pickingShader->setMat4("view", view);
    m_pickingShader->setMat4("projection", projection);

    // Render each object with its ID as color (ID starts at 1, 0 = background)
    uint32_t objectID = 1;
    for (const auto& obj : scene.getObjects()) {
        if (!obj->isVisible() || !obj->getMesh()) {
            ++objectID;
            continue;
        }

        m_pickingShader->setMat4("model", obj->getModelMatrix());
        glUniform1ui(glGetUniformLocation(m_pickingShader->getProgram(), "objectID"), objectID);

        obj->draw();
        ++objectID;
    }

    // Read pixel at mouse position (flip Y coordinate)
    int readX = mouseX;
    int readY = m_pickingHeight - mouseY - 1;

    if (readX < 0 || readX >= m_pickingWidth || readY < 0 || readY >= m_pickingHeight) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return -1;
    }

    unsigned char pixel[3];
    glReadPixels(readX, readY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    // Decode object ID from color
    uint32_t pickedID = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);

    // Unbind framebuffer and restore viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_pickingWidth, m_pickingHeight);

    // Return object index (0-based) or -1 if background
    if (pickedID == 0) {
        return -1;
    }
    return static_cast<int>(pickedID - 1);
}

void Renderer::renderBackground() {
    // Disable depth writing for background
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    m_backgroundShader->use();
    m_backgroundShader->setVec3("topColor", m_background.topColor);
    m_backgroundShader->setVec3("bottomColor", m_background.bottomColor);

    glBindVertexArray(m_backgroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);  // Full-screen quad (2 triangles)
    glBindVertexArray(0);

    // Re-enable depth for scene rendering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}
