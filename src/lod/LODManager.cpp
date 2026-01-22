#include "LODManager.h"
#include "MeshSimplifier.h"
#include "LODSelector.h"
#include "scene/SceneObject.h"
#include <iostream>

LODManager::LODManager() {
    // Start worker thread
    m_workerThread = std::thread(&LODManager::workerLoop, this);
}

LODManager::~LODManager() {
    // Signal shutdown
    m_shutdown.store(true, std::memory_order_release);

    // Cancel any active task
    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        if (m_activeTask) {
            m_activeTask->progress.cancel();
            m_activeTask->simplificationProgress.cancel();
        }
    }

    // Wake up worker thread
    m_queueCondition.notify_all();

    // Wait for worker to finish
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void LODManager::submitTask(std::unique_ptr<LODTask> task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingTasks.push(std::move(task));
    }
    m_queueCondition.notify_one();
}

int LODManager::processCompletedTasks() {
    std::vector<std::unique_ptr<LODTask>> tasksToProcess;

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
        if (task->targetObject && !task->resultLevels.empty()) {
            task->targetObject->applyLODLevels(std::move(task->resultLevels));
            ++count;
        }
    }

    return count;
}

void LODManager::cancelAll() {
    // Cancel active task
    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        if (m_activeTask) {
            m_activeTask->progress.cancel();
            m_activeTask->simplificationProgress.cancel();
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

bool LODManager::isBusy() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask != nullptr;
}

const LODProgress* LODManager::getActiveProgress() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask ? &m_activeTask->progress : nullptr;
}

std::string LODManager::getActiveObjectName() const {
    std::lock_guard<std::mutex> lock(m_activeMutex);
    return m_activeTask ? m_activeTask->objectName : "";
}

size_t LODManager::getQueuedTaskCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_pendingTasks.size();
}

void LODManager::workerLoop() {
    while (!m_shutdown.load(std::memory_order_acquire)) {
        std::unique_ptr<LODTask> task;

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

void LODManager::processTask(LODTask& task) {
    try {
        uint32_t originalTriangles = static_cast<uint32_t>(task.inputData.indices.size() / 3);

        // LOD 0 is the original mesh
        LODLevel lod0(MeshData(task.inputData), LODSelector::LOD0_THRESHOLD);
        task.resultLevels.push_back(std::move(lod0));

        if (task.progress.isCancelled()) return;

        // Generate LOD 1 (50% triangles)
        task.progress.setPhase(1);
        task.simplificationProgress.reset();
        uint32_t target1 = static_cast<uint32_t>(originalTriangles * LODSelector::LOD1_RATIO);
        if (target1 >= 4) {
            MeshData lod1Data = MeshSimplifier::simplifyWithProgress(
                task.inputData, target1, task.simplificationProgress);

            if (task.progress.isCancelled()) return;

            LODLevel lod1(std::move(lod1Data), LODSelector::LOD1_THRESHOLD);
            task.resultLevels.push_back(std::move(lod1));
        }

        // Generate LOD 2 (25% triangles)
        task.progress.setPhase(2);
        task.simplificationProgress.reset();
        uint32_t target2 = static_cast<uint32_t>(originalTriangles * LODSelector::LOD2_RATIO);
        if (target2 >= 4) {
            MeshData lod2Data = MeshSimplifier::simplifyWithProgress(
                task.inputData, target2, task.simplificationProgress);

            if (task.progress.isCancelled()) return;

            LODLevel lod2(std::move(lod2Data), LODSelector::LOD2_THRESHOLD);
            task.resultLevels.push_back(std::move(lod2));
        }

        // Generate LOD 3 (20% triangles)
        task.progress.setPhase(3);
        task.simplificationProgress.reset();
        uint32_t target3 = static_cast<uint32_t>(originalTriangles * LODSelector::LOD3_RATIO);
        if (target3 >= 4) {
            MeshData lod3Data = MeshSimplifier::simplifyWithProgress(
                task.inputData, target3, task.simplificationProgress);

            if (task.progress.isCancelled()) return;

            LODLevel lod3(std::move(lod3Data), LODSelector::LOD3_THRESHOLD);
            task.resultLevels.push_back(std::move(lod3));
        }

        // Generate LOD 4 (10% triangles)
        task.progress.setPhase(4);
        task.simplificationProgress.reset();
        uint32_t target4 = static_cast<uint32_t>(originalTriangles * LODSelector::LOD4_RATIO);
        if (target4 >= 4) {
            MeshData lod4Data = MeshSimplifier::simplifyWithProgress(
                task.inputData, target4, task.simplificationProgress);

            if (task.progress.isCancelled()) return;

            LODLevel lod4(std::move(lod4Data), LODSelector::LOD4_THRESHOLD);
            task.resultLevels.push_back(std::move(lod4));
        }

        // Generate LOD 5 (5% triangles)
        task.progress.setPhase(5);
        task.simplificationProgress.reset();
        uint32_t target5 = static_cast<uint32_t>(originalTriangles * LODSelector::LOD5_RATIO);
        if (target5 >= 4) {
            MeshData lod5Data = MeshSimplifier::simplifyWithProgress(
                task.inputData, target5, task.simplificationProgress);

            if (task.progress.isCancelled()) return;

            LODLevel lod5(std::move(lod5Data), LODSelector::LOD5_THRESHOLD);
            task.resultLevels.push_back(std::move(lod5));
        }

        task.progress.setPhase(6);
        task.progress.totalProgress.store(1.0f, std::memory_order_relaxed);

        if (!task.progress.isCancelled()) {
            task.progress.completed.store(true, std::memory_order_release);
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << task.objectName << "] LOD generation error: " << e.what() << std::endl;
        task.progress.hasError.store(true, std::memory_order_release);
    }
}
