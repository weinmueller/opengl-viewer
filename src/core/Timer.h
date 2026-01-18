#pragma once

#include <GLFW/glfw3.h>

class Timer {
public:
    Timer() : m_lastTime(0.0), m_deltaTime(0.0), m_frameCount(0), m_fps(0.0), m_fpsTimer(0.0) {}

    void update() {
        double currentTime = glfwGetTime();
        m_deltaTime = currentTime - m_lastTime;
        m_lastTime = currentTime;

        m_frameCount++;
        m_fpsTimer += m_deltaTime;
        if (m_fpsTimer >= 1.0) {
            m_fps = static_cast<double>(m_frameCount) / m_fpsTimer;
            m_frameCount = 0;
            m_fpsTimer = 0.0;
        }
    }

    double getDeltaTime() const { return m_deltaTime; }
    float getDeltaTimeF() const { return static_cast<float>(m_deltaTime); }
    double getTime() const { return m_lastTime; }
    double getFPS() const { return m_fps; }

private:
    double m_lastTime;
    double m_deltaTime;
    int m_frameCount;
    double m_fps;
    double m_fpsTimer;
};
