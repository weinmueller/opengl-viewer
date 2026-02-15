#include "LODManager.h"
#include "MeshSimplifier.h"
#include "LODSelector.h"
#include "scene/SceneObject.h"
#include <iostream>

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

        if (!task.progress.isCancelled()) {
            task.progress.complete();
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << task.objectName << "] LOD generation error: " << e.what() << std::endl;
        task.progress.setError();
    }
}

bool LODManager::applyTaskResult(LODTask& task) {
    if (task.targetObject && !task.resultLevels.empty()) {
        task.targetObject->applyLODLevels(std::move(task.resultLevels));
        return true;
    }
    return false;
}

void LODManager::cancelActiveTask() {
    // First call base implementation (cancels outer progress)
    TaskManager<LODTask>::cancelActiveTask();

    // Also cancel the nested simplification progress so the inner loop exits promptly
    if (auto* task = getActiveTask()) {
        task->simplificationProgress.cancel();
    }
}
