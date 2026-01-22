#pragma once

#include "core/Window.h"
#include "core/Timer.h"
#include "renderer/Renderer.h"
#include "renderer/Camera.h"
#include "scene/Scene.h"
#include "mesh/Mesh.h"
#include "geometry/SubdivisionManager.h"
#include "lod/LODManager.h"
#include <memory>
#include <string>
#include <vector>

class Application {
public:
    Application(int width, int height, const std::string& title, float creaseAngle = 30.0f);
    ~Application() = default;

    int run(const std::vector<std::string>& meshPaths = {});

private:
    void setupCallbacks();
    void processInput();
    void update(float deltaTime);
    void render();

    void onKeyPressed(int key, int scancode, int action, int mods);
    void onMouseButton(int button, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    void onResize(int width, int height);

    bool loadMesh(const std::string& path);
    void focusOnScene();
    void subdivideSelected(bool smooth);
    void generateLODForObject(SceneObject* obj);

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<SubdivisionManager> m_subdivisionManager;
    std::unique_ptr<LODManager> m_lodManager;
    Camera m_camera;
    Scene m_scene;
    Timer m_timer;

    bool m_leftMouseDown{false};
    bool m_middleMouseDown{false};
    bool m_rightMouseDown{false};
    double m_lastMouseX{0.0};
    double m_lastMouseY{0.0};

    float m_creaseAngle{30.0f};
};
