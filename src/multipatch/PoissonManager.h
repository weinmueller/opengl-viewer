#pragma once

#include "async/TaskManager.h"
#include "async/PoissonTask.h"
#include <memory>
#include <string>

#ifdef GISMO_AVAILABLE
namespace gismo {
    template<class T> class gsMultiPatch;
}
#endif

// Background task manager for Poisson equation solving
class PoissonManager : public TaskManager<PoissonTask> {
public:
    PoissonManager() = default;
    ~PoissonManager() override { shutdown(); }

#ifdef GISMO_AVAILABLE
    // Start solving Poisson equation
    void startSolving(const std::string& filePath, const std::string& name,
                      gismo::gsMultiPatch<>* multipatch);
#endif

    // Check if solution is available
    bool hasSolution() const { return m_solution.valid; }

    // Get the solution (valid after solving completes)
    const PoissonSolution& getSolution() const { return m_solution; }
    PoissonSolution& getSolution() { return m_solution; }

    // Get solution range
    float getSolutionMin() const { return m_solution.minValue; }
    float getSolutionMax() const { return m_solution.maxValue; }

protected:
    void processTask(PoissonTask& task) override;
    bool applyTaskResult(PoissonTask& task) override;

private:
    PoissonSolution m_solution;
};
