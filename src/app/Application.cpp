#include "Application.h"
#include "mesh/MeshLoader.h"
#include "mesh/MeshData.h"
#include "async/LODTask.h"
#include "async/SubdivisionTask.h"
#include <iostream>
#include <algorithm>
#include <GLFW/glfw3.h>

Application::Application(int width, int height, const std::string& title,
                         float creaseAngle, const std::string& defaultTexture)
    : m_camera(5.0f)
    , m_creaseAngle(creaseAngle)
    , m_defaultTexturePath(defaultTexture)
{
    m_window = std::make_unique<Window>(width, height, title);
    m_renderer = std::make_unique<Renderer>();
    m_renderer->init(width, height, m_defaultTexturePath);
    m_subdivisionManager = std::make_unique<SubdivisionManager>();
    m_lodManager = std::make_unique<LODManager>();
    m_multipatchManager = std::make_unique<MultiPatchManager>();

    // Pass managers to renderer for progress display
    m_renderer->setSubdivisionManager(m_subdivisionManager.get());
    m_renderer->setLODManager(m_lodManager.get());
    m_renderer->setMultiPatchManager(m_multipatchManager.get());

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

int Application::run(const std::vector<std::string>& meshPaths) {
    for (const auto& path : meshPaths) {
        if (!loadMesh(path)) {
            std::cerr << "Failed to load mesh: " << path << std::endl;
        }
    }

    if (m_scene.getObjectCount() > 0) {
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
    // Arrow keys for camera orbit (continuous while held)
    const float orbitSpeed = 2.0f;

    if (m_window->isKeyPressed(GLFW_KEY_LEFT)) {
        m_camera.orbit(orbitSpeed, 0.0f);
    }
    if (m_window->isKeyPressed(GLFW_KEY_RIGHT)) {
        m_camera.orbit(-orbitSpeed, 0.0f);
    }
    if (m_window->isKeyPressed(GLFW_KEY_UP)) {
        m_camera.orbit(0.0f, orbitSpeed);
    }
    if (m_window->isKeyPressed(GLFW_KEY_DOWN)) {
        m_camera.orbit(0.0f, -orbitSpeed);
    }
}

void Application::update(float deltaTime) {
    // Update camera animation
    m_cameraAnimation.update(deltaTime, m_camera);

    // Process completed subdivision tasks (GPU upload on main thread)
    m_subdivisionManager->processCompletedTasks();

    // Process completed LOD generation tasks
    m_lodManager->processCompletedTasks();

    // Process completed tessellation tasks for multipatch
    m_multipatchManager->processCompletedTasks();

    // Auto-enable solution visualization when Poisson solving completes
    if (m_multipatchManager->isSolutionReady()) {
        m_multipatchManager->clearSolutionReady();
        m_renderer->setSolutionVisualization(true);
    }

    // Update multipatch tessellation based on view
    m_multipatchManager->updateTessellation(m_camera, m_window->getAspectRatio(),
                                            m_window->getWidth(), m_window->getHeight());

    // Check for objects that need LOD regeneration (after subdivision)
    for (const auto& obj : m_scene.getObjects()) {
        if (obj->needsLODRegeneration()) {
            obj->clearLODRegenerationFlag();
            generateLODForObject(obj.get());
        }
    }

    // Update scene objects (checks for completed async GPU uploads)
    m_scene.update();
}

void Application::render() {
    // Update animation state for help overlay
    m_renderer->setAnimationState(m_cameraAnimation.isPlaying(), m_cameraAnimation.isLoaded());

    m_renderer->render(m_scene, m_camera, m_window->getAspectRatio());
}

void Application::onKeyPressed(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                // Cancel animation, background tasks, or exit if idle
                if (m_cameraAnimation.isPlaying()) {
                    m_cameraAnimation.stop();
                } else if (m_subdivisionManager->isBusy()) {
                    m_subdivisionManager->cancelAll();
                } else if (m_multipatchManager->isSolvingPoisson()) {
                    m_multipatchManager->getPoissonManager()->cancelAll();
                } else if (m_multipatchManager->isBusy()) {
                    m_multipatchManager->cancelAll();
                } else {
                    glfwSetWindowShouldClose(m_window->getHandle(), true);
                }
                break;
            case GLFW_KEY_W:
                m_renderer->toggleWireframe();
                break;
            case GLFW_KEY_F:
            case GLFW_KEY_SPACE:
                focusOnScene();
                break;
            case GLFW_KEY_S:
                subdivideSelected(true);  // Smooth subdivision
                break;
            case GLFW_KEY_D:
                subdivideSelected(false); // Simple subdivision
                break;
            case GLFW_KEY_C:
                m_renderer->toggleBackfaceCulling();
                break;
            case GLFW_KEY_H:
                m_renderer->toggleHelpOverlay();
                break;
            case GLFW_KEY_G:
                m_renderer->toggleFrustumCulling();
                break;
            case GLFW_KEY_L:
                m_renderer->toggleLOD();
                break;
            case GLFW_KEY_K:
                m_renderer->toggleLODDebugColors();
                break;
            case GLFW_KEY_T:
                m_renderer->toggleTextures();
                break;
            case GLFW_KEY_P:
                // Poisson solving / solution visualization toggle
                if (m_multipatchManager->hasSolution()) {
                    // Toggle visualization if already solved
                    m_renderer->toggleSolutionVisualization();
                } else if (m_multipatchManager->canSolvePoisson() &&
                           !m_multipatchManager->isSolvingPoisson()) {
                    // Start solving if BVP file and not already solving
                    m_multipatchManager->startPoissonSolving();
                }
                break;
            case GLFW_KEY_A:
                // Toggle camera animation
                m_cameraAnimation.toggle();
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

        // Perform picking on right-click press
        if (action == GLFW_PRESS) {
            double mouseX, mouseY;
            m_window->getCursorPos(mouseX, mouseY);

            int pickedIndex = m_renderer->pick(
                m_scene, m_camera, m_window->getAspectRatio(),
                static_cast<int>(mouseX), static_cast<int>(mouseY)
            );

            // Clear all selections first
            for (const auto& obj : m_scene.getObjects()) {
                obj->setSelected(false);
            }

            // Select the picked object
            if (pickedIndex >= 0 && pickedIndex < static_cast<int>(m_scene.getObjectCount())) {
                SceneObject* obj = m_scene.getObject(pickedIndex);
                if (obj) {
                    obj->setSelected(true);
                }
            }
        }
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
    m_renderer->resize(width, height);
}

bool Application::loadMesh(const std::string& path) {
    // Check if this is a multipatch file (G+Smo XML)
    size_t dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".xml") {
            // Load as multipatch with view-dependent tessellation
            return m_multipatchManager->load(path, m_scene, 8);  // Start with coarse mesh
        }
    }

    // Standard mesh loading
    auto loader = MeshLoader::createForFile(path);
    if (!loader) {
        std::cerr << "No loader available for: " << path << std::endl;
        return false;
    }

    MeshData meshData;
    if (!loader->load(path, meshData)) {
        return false;
    }

    // Extract filename from path for naming
    std::string name = path;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        name = path.substr(lastSlash + 1);
    }

    SceneObject* obj = m_scene.addObject(name);
    obj->setMeshData(meshData);

    // Generate LOD levels automatically for the loaded mesh
    generateLODForObject(obj);

    // Assign different colors to each object
    static const glm::vec3 colors[] = {
        {0.8f, 0.3f, 0.3f},  // Red
        {0.3f, 0.8f, 0.3f},  // Green
        {0.3f, 0.3f, 0.8f},  // Blue
        {0.8f, 0.8f, 0.3f},  // Yellow
        {0.8f, 0.3f, 0.8f},  // Magenta
        {0.3f, 0.8f, 0.8f},  // Cyan
        {0.8f, 0.6f, 0.3f},  // Orange
        {0.6f, 0.3f, 0.8f},  // Purple
    };
    size_t colorIndex = (m_scene.getObjectCount() - 1) % 8;
    obj->setColor(colors[colorIndex]);

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

