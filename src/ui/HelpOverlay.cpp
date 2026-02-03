#include "HelpOverlay.h"
#include <glm/glm.hpp>
#include <sstream>
#include <iomanip>

void HelpOverlay::render(int screenWidth, int screenHeight, const ToggleStates& toggles) {
    if (!m_visible || !m_textRenderer) return;

    // Help content with toggle indicators
    struct HelpLine {
        std::string text;
        int toggleType;  // 0=none, 1=wireframe, 2=backface, 3=frustum, 4=lod, 5=lodDebug, 6=textures, 7=solution
    };

    std::vector<HelpLine> helpLines = {
        {"=== KEYBOARD ===", 0},
        {"H      Help toggle", 0},
        {"W      Wireframe", 1},
        {"T      Textures", 6},
        {"C      Back-face culling", 2},
        {"G      Frustum culling", 3},
        {"L      LOD system", 4},
        {"K      LOD debug colors", 5},
        {"F      Focus", 0},
        {"S      Subdivide (smooth)", 0},
        {"D      Subdivide (midpoint)", 0},
        {"Arrows Orbit camera", 0},
        {"ESC    Cancel/Exit", 0},
    };

    // Add Poisson key if BVP data is available
    if (toggles.canSolvePoisson || toggles.hasSolution) {
        if (toggles.isSolvingPoisson) {
            helpLines.push_back({"P      Solving...", 0});
        } else if (toggles.hasSolution) {
            helpLines.push_back({"P      Solution view", 7});
        } else {
            helpLines.push_back({"P      Solve Poisson", 0});
        }
    }

    // Add animation key if animation is loaded
    if (toggles.animationLoaded) {
        helpLines.push_back({"A      Animation", 8});
    }

    helpLines.push_back({"", 0});
    helpLines.push_back({"=== MOUSE ===", 0});
    helpLines.push_back({"Left   Orbit", 0});
    helpLines.push_back({"Middle Pan", 0});
    helpLines.push_back({"Right  Select", 0});
    helpLines.push_back({"Scroll Zoom", 0});

    const float scale = 1.5f;
    const float charW = TextRenderer::getCharWidth() * scale;
    const float charH = TextRenderer::getCharHeight() * scale;
    const float lineHeight = charH + 2.0f;
    const float padding = 10.0f;

    // Calculate overlay dimensions
    size_t maxLen = 0;
    for (const auto& line : helpLines) {
        maxLen = std::max(maxLen, line.text.length());
    }
    float overlayWidth = maxLen * charW + padding * 2;
    float overlayHeight = helpLines.size() * lineHeight + padding * 2;

    // Position at top-left corner with margin
    float overlayX = 10.0f;
    float overlayY = 10.0f;

    // Colors
    const glm::vec4 headerColor(0.5f, 0.8f, 1.0f, 1.0f);    // Blue for headers
    const glm::vec4 normalColor(0.7f, 0.7f, 0.75f, 1.0f);   // Gray for normal text
    const glm::vec4 activeColor(0.4f, 1.0f, 0.5f, 1.0f);    // Bright green for ON

    m_textRenderer->begin(screenWidth, screenHeight);

    // Draw background
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, overlayHeight,
                               glm::vec4(0.1f, 0.1f, 0.15f, 0.92f));

    // Draw border
    float borderWidth = 2.0f;
    glm::vec4 borderColor(0.4f, 0.6f, 0.9f, 1.0f);
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY + overlayHeight - borderWidth, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY, borderWidth, overlayHeight, borderColor);
    m_textRenderer->renderQuad(overlayX + overlayWidth - borderWidth, overlayY, borderWidth, overlayHeight, borderColor);

    float textY = overlayY + padding;
    for (const auto& line : helpLines) {
        if (!line.text.empty()) {
            // Determine if this toggle is active
            bool isActive = false;
            if (line.toggleType == 1) isActive = toggles.wireframe;
            else if (line.toggleType == 2) isActive = toggles.backfaceCulling;
            else if (line.toggleType == 3) isActive = toggles.frustumCulling;
            else if (line.toggleType == 4) isActive = toggles.lodEnabled;
            else if (line.toggleType == 5) isActive = toggles.lodDebugColors;
            else if (line.toggleType == 6) isActive = toggles.texturesEnabled;
            else if (line.toggleType == 7) isActive = toggles.solutionVisualization;
            else if (line.toggleType == 8) isActive = toggles.animationPlaying;

            // Set color based on state
            glm::vec4 color;
            if (line.text.find("===") != std::string::npos) {
                color = headerColor;
            } else if (isActive) {
                color = activeColor;
            } else {
                color = normalColor;
            }
            m_textRenderer->renderText(line.text, overlayX + padding, textY, scale, color);
        }
        textY += lineHeight;
    }

    m_textRenderer->end();
}

