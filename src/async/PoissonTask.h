#pragma once

#include "Progress.h"
#include <string>
#include <memory>
#include <vector>

#ifdef GISMO_AVAILABLE
#include <gismo.h>
#endif

// Phase names for Poisson solver progress display
inline const char* POISSON_PHASE_NAMES[] = {
    "Starting...",
    "Loading BVP data",
    "Setting up basis",
    "Assembling system",
    "Solving linear system",
    "Computing solution range"
};

constexpr int POISSON_PHASE_COUNT = 6;

// Poisson solution data structure
struct PoissonSolution {
    bool valid{false};
    float minValue{0.0f};
    float maxValue{1.0f};

#ifdef GISMO_AVAILABLE
    // Store the solution field for proper evaluation
    std::unique_ptr<gismo::gsMultiPatch<>> solutionField;

    // Evaluate solution at a parametric point on a specific patch
    float evaluateAt(const gismo::gsGeometry<>& /*geomPatch*/, double u, double v, int patchIndex) const {
        if (!valid || !solutionField || patchIndex >= static_cast<int>(solutionField->nPatches())) {
            return 0.0f;
        }

        // Create parameter point
        gismo::gsMatrix<> uv(2, 1);
        uv << u, v;

        // Evaluate the solution field at this parametric point
        gismo::gsMatrix<> result = solutionField->patch(patchIndex).eval(uv);

        return static_cast<float>(result(0, 0));
    }
#endif
};

// Poisson solving task
struct PoissonTask {
    // Input file path (for loading BVP data)
    std::string filePath;

    // Progress tracking
    Progress progress;

    // Result solution data
    PoissonSolution result;

    // Object name for display
    std::string objectName;

#ifdef GISMO_AVAILABLE
    // Reference to loaded multipatch geometry
    gismo::gsMultiPatch<>* multipatch{nullptr};
#endif

    PoissonTask() {
        progress.totalPhases = POISSON_PHASE_COUNT;
        progress.phaseNames = POISSON_PHASE_NAMES;
    }

    PoissonTask(const std::string& path, const std::string& name)
        : filePath(path)
        , objectName(name)
    {
        progress.totalPhases = POISSON_PHASE_COUNT;
        progress.phaseNames = POISSON_PHASE_NAMES;
        progress.reset();
    }

    // Required by TaskManager template
    Progress& getProgress() { return progress; }
    const Progress& getProgress() const { return progress; }
};
