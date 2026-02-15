#include "MultiPatchManager.h"
#include "GismoLoader.h"
#include "async/TessellationTask.h"
#include "async/PoissonTask.h"
#include <iostream>
#include <algorithm>
#include <cmath>

MultiPatchManager::MultiPatchManager() = default;

bool MultiPatchManager::load(const std::string& path, Scene& scene, int initialTessLevel) {
#ifdef GISMO_AVAILABLE
    std::cout << "MultiPatchManager: Loading " << path << std::endl;

    // Store file path for Poisson solving
    m_loadedFilePath = path;

    // Load the file using G+Smo
    gismo::gsFileData<> fileData(path);

    // Check for BVP data (source function indicates Poisson problem)
    m_hasBVPData = fileData.has<gismo::gsFunctionExpr<>>();
    if (m_hasBVPData) {
        std::cout << "  BVP data detected - press P to solve Poisson equation" << std::endl;
    }

    // Try to get as multipatch
    if (fileData.has<gismo::gsMultiPatch<>>()) {
        m_multipatch = std::make_unique<gismo::gsMultiPatch<>>();
        fileData.getFirst(*m_multipatch);
        std::cout << "  Found MultiPatch with " << m_multipatch->nPatches() << " patches" << std::endl;
    }
    // Try single geometry - wrap into a single-patch multipatch
    else if (fileData.has<gismo::gsGeometry<>>()) {
        m_multipatch = std::make_unique<gismo::gsMultiPatch<>>();
        auto geom = fileData.getFirst<gismo::gsGeometry<>>();
        m_multipatch->addPatch(gismo::give(geom));
        std::cout << "  Found single geometry, wrapping as 1-patch multipatch" << std::endl;
    }
    else {
        std::cerr << "MultiPatchManager: No geometry found in " << path << std::endl;
        return false;
    }

    // Process the multipatch (shared code for both single and multi-patch)
    {
        // Extract filename for naming
        std::string baseName = path;
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            baseName = path.substr(lastSlash + 1);
        }

        // Create a PatchObject for each patch
        m_patchObjects.reserve(m_multipatch->nPatches());

        for (size_t i = 0; i < m_multipatch->nPatches(); ++i) {
            std::string patchName = baseName + "_patch" + std::to_string(i);

            // Create patch object
            auto patchObj = std::make_unique<PatchObject>(patchName, static_cast<int>(i));

            // Create tessellation callback that captures the patch reference
            // We capture by index since the multipatch is stored in this manager
            gismo::gsGeometry<>* patchPtr = &m_multipatch->patch(i);
            auto tessCallback = [patchPtr](int uSamples, int vSamples) -> MeshData {
                return GismoLoader::tessellatePatch(*patchPtr, uSamples, vSamples);
            };

            patchObj->setTessellationCallback(tessCallback);

            // Perform initial tessellation synchronously
            patchObj->tessellateSync(initialTessLevel);

            // Assign color based on patch index
            static const glm::vec3 colors[] = {
                {0.8f, 0.4f, 0.4f},  // Red
                {0.4f, 0.8f, 0.4f},  // Green
                {0.4f, 0.4f, 0.8f},  // Blue
                {0.8f, 0.8f, 0.4f},  // Yellow
                {0.8f, 0.4f, 0.8f},  // Magenta
                {0.4f, 0.8f, 0.8f},  // Cyan
                {0.8f, 0.6f, 0.4f},  // Orange
                {0.6f, 0.4f, 0.8f},  // Purple
            };
            patchObj->setColor(colors[i % 8]);

            // Store raw pointer before moving to scene
            PatchObject* rawPtr = patchObj.get();
            m_patchObjects.push_back(rawPtr);

            // Add to scene (scene takes ownership via SceneObject base class)
            scene.addObject(std::move(patchObj));

            std::cout << "  Patch " << i << ": level " << initialTessLevel
                      << " (" << rawPtr->getMeshData().vertices.size() << " vertices)" << std::endl;
        }

        std::cout << "  Loaded " << m_patchObjects.size() << " patches" << std::endl;
        return true;
    }
