#pragma once

#include "util/TextRenderer.h"
#include <string>
#include <vector>

// Toggle states for help overlay display
struct ToggleStates {
    bool wireframe{false};
    bool backfaceCulling{true};
    bool frustumCulling{true};
    bool lodEnabled{true};
    bool lodDebugColors{false};
    bool texturesEnabled{true};
    // Triangle stats
    uint32_t renderedTriangles{0};
    uint32_t originalTriangles{0};
    float lodSavingsPercent{0.0f};
};

class HelpOverlay {
public:
    HelpOverlay() = default;
    ~HelpOverlay() = default;

    void setTextRenderer(TextRenderer* renderer) { m_textRenderer = renderer; }
    void render(int screenWidth, int screenHeight, const ToggleStates& toggles);
    void renderStats(int screenWidth, int screenHeight, const ToggleStates& toggles);

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

private:
    TextRenderer* m_textRenderer{nullptr};
    bool m_visible{false};
};
