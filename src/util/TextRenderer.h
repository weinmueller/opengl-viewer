#pragma once

#include "core/Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

// Shared bitmap font text renderer for UI overlays
// Uses an 8x8 bitmap font supporting ASCII 32-126
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize GPU resources (call after OpenGL context is ready)
    void init();

    // Begin a rendering batch (sets up state)
    void begin(int screenWidth, int screenHeight);

    // End a rendering batch (restores state)
    void end();

    // Render text at position with scale and color
    void renderText(const std::string& text, float x, float y, float scale,
                   const glm::vec4& color);

    // Render a solid quad (for backgrounds, borders, progress bars)
    void renderQuad(float x, float y, float width, float height,
                   const glm::vec4& color);

    // Get character dimensions
    static constexpr int getCharWidth() { return GLYPH_WIDTH; }
    static constexpr int getCharHeight() { return GLYPH_HEIGHT; }

private:
    void createFontTexture();

    std::unique_ptr<Shader> m_shader;
    GLuint m_fontTexture{0};
    GLuint m_vao{0};
    GLuint m_vbo{0};

    int m_screenWidth{0};
    int m_screenHeight{0};

    static constexpr int GLYPH_WIDTH = 8;
    static constexpr int GLYPH_HEIGHT = 8;
    static constexpr int FIRST_CHAR = 32;   // Space
    static constexpr int CHAR_COUNT = 95;   // Printable ASCII
    static constexpr int FONT_COLS = 16;
    static constexpr int FONT_ROWS = 6;
};
