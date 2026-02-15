#pragma once

#include "Progress.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <vector>
#include <atomic>
#include <string>

// Snapshot of progress values, safe to read after the lock is released
struct ProgressSnapshot {
    int phase{0};
    float phaseProgress{0.0f};
    float totalProgress{0.0f};
    bool completed{false};
    bool cancelled{false};
    bool hasError{false};
    int totalPhases{1};
    const char* phaseName{"Processing..."};

    static ProgressSnapshot fromProgress(const Progress& p) {
        ProgressSnapshot snap;
        snap.phase = p.phase.load(std::memory_order_relaxed);
        snap.phaseProgress = p.phaseProgress.load(std::memory_order_relaxed);
        snap.totalProgress = p.totalProgress.load(std::memory_order_relaxed);
        snap.completed = p.completed.load(std::memory_order_relaxed);
        snap.cancelled = p.cancelled.load(std::memory_order_relaxed);
        snap.hasError = p.hasError.load(std::memory_order_relaxed);
        snap.totalPhases = p.totalPhases;
        snap.phaseName = p.getPhaseName();
        return snap;
    }
};

// Template base class for background task managers
// TaskType must have:
//   - Progress& getProgress()
//   - std::string objectName
//   - SceneObject* targetObject
template<typename TaskType>
class TaskManager {
public:
    TaskManager() {
        m_workerThread = std::thread(&TaskManager::workerLoop, this);
    }

    virtual ~TaskManager() {
        shutdown();
    }

    // Non-copyable, non-movable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    // Submit a new task
    void submitTask(std::unique_ptr<TaskType> task) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_pendingTasks.push(std::move(task));
        }
        m_queueCondition.notify_one();
    }

    // Call each frame from main thread to process completed tasks
    // Returns the number of tasks that were completed and applied
    int processCompletedTasks() {
        std::vector<std::unique_ptr<TaskType>> tasksToProcess;

        // Grab completed tasks
        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            tasksToProcess.swap(m_completedTasks);
        }

        int count = 0;
        for (auto& task : tasksToProcess) {
            if (task->getProgress().isCancelled()) {
                continue;
            }

            if (task->getProgress().hasError.load(std::memory_order_relaxed)) {
                continue;
            }

            // Apply result to scene object (implemented by derived class)
            if (applyTaskResult(*task)) {
                ++count;
            }
        }

        return count;
    }

    // Cancel all pending and active tasks
    void cancelAll() {
        // Cancel active task
        {
            std::lock_guard<std::mutex> lock(m_activeMutex);
            if (m_activeTask) {
                cancelActiveTask();
            }
        }

        // Clear pending queue
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_pendingTasks.empty()) {
                m_pendingTasks.pop();
            }
        }
    }

    // Check if there's an active task
    bool isBusy() const {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        return m_activeTask != nullptr;
    }

    // Get a snapshot of the active task's progress (safe to use after call returns)
    // Returns false if no active task
    bool getActiveProgressSnapshot(ProgressSnapshot& snapshot) const {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        if (!m_activeTask) return false;
        snapshot = ProgressSnapshot::fromProgress(m_activeTask->getProgress());
        return true;
    }

    // Get active task's object name (for UI display)
    std::string getActiveObjectName() const {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        return m_activeTask ? m_activeTask->objectName : "";
    }

    // Get number of queued tasks (not including active)
    size_t getQueuedTaskCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_pendingTasks.size();
    }

protected:
    // Call from derived class destructors to ensure clean shutdown
    // before derived members are destroyed. Safe to call multiple times.
    void shutdown() {
        if (m_shutdown.load(std::memory_order_acquire)) return;

        m_shutdown.store(true, std::memory_order_release);

        // Cancel active task (non-virtual: just set the cancel flag)
        {
            std::lock_guard<std::mutex> lock(m_activeMutex);
            if (m_activeTask) {
                m_activeTask->getProgress().cancel();
            }
        }

        m_queueCondition.notify_all();

        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }

    // Process a single task (implemented by derived class)
    virtual void processTask(TaskType& task) = 0;

    // Apply task result to scene object (implemented by derived class)
    // Returns true if successfully applied
    virtual bool applyTaskResult(TaskType& task) = 0;

    // Cancel the active task (can be overridden for additional cleanup)
    // Called with m_activeMutex held — safe to access getActiveTask()
    virtual void cancelActiveTask() {
        if (m_activeTask) {
            m_activeTask->getProgress().cancel();
        }
    }

    // Access the active task (only valid while m_activeMutex is held,
    // i.e. from within cancelActiveTask() overrides)
    TaskType* getActiveTask() const { return m_activeTask; }

private:
    void workerLoop() {
        while (!m_shutdown.load(std::memory_order_acquire)) {
            std::unique_ptr<TaskType> task;

            // Wait for a task
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCondition.wait(lock, [this] {
                    return m_shutdown.load(std::memory_order_acquire) || !m_pendingTasks.empty();
                });

                if (m_shutdown.load(std::memory_order_acquire) && m_pendingTasks.empty()) {
                    break;
                }

                if (!m_pendingTasks.empty()) {
                    task = std::move(m_pendingTasks.front());
                    m_pendingTasks.pop();
                }
            }

            if (task) {
                // Initialize progress before setting as active
                task->getProgress().phase.store(0, std::memory_order_relaxed);
                task->getProgress().totalProgress.store(0.0f, std::memory_order_relaxed);
                task->getProgress().phaseProgress.store(0.0f, std::memory_order_relaxed);

                // Set as active task
                {
                    std::lock_guard<std::mutex> lock(m_activeMutex);
                    m_activeTask = task.get();
                }

                // Process the task
                processTask(*task);

                // Clear active task
                {
                    std::lock_guard<std::mutex> lock(m_activeMutex);
                    m_activeTask = nullptr;
                }

                // Move to completed queue
                {
                    std::lock_guard<std::mutex> lock(m_completedMutex);
                    m_completedTasks.push_back(std::move(task));
                }
            }
        }
    }

    // Worker thread
    std::thread m_workerThread;

    // Task queue
    std::queue<std::unique_ptr<TaskType>> m_pendingTasks;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;

    // Completed tasks waiting for main thread to apply
    std::vector<std::unique_ptr<TaskType>> m_completedTasks;
    mutable std::mutex m_completedMutex;

    // Currently active task (owned by worker thread)
    TaskType* m_activeTask{nullptr};
    mutable std::mutex m_activeMutex;

    // Shutdown flag
    std::atomic<bool> m_shutdown{false};
};
