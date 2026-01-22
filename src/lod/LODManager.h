#pragma once

#include "LODTask.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <vector>

class LODManager {
public:
    LODManager();
    ~LODManager();

    // Non-copyable, non-movable
    LODManager(const LODManager&) = delete;
    LODManager& operator=(const LODManager&) = delete;

    // Submit a new LOD generation task
    void submitTask(std::unique_ptr<LODTask> task);

    // Call each frame from main thread to process completed tasks
    // Returns the number of tasks that were completed and applied
    int processCompletedTasks();

    // Cancel all pending and active tasks
    void cancelAll();

    // Check if there's an active task
    bool isBusy() const;

    // Get progress of active task (for UI display)
    // Returns nullptr if no active task
    const LODProgress* getActiveProgress() const;

    // Get active task's object name (for UI display)
    std::string getActiveObjectName() const;

    // Get number of queued tasks (not including active)
    size_t getQueuedTaskCount() const;

private:
    void workerLoop();
    void processTask(LODTask& task);

    // Worker thread
    std::thread m_workerThread;

    // Task queue
    std::queue<std::unique_ptr<LODTask>> m_pendingTasks;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;

    // Completed tasks waiting for main thread to apply
    std::vector<std::unique_ptr<LODTask>> m_completedTasks;
    mutable std::mutex m_completedMutex;

    // Currently active task (owned by worker thread)
    LODTask* m_activeTask{nullptr};
    mutable std::mutex m_activeMutex;

    // Shutdown flag
    std::atomic<bool> m_shutdown{false};
};
