#pragma once

#include <atomic>
#include <cstddef>

// Unified progress tracking with atomics for lock-free updates
// Used by background tasks (subdivision, LOD generation, etc.)
struct Progress {
    // Current phase number (0 = not started)
    std::atomic<int> phase{0};

    // Progress within current phase (0.0 - 1.0)
    std::atomic<float> phaseProgress{0.0f};

    // Total progress across all phases (0.0 - 1.0)
    std::atomic<float> totalProgress{0.0f};

    // Completion flag
    std::atomic<bool> completed{false};

    // Cancellation request flag
    std::atomic<bool> cancelled{false};

    // Error flag
    std::atomic<bool> hasError{false};

    // Total number of phases (set by task)
    int totalPhases{1};

    // Phase names array (set by derived task types)
    const char* const* phaseNames{nullptr};

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
        // Update total progress based on phase (clamp to avoid negative values for phase 0)
        float baseProgress = (p <= 0) ? 0.0f : (p - 1) / static_cast<float>(totalPhases);
        totalProgress.store(baseProgress, std::memory_order_relaxed);
    }

    void updatePhaseProgress(float progress) {
        phaseProgress.store(progress, std::memory_order_relaxed);
        // Update total progress
        int currentPhase = phase.load(std::memory_order_relaxed);
        float baseProgress = (currentPhase - 1) / static_cast<float>(totalPhases);
        float phaseContribution = progress / static_cast<float>(totalPhases);
        totalProgress.store(baseProgress + phaseContribution, std::memory_order_relaxed);
    }

    const char* getPhaseName() const {
        if (!phaseNames) return "Processing...";
        int p = phase.load(std::memory_order_relaxed);
        if (p >= 0 && p <= totalPhases) {
            return phaseNames[p];
        }
        return "Unknown";
    }

    bool isCancelled() const {
        return cancelled.load(std::memory_order_relaxed);
    }

    void cancel() {
        cancelled.store(true, std::memory_order_relaxed);
    }

    void complete() {
        totalProgress.store(1.0f, std::memory_order_relaxed);
        completed.store(true, std::memory_order_release);
    }

    void setError() {
        hasError.store(true, std::memory_order_release);
    }
};
