#include "SubdivisionManager.h"
#include "Subdivision.h"
#include "scene/SceneObject.h"
#include <iostream>

void SubdivisionManager::processTask(SubdivisionTask& task) {
    try {
        if (task.smooth) {
            task.resultData = Subdivision::loopSubdivideWithProgress(
                task.inputData, task.creaseAngle, task.progress);
        } else {
            task.resultData = Subdivision::midpointSubdivideWithProgress(
                task.inputData, task.progress);
        }

        if (!task.progress.isCancelled()) {
            task.progress.complete();
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << task.objectName << "] Subdivision error: " << e.what() << std::endl;
        task.progress.setError();
    }
}

bool SubdivisionManager::applyTaskResult(SubdivisionTask& task) {
    if (task.targetObject) {
        task.targetObject->applySubdividedMesh(std::move(task.resultData));
        return true;
    }
    return false;
}