#else
    (void)path;
    (void)scene;
    (void)initialTessLevel;
    std::cerr << "MultiPatchManager: G+Smo support not available" << std::endl;
    return false;
#endif
}

void MultiPatchManager::updateTessellation(const Camera& camera, float aspectRatio,
                                           int viewportWidth, int viewportHeight) {
    // Skip if auto-refinement is disabled or no patches
    if (!m_autoRefinement || m_patchObjects.empty()) {
        return;
    }

    // Compute view-projection matrix using camera's actual projection (respects animated FOV)
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);
    glm::mat4 viewProj = proj * view;

    for (PatchObject* patch : m_patchObjects) {
        if (!patch || patch->isRetessellating()) {
            continue;
        }

        // Calculate screen-space size of this patch
        float screenSize = calculateScreenSize(patch->getWorldBounds(), viewProj,
                                               viewportWidth, viewportHeight);

        // Determine target tessellation level
        int targetLevel = calculateTessLevel(screenSize);

        // Apply hysteresis to prevent rapid switching
        int currentLevel = patch->getTessellationLevel();
        int pendingLevel = patch->getPendingTessellationLevel();

        // Only trigger re-tessellation if the change is significant
        float levelRatio = static_cast<float>(targetLevel) / static_cast<float>(currentLevel);
        bool shouldRefine = levelRatio > (1.0f + m_hysteresisFactor);
        bool shouldCoarsen = levelRatio < (1.0f - m_hysteresisFactor);

        if ((shouldRefine || shouldCoarsen) && targetLevel != pendingLevel) {
            patch->requestTessellation(targetLevel);

            // Submit background tessellation task
            patch->setRetessellating(true);

            // Create tessellation callback - use solution if available
#ifdef GISMO_AVAILABLE
            int patchIndex = patch->getPatchIndex();
            gismo::gsGeometry<>* patchPtr = &m_multipatch->patch(patchIndex);

            // Capture solution pointer for use in tessellation
            const PoissonSolution* solPtr = m_poissonManager.hasSolution() ?
                &m_poissonManager.getSolution() : nullptr;

            auto tessFunc = [patchPtr, solPtr, patchIndex](int u, int v) -> MeshData {
                if (solPtr && solPtr->valid) {
                    return GismoLoader::tessellatePatchWithSolution(*patchPtr, u, v, solPtr, patchIndex);
                } else {
                    return GismoLoader::tessellatePatch(*patchPtr, u, v);
                }
            };

            auto task = std::make_unique<TessellationTask>(
                patch, patch->getName(), tessFunc, targetLevel);
            m_tessManager.submitTask(std::move(task));
#endif
        }
    }
}

void MultiPatchManager::processCompletedTasks() {
    m_tessManager.processCompletedTasks();

    // Check for completed Poisson solving
    int poissonCompleted = m_poissonManager.processCompletedTasks();
    if (poissonCompleted > 0 && m_poissonManager.hasSolution()) {
        // Re-tessellate all patches with solution values
        retessellateWithSolution();
        // Signal that solution visualization should be enabled
        m_solutionReady = true;
    }
}

void MultiPatchManager::startPoissonSolving() {
#ifdef GISMO_AVAILABLE
    if (!m_hasBVPData || !m_multipatch) {
        std::cerr << "MultiPatchManager: Cannot solve Poisson - no BVP data available" << std::endl;
        return;
    }

    if (m_poissonManager.isBusy()) {
        std::cerr << "MultiPatchManager: Poisson solver already running" << std::endl;
        return;
    }

    // Extract filename for display
    std::string name = m_loadedFilePath;
    size_t lastSlash = m_loadedFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        name = m_loadedFilePath.substr(lastSlash + 1);
    }

    m_poissonManager.startSolving(m_loadedFilePath, name, m_multipatch.get());
