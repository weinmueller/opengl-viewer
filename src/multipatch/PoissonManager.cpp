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

    gismo::gsFileData<> fileData(task.filePath);

    // Load source function (id=1 in standard BVP files)
    gismo::gsFunctionExpr<> f;
    if (fileData.hasId(1)) {
        fileData.getId(1, f);
    } else if (fileData.has<gismo::gsFunctionExpr<>>()) {
        fileData.getFirst(f);
    } else {
        std::cerr << "PoissonManager: No source function found in file" << std::endl;
        progress.setError();
        return;
    }
    // Load boundary conditions (id=2 in standard BVP files)
    gismo::gsBoundaryConditions<> bc;
    if (fileData.hasId(2)) {
        fileData.getId(2, bc);
    } else if (fileData.has<gismo::gsBoundaryConditions<>>()) {
        fileData.getFirst(bc);
    } else {
        // Set up default homogeneous Dirichlet BCs on all boundaries
        for (size_t p = 0; p < task.multipatch->nPatches(); ++p) {
            for (int s = 1; s <= 4; ++s) {
                gismo::patchSide ps(p, s);
                bc.addCondition(ps, gismo::condition_type::dirichlet, nullptr);
            }
        }
    }

    // Link boundary conditions to the geometry
    bc.setGeoMap(*task.multipatch);

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 2: Setting up basis
    progress.setPhase(2);

    // Create basis from the multipatch geometry (true = use poly-splines, not NURBS)
    gismo::gsMultiBasis<> basis(*task.multipatch, true);

    // Refine the basis multiple times for better accuracy
    const int numRefinements = 5;  // More refinements = more DOFs
    for (int i = 0; i < numRefinements; ++i) {
        basis.uniformRefine();
        if (progress.isCancelled()) return;
        progress.updatePhaseProgress(static_cast<float>(i + 1) / numRefinements);
    }

    // Phase 3: Assembling system
    progress.setPhase(3);

    // Create Poisson assembler
    gismo::gsPoissonAssembler<> assembler(*task.multipatch, basis, bc, f);

    // Load assembler options from file if available (id=4)
    if (fileData.hasId(4)) {
        gismo::gsOptionList opts;
        fileData.getId(4, opts);
        assembler.options().update(opts, gismo::gsOptionList::addIfUnknown);
    }
    assembler.options().setInt("DirichletStrategy", gismo::dirichlet::elimination);
    assembler.assemble();

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 4: Solving linear system
    progress.setPhase(4);

    // Solve using CG solver
    gismo::gsSparseSolver<>::CGDiagonal solver;
    solver.compute(assembler.matrix());
    gismo::gsMatrix<> solVector = solver.solve(assembler.rhs());

    progress.updatePhaseProgress(1.0f);
    if (progress.isCancelled()) return;

    // Phase 5: Computing solution range
    progress.setPhase(5);

    // Construct the solution as a gsMultiPatch using the assembler
    task.result.solutionField = std::make_unique<gismo::gsMultiPatch<>>();
    assembler.constructSolution(solVector, *task.result.solutionField);

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
    return true;
}
