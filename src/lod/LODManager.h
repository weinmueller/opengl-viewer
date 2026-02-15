#pragma once

#include "async/TaskManager.h"
#include "async/LODTask.h"

class LODManager : public TaskManager<LODTask> {
public:
    LODManager() = default;
    ~LODManager() override { shutdown(); }

protected:
    // Process a LOD generation task (runs on worker thread)
    void processTask(LODTask& task) override;

    // Apply completed task result to scene object (runs on main thread)
    bool applyTaskResult(LODTask& task) override;

    // Cancel active task (also cancels nested simplification progress)
    void cancelActiveTask() override;
};
