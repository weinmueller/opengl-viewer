#include "Application.h"
#include "mesh/MeshLoader.h"
#include "mesh/MeshData.h"
#include <iostream>
#include <GLFW/glfw3.h>

Application::Application(int width, int height, const std::string& title)
    : m_camera(5.0f)
{
    m_window = std::make_unique<Window>(width, height, title);
    m_renderer = std::make_unique<Renderer>();
    m_renderer->init();

    setupCallbacks();
}

void Application::setupCallbacks() {
    m_window->setKeyCallback([this](int key, int scancode, int action, int mods) {
        onKeyPressed(key, scancode, action, mods);
    });

    m_window->setMouseButtonCallback([this](int button, int action, int mods) {
        onMouseButton(button, action, mods);
    });

    m_window->setCursorPosCallback([this](double xpos, double ypos) {
        onMouseMove(xpos, ypos);
    });

    m_window->setScrollCallback([this](double xoffset, double yoffset) {
        onScroll(xoffset, yoffset);
    });

    m_window->setResizeCallback([this](int width, int height) {
        onResize(width, height);
    });
}

int Application::run(const std::string& meshPath) {
    if (!meshPath.empty()) {
        if (!loadMesh(meshPath)) {
            std::cerr << "Failed to load mesh: " << meshPath << std::endl;
            return 1;
        }
        focusOnScene();
    }

    while (!m_window->shouldClose()) {
        m_timer.update();
        processInput();
        update(m_timer.getDeltaTimeF());
        render();
        m_window->swapBuffers();
        m_window->pollEvents();
    }

    return 0;
}

void Application::processInput() {
    if (m_window->isKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_window->getHandle(), true);
    }
}

void Application::update(float deltaTime) {
    (void)deltaTime;
}

void Application::render() {
    m_renderer->render(m_scene, m_camera, m_window->getAspectRatio());
}

void Application::onKeyPressed(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_W:
                m_renderer->toggleWireframe();
                break;
            case GLFW_KEY_F:
                focusOnScene();
                break;
        }
    }
}

void Application::onMouseButton(int button, int action, int mods) {
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_leftMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        m_middleMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        m_rightMouseDown = (action == GLFW_PRESS);
    }

    m_window->getCursorPos(m_lastMouseX, m_lastMouseY);
}

void Application::onMouseMove(double xpos, double ypos) {
    double deltaX = xpos - m_lastMouseX;
    double deltaY = ypos - m_lastMouseY;

    if (m_leftMouseDown) {
        m_camera.orbit(static_cast<float>(-deltaX), static_cast<float>(deltaY));
    }

    if (m_middleMouseDown) {
        m_camera.pan(static_cast<float>(deltaX), static_cast<float>(deltaY));
    }

    m_lastMouseX = xpos;
    m_lastMouseY = ypos;
}

void Application::onScroll(double xoffset, double yoffset) {
    (void)xoffset;
    m_camera.zoom(static_cast<float>(yoffset));
}

void Application::onResize(int width, int height) {
    (void)width;
    (void)height;
}

bool Application::loadMesh(const std::string& path) {
    auto loader = MeshLoader::createForFile(path);
    if (!loader) {
        std::cerr << "No loader available for: " << path << std::endl;
        return false;
    }

    MeshData meshData;
    if (!loader->load(path, meshData)) {
        return false;
    }

    auto mesh = std::make_shared<Mesh>();
    mesh->upload(meshData);

    SceneObject* obj = m_scene.addObject("LoadedMesh");
    obj->setMesh(mesh);

    return true;
}

void Application::focusOnScene() {
    glm::vec3 center = m_scene.getSceneCenter();
    float radius = m_scene.getSceneRadius();

    m_camera.setTarget(center);
    if (radius > 0.0f) {
        m_camera.setDistance(radius * 2.5f);
    }
}
