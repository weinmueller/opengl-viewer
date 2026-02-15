#include "ProgressOverlay.h"
#include "geometry/SubdivisionManager.h"
#include "lod/LODManager.h"
#include "multipatch/MultiPatchManager.h"
#include "multipatch/PoissonManager.h"
#include <glm/glm.hpp>
#include <sstream>
#include <iomanip>

void ProgressOverlay::renderProgressBar(float x, float y, float width, float height, float progress) {
    // Background (dark)
    m_textRenderer->renderQuad(x, y, width, height, glm::vec4(0.2f, 0.2f, 0.25f, 1.0f));

    // Progress fill (blue gradient)
    float fillWidth = width * progress;
    if (fillWidth > 0.0f) {
        m_textRenderer->renderQuad(x, y, fillWidth, height, glm::vec4(0.3f, 0.5f, 0.9f, 1.0f));
    }

    // Border
    glm::vec4 borderColor(0.4f, 0.6f, 0.9f, 1.0f);
    float borderWidth = 1.0f;
    m_textRenderer->renderQuad(x, y, width, borderWidth, borderColor);
    m_textRenderer->renderQuad(x, y + height - borderWidth, width, borderWidth, borderColor);
    m_textRenderer->renderQuad(x, y, borderWidth, height, borderColor);
    m_textRenderer->renderQuad(x + width - borderWidth, y, borderWidth, height, borderColor);
}

void ProgressOverlay::render(int screenWidth, int screenHeight,
                             const SubdivisionManager* subdivManager,
                             const LODManager* lodManager,
                             const MultiPatchManager* multipatchManager) {
    if (!m_textRenderer) return;

    // Check if any manager is busy
    bool subdivBusy = subdivManager && subdivManager->isBusy();
    bool lodBusy = lodManager && lodManager->isBusy();
    bool tessBusy = multipatchManager && multipatchManager->isBusy();
    bool poissonBusy = multipatchManager && multipatchManager->isSolvingPoisson();

    if (!subdivBusy && !lodBusy && !tessBusy && !poissonBusy) {
        return;
    }

    std::string objectName;
    float totalProgress = 0.0f;
    std::string phaseName;
    size_t queuedCount = 0;
    std::string taskType;
    ProgressSnapshot snapshot;

    if (subdivBusy) {
        if (!subdivManager->getActiveProgressSnapshot(snapshot)) return;

        objectName = subdivManager->getActiveObjectName();
        totalProgress = snapshot.totalProgress;
        phaseName = snapshot.phaseName;
        queuedCount = subdivManager->getQueuedTaskCount();
        taskType = "Subdividing";
    } else if (lodBusy) {
        if (!lodManager->getActiveProgressSnapshot(snapshot)) return;

        objectName = lodManager->getActiveObjectName();
        totalProgress = snapshot.totalProgress;
        phaseName = snapshot.phaseName;
        queuedCount = lodManager->getQueuedTaskCount();
        taskType = "Generating LOD";
    } else if (poissonBusy) {
        const PoissonManager* poissonMgr = multipatchManager->getPoissonManager();
        if (!poissonMgr->getActiveProgressSnapshot(snapshot)) return;

        objectName = poissonMgr->getActiveObjectName();
        totalProgress = snapshot.totalProgress;
        phaseName = snapshot.phaseName;
        queuedCount = poissonMgr->getQueuedTaskCount();
        taskType = "Solving Poisson";
    } else if (tessBusy) {
        if (!multipatchManager->getActiveProgressSnapshot(snapshot)) return;

        objectName = multipatchManager->getActiveObjectName();
        totalProgress = snapshot.totalProgress;
        phaseName = snapshot.phaseName;
        queuedCount = multipatchManager->getQueuedTaskCount();
        taskType = "Tessellating";
    }

    // Overlay dimensions
    const float scale = 1.5f;
    const float charW = TextRenderer::getCharWidth() * scale;
    const float charH = TextRenderer::getCharHeight() * scale;
    const float padding = 10.0f;
    const float progressBarHeight = 16.0f;
    const float overlayWidth = 300.0f;
    const float overlayHeight = charH * 3 + progressBarHeight + padding * 4;

    // Position at bottom center
    float overlayX = (screenWidth - overlayWidth) / 2.0f;
    float overlayY = screenHeight - overlayHeight - 20.0f;

    m_textRenderer->begin(screenWidth, screenHeight);

    // Draw background
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, overlayHeight,
                               glm::vec4(0.1f, 0.1f, 0.15f, 0.95f));

    // Draw border
    float borderWidth = 2.0f;
    glm::vec4 borderColor(0.4f, 0.6f, 0.9f, 1.0f);
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY + overlayHeight - borderWidth, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY, borderWidth, overlayHeight, borderColor);
    m_textRenderer->renderQuad(overlayX + overlayWidth - borderWidth, overlayY, borderWidth, overlayHeight, borderColor);

    // Draw object name
    std::string title = taskType + ": " + objectName;
    if (title.length() * charW > overlayWidth - 2 * padding) {
        title = title.substr(0, static_cast<size_t>((overlayWidth - 2 * padding) / charW) - 3) + "...";
    }
    m_textRenderer->renderText(title, overlayX + padding, overlayY + padding, scale,
                               glm::vec4(0.5f, 0.8f, 1.0f, 1.0f));

    // Draw phase name
    m_textRenderer->renderText(phaseName.c_str(), overlayX + padding, overlayY + padding + charH + 4.0f, scale,
                               glm::vec4(0.7f, 0.7f, 0.75f, 1.0f));

    // Draw progress bar
    float barY = overlayY + padding * 2 + charH * 2;
    float barWidth = overlayWidth - 2 * padding;
    renderProgressBar(overlayX + padding, barY, barWidth, progressBarHeight, totalProgress);

    // Draw percentage
    std::ostringstream percentStr;
    percentStr << std::fixed << std::setprecision(0) << (totalProgress * 100.0f) << "%";
    std::string pct = percentStr.str();
    float pctX = overlayX + (overlayWidth - pct.length() * charW) / 2.0f;
    m_textRenderer->renderText(pct, pctX, barY + (progressBarHeight - charH) / 2.0f, scale,
                               glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Draw queued count if any
    if (queuedCount > 0) {
        std::ostringstream queueStr;
        queueStr << "+" << queuedCount << " queued";
        float queueY = overlayY + overlayHeight - charH - padding;
        m_textRenderer->renderText(queueStr.str(), overlayX + padding, queueY, scale,
                                   glm::vec4(0.6f, 0.6f, 0.65f, 1.0f));
    }

    m_textRenderer->end();
}
