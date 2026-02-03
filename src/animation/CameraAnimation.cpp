#include "CameraAnimation.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

bool CameraAnimation::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open animation file: " << path << std::endl;
        return false;
    }

    try {
        json j = json::parse(file);

        // Parse name
        m_name = j.value("name", "Unnamed Animation");

        // Parse keyframes
        m_keyframes.clear();
        if (j.contains("keyframes") && j["keyframes"].is_array()) {
            for (const auto& kf : j["keyframes"]) {
                CameraKeyframe keyframe;
                keyframe.time = kf.value("time", 0.0f);

                // Parse target (array of 3 floats)
                if (kf.contains("target") && kf["target"].is_array() && kf["target"].size() >= 3) {
                    keyframe.target.x = kf["target"][0].get<float>();
                    keyframe.target.y = kf["target"][1].get<float>();
                    keyframe.target.z = kf["target"][2].get<float>();
                }

                keyframe.distance = kf.value("distance", 5.0f);
                keyframe.yaw = kf.value("yaw", 0.0f);
                keyframe.pitch = kf.value("pitch", 30.0f);
                keyframe.fov = kf.value("fov", 45.0f);

                m_keyframes.push_back(keyframe);
            }
        }

        // Sort keyframes by time
        std::sort(m_keyframes.begin(), m_keyframes.end(),
                  [](const CameraKeyframe& a, const CameraKeyframe& b) {
                      return a.time < b.time;
                  });

        // Calculate duration
        if (!m_keyframes.empty()) {
            m_duration = m_keyframes.back().time;
        } else {
            m_duration = 0.0f;
        }

        std::cout << "Loaded animation: " << m_name << " (" << m_keyframes.size()
                  << " keyframes, " << m_duration << "s)" << std::endl;
        return !m_keyframes.empty();

    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in animation file: " << e.what() << std::endl;
        return false;
    }
}

void CameraAnimation::play() {
    if (!m_keyframes.empty()) {
        m_playing = true;
    }
}

void CameraAnimation::stop() {
    m_playing = false;
}

void CameraAnimation::toggle() {
    if (m_playing) {
        stop();
    } else {
        play();
    }
}

float CameraAnimation::cubicEaseInOut(float t) {
    // Cubic ease-in-out: smooth acceleration and deceleration
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

CameraKeyframe CameraAnimation::interpolate(float time) const {
    if (m_keyframes.empty()) {
        return CameraKeyframe{};
    }

    if (m_keyframes.size() == 1) {
        return m_keyframes[0];
    }

    // Clamp time to valid range
    time = glm::clamp(time, 0.0f, m_duration);

    // Find surrounding keyframes
    size_t nextIdx = 0;
    for (size_t i = 0; i < m_keyframes.size(); ++i) {
        if (m_keyframes[i].time >= time) {
            nextIdx = i;
            break;
        }
        nextIdx = i + 1;
    }

    // Handle edge cases
    if (nextIdx == 0) {
        return m_keyframes[0];
    }
    if (nextIdx >= m_keyframes.size()) {
        return m_keyframes.back();
    }

    const CameraKeyframe& prev = m_keyframes[nextIdx - 1];
    const CameraKeyframe& next = m_keyframes[nextIdx];

    // Calculate interpolation factor
    float segmentDuration = next.time - prev.time;
    float t = (segmentDuration > 0.0f) ? (time - prev.time) / segmentDuration : 0.0f;

    // Apply easing
    float easedT = cubicEaseInOut(t);

    return CameraKeyframe::lerp(prev, next, easedT);
}

void CameraAnimation::update(float deltaTime, Camera& camera) {
    if (!m_playing || m_keyframes.empty() || m_duration <= 0.0f) {
        return;
    }

    // Update time based on direction
    if (m_forward) {
        m_currentTime += deltaTime;
        if (m_currentTime >= m_duration) {
            m_currentTime = m_duration;
            m_forward = false;  // Reverse direction for pingpong
        }
    } else {
        m_currentTime -= deltaTime;
        if (m_currentTime <= 0.0f) {
            m_currentTime = 0.0f;
            m_forward = true;  // Reverse direction for pingpong
        }
    }

    // Interpolate and apply to camera
    CameraKeyframe kf = interpolate(m_currentTime);
    camera.setTarget(kf.target);
    camera.setDistance(kf.distance);
    camera.setYawPitch(kf.yaw, kf.pitch);
    camera.setFOV(kf.fov);
}
