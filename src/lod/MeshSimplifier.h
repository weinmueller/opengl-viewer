#pragma once

#include "mesh/MeshData.h"
#include <atomic>
#include <functional>

// Progress callback for mesh simplification
struct SimplificationProgress {
    std::atomic<float> progress{0.0f};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> completed{false};

    void reset() {
        progress.store(0.0f, std::memory_order_relaxed);
        cancelled.store(false, std::memory_order_relaxed);
        completed.store(false, std::memory_order_relaxed);
    }

    bool isCancelled() const {
        return cancelled.load(std::memory_order_relaxed);
    }

    void cancel() {
        cancelled.store(true, std::memory_order_relaxed);
    }
};

// QEM-based mesh simplification
class MeshSimplifier {
public:
    // Simplify mesh to target number of triangles
    // Returns simplified mesh data
    static MeshData simplify(const MeshData& input, uint32_t targetTriangles);

    // Simplify with progress tracking (for background threading)
    static MeshData simplifyWithProgress(
        const MeshData& input,
        uint32_t targetTriangles,
        SimplificationProgress& progress);

    // Simplify to a fraction of original triangles
    static MeshData simplifyRatio(const MeshData& input, float ratio);

    // Simplify with ratio and progress
    static MeshData simplifyRatioWithProgress(
        const MeshData& input,
        float ratio,
        SimplificationProgress& progress);

private:
    // Internal implementation with quadric error metrics
    struct Impl;
};
