#pragma once

#include "core/Shader.h"
#include "geometry/SubdivisionManager.h"
#include <glad/gl.h>
#include <string>
#include <memory>

class ProgressOverlay {
public:
    ProgressOverlay();
    ~ProgressOverlay();

    void init();
    void render(int screenWidth, int screenHeight, const SubdivisionManager* manager);

private:
    void createFontTexture();
    void renderText(const std::string& text, float x, float y, float scale);
    void renderQuad(float x, float y, float width, float height);
    void renderProgressBar(float x, float y, float width, float height, float progress);

    std::unique_ptr<Shader> m_textShader;
    GLuint m_fontTexture{0};
    GLuint m_vao{0};
    GLuint m_vbo{0};

    int m_screenWidth{0};
    int m_screenHeight{0};

    static constexpr int FONT_CHAR_WIDTH = 8;
    static constexpr int FONT_CHAR_HEIGHT = 8;
    static constexpr int FONT_FIRST_CHAR = 32;
    static constexpr int FONT_CHAR_COUNT = 95;
    static constexpr int FONT_COLS = 16;
    static constexpr int FONT_ROWS = 6;
};
