#pragma once

#include "async/TaskManager.h"
#include "async/SubdivisionTask.h"

class SubdivisionManager : public TaskManager<SubdivisionTask> {
public:
    SubdivisionManager() = default;
    ~SubdivisionManager() override = default;

protected:
    // Process a subdivision task (runs on worker thread)
    void processTask(SubdivisionTask& task) override;

    // Apply completed task result to scene object (runs on main thread)
    bool applyTaskResult(SubdivisionTask& task) override;
};
