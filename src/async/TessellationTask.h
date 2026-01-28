#pragma once

#include "Progress.h"
#include "mesh/MeshData.h"
#include <string>
#include <functional>

class PatchObject;

// Callback type for tessellation function
using TessellationFunction = std::function<MeshData(int, int)>;

// Phase names for progress display
inline const char* TESSELLATION_PHASE_NAMES[] = {
    "Starting...",
    "Tessellating patch"
};

constexpr int TESSELLATION_PHASE_COUNT = 2;

// Tessellation task for re-tessellating a patch at a new level
struct TessellationTask {
    // Tessellation callback (captures G+Smo patch reference)
    TessellationFunction tessellateFunc;

    // Result mesh data (populated by worker thread)
    MeshData resultData;

    // Progress tracking
    Progress progress;

    // Target patch object to apply result to
    PatchObject* targetObject{nullptr};

    // Object name for display
    std::string objectName;

    // New tessellation level
    int newLevel{16};

    TessellationTask() {
        progress.totalPhases = TESSELLATION_PHASE_COUNT;
        progress.phaseNames = TESSELLATION_PHASE_NAMES;
    }

    TessellationTask(PatchObject* target, const std::string& name,
                     TessellationFunction func, int level)
        : tessellateFunc(std::move(func))
        , targetObject(target)
        , objectName(name)
        , newLevel(level)
    {
        progress.totalPhases = TESSELLATION_PHASE_COUNT;
        progress.phaseNames = TESSELLATION_PHASE_NAMES;
        progress.reset();
    }

    // Required by TaskManager template
    Progress& getProgress() { return progress; }
    const Progress& getProgress() const { return progress; }
};
