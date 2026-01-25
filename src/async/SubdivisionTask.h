#pragma once

#include "Progress.h"
#include "mesh/MeshData.h"
#include <string>

class SceneObject;

// Phase names for progress display
inline const char* SUBDIVISION_PHASE_NAMES[] = {
    "Starting...",
    "Computing face normals",
    "Building adjacency",
    "Merging data",
    "Building edge list",
    "Detecting sharp edges",
    "Repositioning vertices",
    "Creating edge vertices",
    "Generating triangles"
};

constexpr int SUBDIVISION_PHASE_COUNT = 8;

// Type alias for backward compatibility
using SubdivisionProgress = Progress;

// Subdivision task containing all data needed for background processing
struct SubdivisionTask {
    // Input mesh data (copied for thread safety)
    MeshData inputData;

    // Result mesh data (populated by worker thread)
    MeshData resultData;

    // Progress tracking
    Progress progress;

    // Target scene object to apply result to
    SceneObject* targetObject{nullptr};

    // Object name for display
    std::string objectName;

    // Subdivision parameters
    bool smooth{true};          // true = Loop, false = midpoint
    float creaseAngle{180.0f};   // Only used for Loop subdivision

    SubdivisionTask() {
        progress.totalPhases = SUBDIVISION_PHASE_COUNT;
        progress.phaseNames = SUBDIVISION_PHASE_NAMES;
    }

    SubdivisionTask(SceneObject* target, const std::string& name, const MeshData& data,
                   bool smoothSubdiv, float angle)
        : inputData(data)
        , targetObject(target)
        , objectName(name)
        , smooth(smoothSubdiv)
        , creaseAngle(angle)
    {
        progress.totalPhases = SUBDIVISION_PHASE_COUNT;
        progress.phaseNames = SUBDIVISION_PHASE_NAMES;
        progress.reset();
    }

    // Required by TaskManager template
    Progress& getProgress() { return progress; }
    const Progress& getProgress() const { return progress; }
};