#else
    std::cerr << "MultiPatchManager: G+Smo support not available" << std::endl;
#endif
}

void MultiPatchManager::retessellateWithSolution() {
#ifdef GISMO_AVAILABLE
    if (!m_poissonManager.hasSolution() || !m_multipatch) {
        return;
    }

    const PoissonSolution& solution = m_poissonManager.getSolution();

    // Use high resolution for solution display
    const int solutionTessLevel = 64;

    for (PatchObject* patch : m_patchObjects) {
        if (!patch) continue;

        int patchIndex = patch->getPatchIndex();

        // Use higher tessellation for solution visualization
        int level = std::max(patch->getTessellationLevel(), solutionTessLevel);

        // Tessellate with solution values
        MeshData meshData = GismoLoader::tessellatePatchWithSolution(
            m_multipatch->patch(patchIndex), level, level, &solution, patchIndex);

        // Update tessellation level and apply mesh data
        patch->setTessellationLevel(level);
        patch->setMeshData(meshData);
    }
#endif
}

int MultiPatchManager::calculateTessLevel(float screenSize) const {
    if (screenSize <= m_thresholds.minScreenSize) {
        return m_thresholds.minLevel;
    }
    if (screenSize >= m_thresholds.maxScreenSize) {
        return m_thresholds.maxLevel;
    }

    // Logarithmic interpolation for smoother level transitions
    float t = (std::log(screenSize) - std::log(m_thresholds.minScreenSize)) /
              (std::log(m_thresholds.maxScreenSize) - std::log(m_thresholds.minScreenSize));

    // Round to power-of-2-ish levels for better tessellation quality
    float levelF = m_thresholds.minLevel * std::pow(
        static_cast<float>(m_thresholds.maxLevel) / m_thresholds.minLevel, t);

    // Snap to reasonable levels: 4, 8, 12, 16, 24, 32, 48, 64, 128
    static const int levels[] = {4, 8, 12, 16, 24, 32, 48, 64, 128};
    int closestLevel = levels[0];
    float closestDist = std::abs(levelF - levels[0]);

    for (int level : levels) {
        float dist = std::abs(levelF - level);
        if (dist < closestDist) {
            closestDist = dist;
            closestLevel = level;
        }
    }

    return closestLevel;
}

float MultiPatchManager::calculateScreenSize(const BoundingBox& bounds, const glm::mat4& viewProj,
                                             int viewportWidth, int viewportHeight) const {
    if (!bounds.isValid()) {
        return 0.0f;
    }

    // Project bounding box corners to screen space
    glm::vec3 corners[8];
    bounds.getCorners(corners);

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    bool anyVisible = false;
    bool anyBehind = false;

    for (int i = 0; i < 8; ++i) {
        glm::vec4 clipPos = viewProj * glm::vec4(corners[i], 1.0f);

        // Track if any corners are behind camera
        if (clipPos.w <= 0.0f) {
            anyBehind = true;
            continue;
        }

        // Perspective divide
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

        // Convert to screen coordinates
        float screenX = (ndc.x * 0.5f + 0.5f) * viewportWidth;
        float screenY = (ndc.y * 0.5f + 0.5f) * viewportHeight;

        minX = std::min(minX, screenX);
        maxX = std::max(maxX, screenX);
        minY = std::min(minY, screenY);
        maxY = std::max(maxY, screenY);
        anyVisible = true;
    }

    // If some corners are behind camera but some are visible,
    // the object likely fills a large portion of the screen - use max detail
    if (anyBehind && anyVisible) {
        return m_thresholds.maxScreenSize + 1.0f;  // Force max tessellation
    }

    // If all corners behind camera, object is very close - use max detail
    if (!anyVisible) {
        return m_thresholds.maxScreenSize + 1.0f;
    }

    // Return the larger dimension (width or height in pixels)
    return std::max(maxX - minX, maxY - minY);
}
