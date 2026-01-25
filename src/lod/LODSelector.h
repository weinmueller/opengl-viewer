#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Utility class for calculating screen-space size of objects for LOD selection
class LODSelector {
public:
    // Calculate screen-space diameter in pixels of a bounding sphere
    // Parameters:
    //   worldCenter: Center of the bounding sphere in world space
    //   worldRadius: Radius of the bounding sphere in world space
    //   viewMatrix: Camera view matrix
    //   projMatrix: Camera projection matrix
    //   screenHeight: Height of the viewport in pixels
    static float calculateScreenSize(
        const glm::vec3& worldCenter,
        float worldRadius,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        int screenHeight)
    {
        // Transform center to view space
        glm::vec4 viewCenter = viewMatrix * glm::vec4(worldCenter, 1.0f);
        float distance = -viewCenter.z;  // Distance from camera (negative z in view space)

        if (distance <= 0.0f) {
            // Object is behind camera, return large value to use highest LOD
            return 10000.0f;
        }

        // Calculate projected size using projection matrix
        // The projection matrix's [1][1] element is cot(fov/2)
        float projScale = projMatrix[1][1];

        // Calculate screen-space diameter
        // diameter = 2 * radius * projScale / distance * (screenHeight / 2)
        float screenDiameter = (worldRadius * projScale * screenHeight) / distance;

        return screenDiameter;
    }

    // Default LOD thresholds (in screen pixels) - higher values = keep detail longer
    static constexpr float LOD0_THRESHOLD = 400.0f;  // Highest detail
    static constexpr float LOD1_THRESHOLD = 200.0f;
    static constexpr float LOD2_THRESHOLD = 100.0f;
    static constexpr float LOD3_THRESHOLD = 50.0f;
    static constexpr float LOD4_THRESHOLD = 25.0f;
    static constexpr float LOD5_THRESHOLD = 0.0f;    // Lowest detail

    // Default triangle ratios for each LOD level - gentler reduction
    static constexpr float LOD0_RATIO = 1.0f;        // 100%
    static constexpr float LOD1_RATIO = 0.7f;        // 70%
    static constexpr float LOD2_RATIO = 0.5f;        // 50%
    static constexpr float LOD3_RATIO = 0.35f;       // 35%
    static constexpr float LOD4_RATIO = 0.25f;       // 25%
    static constexpr float LOD5_RATIO = 0.15f;       // 15%

    // Hysteresis buffer to prevent LOD popping (10% of threshold)
    static constexpr float HYSTERESIS = 0.1f;

    // Select LOD index based on screen size with hysteresis
    // Returns LOD index (0 = highest detail, 5 = lowest)
    static int selectLOD(float screenSize, int currentLOD, int lodCount) {
        if (lodCount <= 1) return 0;

        // Thresholds for switching up (to higher detail)
        const float thresholdsUp[] = {
            LOD0_THRESHOLD * (1.0f + HYSTERESIS),
            LOD1_THRESHOLD * (1.0f + HYSTERESIS),
            LOD2_THRESHOLD * (1.0f + HYSTERESIS),
            LOD3_THRESHOLD * (1.0f + HYSTERESIS),
            LOD4_THRESHOLD * (1.0f + HYSTERESIS),
            0.0f
        };

        // Thresholds for switching down (to lower detail)
        const float thresholdsDown[] = {
            LOD0_THRESHOLD * (1.0f - HYSTERESIS),
            LOD1_THRESHOLD * (1.0f - HYSTERESIS),
            LOD2_THRESHOLD * (1.0f - HYSTERESIS),
            LOD3_THRESHOLD * (1.0f - HYSTERESIS),
            LOD4_THRESHOLD * (1.0f - HYSTERESIS),
            0.0f
        };

        int maxLOD = lodCount - 1;
        int newLOD = currentLOD;

        // Check if we should switch to higher detail (lower LOD index)
        while (newLOD > 0 && screenSize >= thresholdsUp[newLOD - 1]) {
            newLOD--;
        }

        // Check if we should switch to lower detail (higher LOD index)
        while (newLOD < maxLOD && screenSize < thresholdsDown[newLOD]) {
            newLOD++;
        }

        return newLOD;
    }
};
