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
        // Signal shutdown
        m_shutdown.store(true, std::memory_order_release);

        // Cancel any active task
        {
            std::lock_guard<std::mutex> lock(m_activeMutex);
            if (m_activeTask) {
                cancelActiveTask();
            }
        }

        // Wake up worker thread
        m_queueCondition.notify_all();

        // Wait for worker to finish
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
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

    // Get progress of active task (for UI display)
    // Returns nullptr if no active task
    const Progress* getActiveProgress() const {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        return m_activeTask ? &m_activeTask->getProgress() : nullptr;
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
    // Process a single task (implemented by derived class)
    virtual void processTask(TaskType& task) = 0;

    // Apply task result to scene object (implemented by derived class)
    // Returns true if successfully applied
    virtual bool applyTaskResult(TaskType& task) = 0;

    // Cancel the active task (can be overridden for additional cleanup)
    virtual void cancelActiveTask() {
        if (m_activeTask) {
            m_activeTask->getProgress().cancel();
        }
    }

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
