#pragma once

#include "mesh/MeshData.h"
#include "MeshSimplifier.h"
#include "LODLevel.h"
#include <vector>
#include <string>
#include <atomic>

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

// Progress tracking for LOD generation
struct LODProgress {
    std::atomic<int> phase{0};
    std::atomic<float> phaseProgress{0.0f};
    std::atomic<float> totalProgress{0.0f};
    std::atomic<bool> completed{false};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> hasError{false};

    void reset() {
        phase.store(0, std::memory_order_relaxed);
        phaseProgress.store(0.0f, std::memory_order_relaxed);
        totalProgress.store(0.0f, std::memory_order_relaxed);
        completed.store(false, std::memory_order_relaxed);
        cancelled.store(false, std::memory_order_relaxed);
        hasError.store(false, std::memory_order_relaxed);
    }

    void setPhase(int p) {
        phase.store(p, std::memory_order_relaxed);
        phaseProgress.store(0.0f, std::memory_order_relaxed);
        float baseProgress = (p - 1) / static_cast<float>(LOD_PHASE_COUNT);
        totalProgress.store(baseProgress, std::memory_order_relaxed);
    }

    void updatePhaseProgress(float progress) {
        phaseProgress.store(progress, std::memory_order_relaxed);
        int currentPhase = phase.load(std::memory_order_relaxed);
        float baseProgress = (currentPhase - 1) / static_cast<float>(LOD_PHASE_COUNT);
        float phaseContribution = progress / static_cast<float>(LOD_PHASE_COUNT);
        totalProgress.store(baseProgress + phaseContribution, std::memory_order_relaxed);
    }

    const char* getPhaseName() const {
        int p = phase.load(std::memory_order_relaxed);
        if (p >= 0 && p <= LOD_PHASE_COUNT) {
            return LOD_PHASE_NAMES[p];
        }
        return "Unknown";
    }

    bool isCancelled() const {
        return cancelled.load(std::memory_order_relaxed);
    }

    void cancel() {
        cancelled.store(true, std::memory_order_relaxed);
    }
};

// LOD generation task
struct LODTask {
    // Input mesh data (copied for thread safety)
    MeshData inputData;

    // Result LOD levels
    std::vector<LODLevel> resultLevels;

    // Progress tracking
    LODProgress progress;

    // Target scene object to apply result to
    SceneObject* targetObject{nullptr};

    // Object name for display
    std::string objectName;

    // Simplification progress for current phase
    SimplificationProgress simplificationProgress;

    LODTask() = default;

    LODTask(SceneObject* target, const std::string& name, const MeshData& data)
        : inputData(data)
        , targetObject(target)
        , objectName(name)
    {
        progress.reset();
    }
};
