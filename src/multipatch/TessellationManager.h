#pragma once

#include "async/TaskManager.h"
#include "async/TessellationTask.h"

class TessellationManager : public TaskManager<TessellationTask> {
protected:
    void processTask(TessellationTask& task) override;
    bool applyTaskResult(TessellationTask& task) override;
};
