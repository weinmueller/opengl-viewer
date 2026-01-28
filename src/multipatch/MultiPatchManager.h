#pragma once

#include "PatchObject.h"
#include "TessellationManager.h"
#include "scene/Scene.h"
#include "renderer/Camera.h"
#include <string>
#include <vector>
#include <memory>

#ifdef GISMO_AVAILABLE
#include <gismo.h>
#endif

// Tessellation level thresholds based on screen-space size (in pixels)
struct TessellationThresholds {
    int minLevel = 4;       // Minimum tessellation (very distant)
    int maxLevel = 128;     // Maximum tessellation (very close)
    float minScreenSize = 20.0f;   // Below this, use minLevel
    float maxScreenSize = 500.0f;  // Above this, use maxLevel
};

// Manages a G+Smo multipatch with view-dependent tessellation
class MultiPatchManager {
public:
    MultiPatchManager();
    ~MultiPatchManager() = default;

    // Load a multipatch from file and add patches to scene
    bool load(const std::string& path, Scene& scene, int initialTessLevel = 16);

    // Update tessellation levels based on camera view
    // Call this each frame or when camera changes
    void updateTessellation(const Camera& camera, float aspectRatio, int viewportWidth, int viewportHeight);

    // Process completed tessellation tasks (call from main thread)
    void processCompletedTasks();

    // Check if any patches are being re-tessellated
    bool isBusy() const { return m_tessManager.isBusy(); }

    // Get progress info for UI
    const Progress* getActiveProgress() const { return m_tessManager.getActiveProgress(); }
    std::string getActiveObjectName() const { return m_tessManager.getActiveObjectName(); }
    size_t getQueuedTaskCount() const { return m_tessManager.getQueuedTaskCount(); }

    // Get tessellation thresholds
    TessellationThresholds& getThresholds() { return m_thresholds; }
    const TessellationThresholds& getThresholds() const { return m_thresholds; }

    // Get patch objects
    const std::vector<PatchObject*>& getPatches() const { return m_patchObjects; }

    // Cancel all pending retessellation
    void cancelAll() { m_tessManager.cancelAll(); }

    // Enable/disable automatic tessellation refinement
    void setAutoRefinement(bool enabled) { m_autoRefinement = enabled; }
    bool isAutoRefinementEnabled() const { return m_autoRefinement; }

private:
    // Calculate appropriate tessellation level based on screen size
    int calculateTessLevel(float screenSize) const;

    // Calculate screen-space size of a bounding box
    float calculateScreenSize(const BoundingBox& bounds, const glm::mat4& viewProj,
                              int viewportWidth, int viewportHeight) const;

#ifdef GISMO_AVAILABLE
    // G+Smo multipatch (stored for re-tessellation)
    std::unique_ptr<gismo::gsMultiPatch<>> m_multipatch;
#endif

    // References to patch objects (owned by Scene)
    std::vector<PatchObject*> m_patchObjects;

    // Background tessellation manager
    TessellationManager m_tessManager;

    // Tessellation thresholds
    TessellationThresholds m_thresholds;

    // Auto refinement flag
    bool m_autoRefinement{true};

    // Hysteresis factor to prevent thrashing
    float m_hysteresisFactor{0.2f};
};
