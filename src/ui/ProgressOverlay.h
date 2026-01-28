#pragma once

#include "util/TextRenderer.h"
#include <string>

class SubdivisionManager;
class LODManager;
class MultiPatchManager;

class ProgressOverlay {
public:
    ProgressOverlay() = default;
    ~ProgressOverlay() = default;

    void setTextRenderer(TextRenderer* renderer) { m_textRenderer = renderer; }
    void render(int screenWidth, int screenHeight,
                const SubdivisionManager* subdivManager,
                const LODManager* lodManager = nullptr,
                const MultiPatchManager* multipatchManager = nullptr);

private:
    void renderProgressBar(float x, float y, float width, float height, float progress);

    TextRenderer* m_textRenderer{nullptr};
};
