#include "SubdivisionManager.h"
#include "Subdivision.h"
#include "scene/SceneObject.h"
#include <iostream>

SubdivisionManager::SubdivisionManager() {
    // Start worker thread
    m_workerThread = std::thread(&SubdivisionManager::workerLoop, this);
}

SubdivisionManager::~SubdivisionManager() {
    // Signal shutdown
    m_shutdown.store(true, std::memory_order_release);

    // Cancel any active task
    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        if (m_activeTask) {
            m_activeTask->progress.cancel();
        }
    }

    // Wake up worker thread
    m_queueCondition.notify_all();

    // Wait for worker to finish
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void SubdivisionManager::submitTask(std::unique_ptr<SubdivisionTask> task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingTasks.push(std::move(task));
    }
    m_queueCondition.notify_one();
}

int SubdivisionManager::processCompletedTasks() {
    std::vector<std::unique_ptr<SubdivisionTask>> tasksToProcess;

    // Grab completed tasks
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        tasksToProcess.swap(m_completedTasks);
    }

    int count = 0;
    for (auto& task : tasksToProcess) {
        if (task->progress.isCancelled()) {
            continue;
        }

        if (task->progress.hasError.load(std::memory_order_relaxed)) {
            continue;
        }

        // Apply result to scene object (GPU upload happens on main thread)
        if (task->targetObject) {
            task->targetObject->applySubdividedMesh(std::move(task->resultData));
            ++count;
        }
    }

    return count;
}

void SubdivisionManager::cancelAll() {
    // Cancel active task
    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        if (m_activeTask) {
            m_activeTask->progress.cancel();
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

bool SubdivisionManager::isBusy() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask != nullptr;
}

const SubdivisionProgress* SubdivisionManager::getActiveProgress() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask ? &m_activeTask->progress : nullptr;
}

std::string SubdivisionManager::getActiveObjectName() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask ? m_activeTask->objectName : "";
}

size_t SubdivisionManager::getQueuedTaskCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_pendingTasks.size();
}

void SubdivisionManager::workerLoop() {
    while (!m_shutdown.load(std::memory_order_acquire)) {
        std::unique_ptr<SubdivisionTask> task;

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
            // Initialize progress to 0% before setting as active
            // This ensures the UI shows 0% when it first renders
            task->progress.phase.store(0, std::memory_order_relaxed);
            task->progress.totalProgress.store(0.0f, std::memory_order_relaxed);
            task->progress.phaseProgress.store(0.0f, std::memory_order_relaxed);

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
            task.progress.completed.store(true, std::memory_order_release);
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << task.objectName << "] Subdivision error: " << e.what() << std::endl;
        task.progress.hasError.store(true, std::memory_order_release);
    }
}