void Application::subdivideSelected(bool smooth) {
    bool anyQueued = false;

    // Subdivide selected objects first
    for (const auto& obj : m_scene.getObjects()) {
        if (obj->isSelected() && obj->canSubdivide()) {
            auto task = std::make_unique<SubdivisionTask>(
                obj.get(), obj->getName(), obj->getMeshData(), smooth, m_creaseAngle);
            m_subdivisionManager->submitTask(std::move(task));
            anyQueued = true;
        }
    }

    // If nothing was selected, subdivide only visible objects (in frustum)
    if (!anyQueued) {
        for (const auto& obj : m_scene.getObjects()) {
            if (obj->canSubdivide() && m_renderer->isVisible(obj->getWorldBounds())) {
                auto task = std::make_unique<SubdivisionTask>(
                    obj.get(), obj->getName(), obj->getMeshData(), smooth, m_creaseAngle);
                m_subdivisionManager->submitTask(std::move(task));
            }
        }
    }
}

void Application::generateLODForObject(SceneObject* obj) {
    if (!obj || !obj->canSubdivide()) {
        return;
    }

    // Only generate LOD if mesh has enough triangles to benefit
    const MeshData& meshData = obj->getMeshData();
    uint32_t triangleCount = static_cast<uint32_t>(meshData.indices.size() / 3);
    if (triangleCount < 100) {
        return;
    }

    auto task = std::make_unique<LODTask>(obj, obj->getName(), meshData);
    m_lodManager->submitTask(std::move(task));
}

bool Application::loadAnimation(const std::string& path) {
    return m_cameraAnimation.loadFromFile(path);
}