void HelpOverlay::renderStats(int screenWidth, int screenHeight, const ToggleStates& toggles) {
    if (!m_textRenderer) return;

    // Format triangle count with K suffix for thousands
    auto formatTriangles = [](uint32_t count) -> std::string {
        if (count >= 1000000) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << (count / 1000000.0f) << "M";
            return ss.str();
        } else if (count >= 1000) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << (count / 1000.0f) << "K";
            return ss.str();
        }
        return std::to_string(count);
    };

    // Build stats strings
    std::string triRendered = "Tris: " + formatTriangles(toggles.renderedTriangles);
    std::string triOriginal = "Full: " + formatTriangles(toggles.originalTriangles);
    std::ostringstream savingsStream;
    savingsStream << "LOD:  " << std::fixed << std::setprecision(0) << toggles.lodSavingsPercent << "%";
    std::string triSavings = savingsStream.str();

    const float scale = 1.5f;
    const float charW = TextRenderer::getCharWidth() * scale;
    const float charH = TextRenderer::getCharHeight() * scale;
    const float lineHeight = charH + 2.0f;
    const float padding = 8.0f;

    // Calculate overlay dimensions
    size_t maxLen = std::max({triRendered.length(), triOriginal.length(), triSavings.length()});
    float overlayWidth = maxLen * charW + padding * 2;
    float overlayHeight = 3 * lineHeight + padding * 2;

    // Position at top-right corner with margin
    float overlayX = screenWidth - overlayWidth - 10.0f;
    float overlayY = 10.0f;

    // Colors
    const glm::vec4 statsColor(1.0f, 0.9f, 0.5f, 1.0f);     // Yellow for current
    const glm::vec4 normalColor(0.7f, 0.7f, 0.75f, 1.0f);   // Gray for original
    const glm::vec4 savingsColor(0.4f, 1.0f, 0.8f, 1.0f);   // Cyan for savings

    m_textRenderer->begin(screenWidth, screenHeight);

    // Draw background
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, overlayHeight,
                               glm::vec4(0.1f, 0.1f, 0.15f, 0.85f));

    // Draw border
    float borderWidth = 1.0f;
    glm::vec4 borderColor(0.3f, 0.5f, 0.7f, 1.0f);
    m_textRenderer->renderQuad(overlayX, overlayY, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY + overlayHeight - borderWidth, overlayWidth, borderWidth, borderColor);
    m_textRenderer->renderQuad(overlayX, overlayY, borderWidth, overlayHeight, borderColor);
    m_textRenderer->renderQuad(overlayX + overlayWidth - borderWidth, overlayY, borderWidth, overlayHeight, borderColor);

    float textY = overlayY + padding;

    // Rendered triangles
    m_textRenderer->renderText(triRendered, overlayX + padding, textY, scale, statsColor);
    textY += lineHeight;

    // Original triangles
    m_textRenderer->renderText(triOriginal, overlayX + padding, textY, scale, normalColor);
    textY += lineHeight;

    // LOD savings
    m_textRenderer->renderText(triSavings, overlayX + padding, textY, scale, savingsColor);

    m_textRenderer->end();
}
