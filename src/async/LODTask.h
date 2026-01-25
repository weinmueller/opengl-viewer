#pragma once

#include "Progress.h"
#include "mesh/MeshData.h"
#include "lod/MeshSimplifier.h"
#include "lod/LODLevel.h"
#include <vector>
#include <string>

class SceneObject;

// Phase names for progress display
inline const char* LOD_PHASE_NAMES[] = {
    "Starting...",
    "Generating LOD 1 (50%)",
    "Generating LOD 2 (30%)",
    "Generating LOD 3 (15%)",
    "Generating LOD 4 (7%)",
    "Generating LOD 5 (3%)",
    "Finalizing..."
};

constexpr int LOD_PHASE_COUNT = 6;

// Type alias for backward compatibility
using LODProgress = Progress;

// LOD generation task
struct LODTask {
    // Input mesh data (copied for thread safety)
    MeshData inputData;

    // Result LOD levels
    std::vector<LODLevel> resultLevels;

    // Progress tracking
    Progress progress;

    // Target scene object to apply result to
    SceneObject* targetObject{nullptr};

    // Object name for display
    std::string objectName;

    // Simplification progress for current phase
    SimplificationProgress simplificationProgress;

    LODTask() {
        progress.totalPhases = LOD_PHASE_COUNT;
        progress.phaseNames = LOD_PHASE_NAMES;
    }

    LODTask(SceneObject* target, const std::string& name, const MeshData& data)
        : inputData(data)
        , targetObject(target)
        , objectName(name)
    {
        progress.totalPhases = LOD_PHASE_COUNT;
        progress.phaseNames = LOD_PHASE_NAMES;
        progress.reset();
    }

    // Required by TaskManager template
    Progress& getProgress() { return progress; }
    const Progress& getProgress() const { return progress; }
};
