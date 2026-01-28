#include "TessellationManager.h"
#include "PatchObject.h"
#include <iostream>

void TessellationManager::processTask(TessellationTask& task) {
    Progress& progress = task.getProgress();

    // Phase 0: Starting
    progress.setPhase(0);

    if (progress.isCancelled()) {
        return;
    }

    // Phase 1: Tessellating
    progress.setPhase(1);
    progress.updatePhaseProgress(0.0f);

    if (!task.tessellateFunc) {
        std::cerr << "TessellationManager: No tessellation function for " << task.objectName << std::endl;
        progress.setError();
        return;
    }

    // Perform tessellation
    task.resultData = task.tessellateFunc(task.newLevel, task.newLevel);

    if (progress.isCancelled()) {
        return;
    }

    progress.updatePhaseProgress(1.0f);
    progress.complete();
}

bool TessellationManager::applyTaskResult(TessellationTask& task) {
    if (!task.targetObject) {
        return false;
    }

    // Apply the tessellated mesh to the patch object
    task.targetObject->applyRetessellatedMesh(std::move(task.resultData), task.newLevel);
    return true;
}
