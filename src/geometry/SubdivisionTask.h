#pragma once

#include "mesh/MeshData.h"
#include <atomic>
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

// Progress tracking with atomics for lock-free updates
struct SubdivisionProgress {
    // Current phase (1-8 for loop subdivision phases, 0 = not started)
    std::atomic<int> phase{0};

    // Progress within current phase (0.0 - 1.0)
    std::atomic<float> phaseProgress{0.0f};

    // Total progress (0.0 - 1.0) across all phases
    std::atomic<float> totalProgress{0.0f};

    // Completion flag
    std::atomic<bool> completed{false};

    // Cancellation request flag
    std::atomic<bool> cancelled{false};

    // Error flag
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
        // Update total progress based on phase (assuming 8 phases for loop subdivision)
        float baseProgress = (p - 1) / static_cast<float>(SUBDIVISION_PHASE_COUNT);
        totalProgress.store(baseProgress, std::memory_order_relaxed);
    }

    void updatePhaseProgress(float progress) {
        phaseProgress.store(progress, std::memory_order_relaxed);
        // Update total progress
        int currentPhase = phase.load(std::memory_order_relaxed);
        float baseProgress = (currentPhase - 1) / static_cast<float>(SUBDIVISION_PHASE_COUNT);
        float phaseContribution = progress / static_cast<float>(SUBDIVISION_PHASE_COUNT);
        totalProgress.store(baseProgress + phaseContribution, std::memory_order_relaxed);
    }

    const char* getPhaseName() const {
        int p = phase.load(std::memory_order_relaxed);
        if (p >= 0 && p <= SUBDIVISION_PHASE_COUNT) {
            return SUBDIVISION_PHASE_NAMES[p];
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

// Subdivision task containing all data needed for background processing
struct SubdivisionTask {
    // Input mesh data (copied for thread safety)
    MeshData inputData;

    // Result mesh data (populated by worker thread)
    MeshData resultData;

    // Progress tracking
    SubdivisionProgress progress;

    // Target scene object to apply result to
    SceneObject* targetObject{nullptr};

    // Object name for display
    std::string objectName;

    // Subdivision parameters
    bool smooth{true};          // true = Loop, false = midpoint
    float creaseAngle{180.0f};   // Only used for Loop subdivision

    SubdivisionTask() = default;

    SubdivisionTask(SceneObject* target, const std::string& name, const MeshData& data,
                   bool smoothSubdiv, float angle)
        : inputData(data)
        , targetObject(target)
        , objectName(name)
        , smooth(smoothSubdiv)
        , creaseAngle(angle)
    {
        progress.reset();
    }
};
