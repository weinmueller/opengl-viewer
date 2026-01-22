#pragma once

#include "core/Shader.h"
#include <glad/gl.h>
#include <string>
#include <vector>
#include <memory>

// Toggle states for help overlay display
struct ToggleStates {
    bool wireframe{false};
    bool backfaceCulling{true};
    bool frustumCulling{true};
    bool lodEnabled{true};
    bool lodDebugColors{false};
};

class HelpOverlay {
public:
    HelpOverlay();
    ~HelpOverlay();

    void init();
    void render(int screenWidth, int screenHeight, const ToggleStates& toggles);

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

private:
    void createFontTexture();
    void renderText(const std::string& text, float x, float y, float scale);
    void renderQuad(float x, float y, float width, float height);

    std::unique_ptr<Shader> m_textShader;
    GLuint m_fontTexture{0};
    GLuint m_vao{0};
    GLuint m_vbo{0};

    bool m_visible{false};
    int m_screenWidth{0};
    int m_screenHeight{0};

    static constexpr int FONT_CHAR_WIDTH = 8;
    static constexpr int FONT_CHAR_HEIGHT = 8;
    static constexpr int FONT_FIRST_CHAR = 32;  // Space
    static constexpr int FONT_CHAR_COUNT = 95;  // Printable ASCII
    static constexpr int FONT_COLS = 16;
    static constexpr int FONT_ROWS = 6;
};
