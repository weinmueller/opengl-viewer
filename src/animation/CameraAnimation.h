#pragma once

#include "CameraKeyframe.h"
#include "../renderer/Camera.h"
#include <string>
#include <vector>

class CameraAnimation {
public:
    CameraAnimation() = default;
    ~CameraAnimation() = default;

    bool loadFromFile(const std::string& path);

    void play();
    void stop();
    void toggle();

    void update(float deltaTime, Camera& camera);

    bool isPlaying() const { return m_playing; }
    bool isLoaded() const { return !m_keyframes.empty(); }
    const std::string& getName() const { return m_name; }

private:
    static float cubicEaseInOut(float t);
    CameraKeyframe interpolate(float time) const;

    std::vector<CameraKeyframe> m_keyframes;
    std::string m_name;

    float m_currentTime{0.0f};
    float m_duration{0.0f};
    bool m_playing{false};
    bool m_forward{true};  // For pingpong: true = forward, false = backward
};
