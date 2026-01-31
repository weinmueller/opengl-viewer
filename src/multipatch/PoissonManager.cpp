#include "PoissonManager.h"
#include <iostream>
#include <cmath>

#ifdef GISMO_AVAILABLE
#include <gismo.h>
#endif

#ifdef GISMO_AVAILABLE
void PoissonManager::startSolving(const std::string& filePath, const std::string& name,
                                   gismo::gsMultiPatch<>* multipatch) {
    auto task = std::make_unique<PoissonTask>(filePath, name);
    task->multipatch = multipatch;
    submitTask(std::move(task));
}
#endif

void PoissonManager::processTask(PoissonTask& task) {
#ifdef GISMO_AVAILABLE
    Progress& progress = task.getProgress();

    if (progress.isCancelled()) return;

    // Phase 1: Loading BVP data
    progress.setPhase(1);
    std::cout << "PoissonManager: Loading BVP data from " << task.filePath << std::endl;

    gismo::gsFileData<> fileData(task.filePath);

    // Check for required BVP components
    if (!fileData.has<gismo::gsFunctionExpr<>>()) {
        std::cerr << "PoissonManager: No source function found in file" << std::endl;
        progress.setError();
        return;
    }

    // Try to load source function
    gismo::gsFunctionExpr<> f;
    fileData.getFirst(f);

    // Try to load boundary conditions
    gismo::gsBoundaryConditions<> bc;
    if (fileData.has<gismo::gsBoundaryConditions<>>()) {
        fileData.getFirst(bc);
    } else {
        // Set up default homogeneous Dirichlet BCs on all boundaries
        for (size_t p = 0; p < task.multipatch->nPatches(); ++p) {
            for (int s = 1; s <= 4; ++s) {  // All 4 sides of each patch
                gismo::patchSide ps(p, s);
                bc.addCondition(ps, gismo::condition_type::dirichlet, nullptr);
            }
        }
    }

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 2: Setting up basis
    progress.setPhase(2);
    std::cout << "PoissonManager: Setting up basis functions" << std::endl;

    // Create basis from the multipatch geometry
    gismo::gsMultiBasis<> basis(*task.multipatch);

    // Refine the basis for better accuracy
    basis.uniformRefine();

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 3: Assembling system
    progress.setPhase(3);
    std::cout << "PoissonManager: Assembling linear system" << std::endl;

    // Create Poisson assembler
    gismo::gsPoissonAssembler<> assembler(*task.multipatch, basis, bc, f);
    assembler.options().setInt("DirichletStrategy", gismo::dirichlet::elimination);

    assembler.assemble();

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 4: Solving linear system
    progress.setPhase(4);
    std::cout << "PoissonManager: Solving linear system ("
              << assembler.matrix().rows() << " DOFs)" << std::endl;

    // Solve using CG solver
    gismo::gsSparseSolver<>::CGDiagonal solver;
    solver.compute(assembler.matrix());
    gismo::gsMatrix<> solVector = solver.solve(assembler.rhs());

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 5: Computing solution range
    progress.setPhase(5);
    std::cout << "PoissonManager: Constructing solution field" << std::endl;

    // Construct the solution as a gsMultiPatch using the assembler
    task.result.solutionField = std::make_unique<gismo::gsMultiPatch<>>();
    assembler.constructSolution(solVector, *task.result.solutionField);

    std::cout << "PoissonManager: Solution field has " << task.result.solutionField->nPatches()
              << " patches" << std::endl;

    // Find min/max values by sampling the solution
    task.result.minValue = std::numeric_limits<float>::max();
    task.result.maxValue = std::numeric_limits<float>::lowest();

    // Sample each patch to find range
    const int sampleCount = 20;
    for (size_t p = 0; p < task.result.solutionField->nPatches(); ++p) {
        const gismo::gsGeometry<>& solPatch = task.result.solutionField->patch(p);
        gismo::gsMatrix<> support = solPatch.support();

        for (int i = 0; i <= sampleCount; ++i) {
            for (int j = 0; j <= sampleCount; ++j) {
                double u = support(0, 0) + (support(0, 1) - support(0, 0)) * i / sampleCount;
                double v = support(1, 0) + (support(1, 1) - support(1, 0)) * j / sampleCount;

                gismo::gsMatrix<> uv(2, 1);
                uv << u, v;
                gismo::gsMatrix<> val = solPatch.eval(uv);

                float fval = static_cast<float>(val(0, 0));
                task.result.minValue = std::min(task.result.minValue, fval);
                task.result.maxValue = std::max(task.result.maxValue, fval);
            }
        }

        if (progress.isCancelled()) return;
    }

    // Handle edge case of constant solution
    if (std::abs(task.result.maxValue - task.result.minValue) < 1e-10f) {
        task.result.minValue -= 0.5f;
        task.result.maxValue += 0.5f;
    }

    std::cout << "PoissonManager: Solution range [" << task.result.minValue
              << ", " << task.result.maxValue << "]" << std::endl;

    task.result.valid = true;
    progress.complete();
#else
    (void)task;
    std::cerr << "PoissonManager: G+Smo support not available" << std::endl;
    task.getProgress().setError();
#endif
}

bool PoissonManager::applyTaskResult(PoissonTask& task) {
    if (!task.result.valid) {
        return false;
    }

    // Move the solution to our storage
    m_solution = std::move(task.result);

    std::cout << "PoissonManager: Solution applied successfully" << std::endl;
    return true;
}
