#include "ProgressOverlay.h"
#include <glm/glm.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>

// Same 8x8 bitmap font data as HelpOverlay
static const unsigned char FONT_DATA[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00,
    // " (34)
    0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    // # (35)
    0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00,
    // $ (36)
    0x18, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x18, 0x00,
    // % (37)
    0xC6, 0xCC, 0x18, 0x30, 0x60, 0xC6, 0x86, 0x00,
    // & (38)
    0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00,
    // ' (39)
    0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ( (40)
    0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00,
    // ) (41)
    0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00,
    // * (42)
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    // + (43)
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    // , (44)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30,
    // - (45)
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    // . (46)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    // / (47)
    0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00,
    // 0 (48)
    0x7C, 0xCE, 0xDE, 0xF6, 0xE6, 0xC6, 0x7C, 0x00,
    // 1 (49)
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    // 2 (50)
    0x7C, 0xC6, 0x06, 0x1C, 0x70, 0xC6, 0xFE, 0x00,
    // 3 (51)
    0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00,
    // 4 (52)
    0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00,
    // 5 (53)
    0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00,
    // 6 (54)
    0x38, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00,
    // 7 (55)
    0xFE, 0xC6, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    // 8 (56)
    0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00,
    // 9 (57)
    0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00,
    // : (58)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
    // ; (59)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30,
    // < (60)
    0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00,
    // = (61)
    0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00,
    // > (62)
    0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00,
    // ? (63)
    0x7C, 0xC6, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00,
    // @ (64)
    0x7C, 0xC6, 0xDE, 0xDE, 0xDC, 0xC0, 0x7C, 0x00,
    // A (65)
    0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00,
    // B (66)
    0xFC, 0xC6, 0xC6, 0xFC, 0xC6, 0xC6, 0xFC, 0x00,
    // C (67)
    0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00,
    // D (68)
    0xF8, 0xCC, 0xC6, 0xC6, 0xC6, 0xCC, 0xF8, 0x00,
    // E (69)
    0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xFE, 0x00,
    // F (70)
    0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xC0, 0x00,
    // G (71)
    0x7C, 0xC6, 0xC0, 0xCE, 0xC6, 0xC6, 0x7E, 0x00,
    // H (72)
    0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00,
    // I (73)
    0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    // J (74)
    0x1E, 0x06, 0x06, 0x06, 0xC6, 0xC6, 0x7C, 0x00,
    // K (75)
    0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00,
    // L (76)
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0x00,
    // M (77)
    0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00,
    // N (78)
    0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00,
    // O (79)
    0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // P (80)
    0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0, 0xC0, 0x00,
    // Q (81)
    0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xDE, 0x7C, 0x06,
    // R (82)
    0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCC, 0xC6, 0x00,
    // S (83)
    0x7C, 0xC6, 0xC0, 0x7C, 0x06, 0xC6, 0x7C, 0x00,
    // T (84)
    0xFE, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // U (85)
    0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // V (86)
    0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00,
    // W (87)
    0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00,
    // X (88)
    0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00,
    // Y (89)
    0xC6, 0xC6, 0x6C, 0x38, 0x18, 0x18, 0x18, 0x00,
    // Z (90)
    0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00,
    // [ (91)
    0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
    // \ (92)
    0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00,
    // ] (93)
    0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00,
    // ^ (94)
    0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
    // _ (95)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
    // ` (96)
    0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    // a (97)
    0x00, 0x00, 0x7C, 0x06, 0x7E, 0xC6, 0x7E, 0x00,
    // b (98)
    0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xFC, 0x00,
    // c (99)
    0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x00,
    // d (100)
    0x06, 0x06, 0x7E, 0xC6, 0xC6, 0xC6, 0x7E, 0x00,
    // e (101)
    0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0x7C, 0x00,
    // f (102)
    0x1C, 0x36, 0x30, 0x78, 0x30, 0x30, 0x30, 0x00,
    // g (103)
    0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x7C,
    // h (104)
    0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00,
    // i (105)
    0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // j (106)
    0x06, 0x00, 0x06, 0x06, 0x06, 0xC6, 0xC6, 0x7C,
    // k (107)
    0xC0, 0xC0, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0x00,
    // l (108)
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // m (109)
    0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xC6, 0xC6, 0x00,
    // n (110)
    0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x00,
    // o (111)
    0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // p (112)
    0x00, 0x00, 0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0,
    // q (113)
    0x00, 0x00, 0x7E, 0xC6, 0xC6, 0x7E, 0x06, 0x06,
    // r (114)
    0x00, 0x00, 0xDC, 0xE6, 0xC0, 0xC0, 0xC0, 0x00,
    // s (115)
    0x00, 0x00, 0x7E, 0xC0, 0x7C, 0x06, 0xFC, 0x00,
    // t (116)
    0x30, 0x30, 0x7C, 0x30, 0x30, 0x36, 0x1C, 0x00,
    // u (117)
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0x7E, 0x00,
    // v (118)
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    // w (119)
    0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00,
    // x (120)
    0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00,
    // y (121)
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x7C,
    // z (122)
    0x00, 0x00, 0xFE, 0x0C, 0x38, 0x60, 0xFE, 0x00,
    // { (123)
    0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00,
    // | (124)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // } (125)
    0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00,
    // ~ (126)
    0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

ProgressOverlay::ProgressOverlay() = default;

ProgressOverlay::~ProgressOverlay() {
    if (m_fontTexture) glDeleteTextures(1, &m_fontTexture);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void ProgressOverlay::init() {
    m_textShader = std::make_unique<Shader>("shaders/text.vert", "shaders/text.frag");
    createFontTexture();

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);

    glNamedBufferStorage(m_vbo, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(m_vao, 1, 0);
}

void ProgressOverlay::createFontTexture() {
    const int texWidth = FONT_COLS * FONT_CHAR_WIDTH;
    const int texHeight = FONT_ROWS * FONT_CHAR_HEIGHT;

    std::vector<unsigned char> texData(texWidth * texHeight, 0);

    for (int charIdx = 0; charIdx < FONT_CHAR_COUNT; ++charIdx) {
        int col = charIdx % FONT_COLS;
        int row = charIdx / FONT_COLS;

        for (int y = 0; y < FONT_CHAR_HEIGHT; ++y) {
            unsigned char rowBits = FONT_DATA[charIdx * FONT_CHAR_HEIGHT + y];
            for (int x = 0; x < FONT_CHAR_WIDTH; ++x) {
                bool pixel = (rowBits >> (7 - x)) & 1;
                int texX = col * FONT_CHAR_WIDTH + x;
                int texY = row * FONT_CHAR_HEIGHT + y;
                texData[texY * texWidth + texX] = pixel ? 255 : 0;
            }
        }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_fontTexture);
    glTextureStorage2D(m_fontTexture, 1, GL_R8, texWidth, texHeight);
    glTextureSubImage2D(m_fontTexture, 0, 0, 0, texWidth, texHeight, GL_RED, GL_UNSIGNED_BYTE, texData.data());
    glTextureParameteri(m_fontTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_fontTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_fontTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_fontTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void ProgressOverlay::renderText(const std::string& text, float x, float y, float scale) {
    const float texWidth = static_cast<float>(FONT_COLS * FONT_CHAR_WIDTH);
    const float texHeight = static_cast<float>(FONT_ROWS * FONT_CHAR_HEIGHT);
    const float charW = FONT_CHAR_WIDTH * scale;
    const float charH = FONT_CHAR_HEIGHT * scale;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT) {
            c = ' ';
        }

        int charIdx = c - FONT_FIRST_CHAR;
        int col = charIdx % FONT_COLS;
        int row = charIdx / FONT_COLS;

        float u0 = (col * FONT_CHAR_WIDTH) / texWidth;
        float v0 = (row * FONT_CHAR_HEIGHT) / texHeight;
        float u1 = ((col + 1) * FONT_CHAR_WIDTH) / texWidth;
        float v1 = ((row + 1) * FONT_CHAR_HEIGHT) / texHeight;

        float xpos = x + i * charW;
        float ypos = y;

        float vertices[] = {
            xpos,          ypos,          u0, v0,
            xpos + charW,  ypos,          u1, v0,
            xpos,          ypos + charH,  u0, v1,

            xpos + charW,  ypos,          u1, v0,
            xpos + charW,  ypos + charH,  u1, v1,
            xpos,          ypos + charH,  u0, v1,
        };

        glNamedBufferSubData(m_vbo, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void ProgressOverlay::renderQuad(float x, float y, float width, float height) {
    float vertices[] = {
        x,         y,          0.0f, 0.0f,
        x + width, y,          0.0f, 0.0f,
        x,         y + height, 0.0f, 0.0f,

        x + width, y,          0.0f, 0.0f,
        x + width, y + height, 0.0f, 0.0f,
        x,         y + height, 0.0f, 0.0f,
    };

    glNamedBufferSubData(m_vbo, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ProgressOverlay::renderProgressBar(float x, float y, float width, float height, float progress) {
    // Background (dark)
    m_textShader->setVec4("textColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_textShader->setVec4("bgColor", glm::vec4(0.2f, 0.2f, 0.25f, 1.0f));
    renderQuad(x, y, width, height);

    // Progress fill (blue gradient)
    float fillWidth = width * progress;
    if (fillWidth > 0.0f) {
        m_textShader->setVec4("bgColor", glm::vec4(0.3f, 0.5f, 0.9f, 1.0f));
        renderQuad(x, y, fillWidth, height);
    }

    // Border
    m_textShader->setVec4("bgColor", glm::vec4(0.4f, 0.6f, 0.9f, 1.0f));
    float borderWidth = 1.0f;
    renderQuad(x, y, width, borderWidth);
    renderQuad(x, y + height - borderWidth, width, borderWidth);
    renderQuad(x, y, borderWidth, height);
    renderQuad(x + width - borderWidth, y, borderWidth, height);
}

void ProgressOverlay::render(int screenWidth, int screenHeight, const SubdivisionManager* manager) {
    if (!manager || !manager->isBusy()) {
        return;
    }

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    const SubdivisionProgress* progress = manager->getActiveProgress();
    if (!progress) {
        return;
    }

    std::string objectName = manager->getActiveObjectName();
    float totalProgress = progress->totalProgress.load(std::memory_order_relaxed);
    const char* phaseName = progress->getPhaseName();
    size_t queuedCount = manager->getQueuedTaskCount();

    // Overlay dimensions
    const float scale = 1.5f;
    const float charW = FONT_CHAR_WIDTH * scale;
    const float charH = FONT_CHAR_HEIGHT * scale;
    const float padding = 10.0f;
    const float progressBarHeight = 16.0f;
    const float overlayWidth = 300.0f;
    const float overlayHeight = charH * 3 + progressBarHeight + padding * 4;

    // Position at bottom center
    float overlayX = (screenWidth - overlayWidth) / 2.0f;
    float overlayY = screenHeight - overlayHeight - 20.0f;

    // Setup rendering state
    glViewport(0, 0, screenWidth, screenHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_textShader->use();
    m_textShader->setVec2("screenSize", glm::vec2(screenWidth, screenHeight));
    glBindTextureUnit(0, m_fontTexture);
    m_textShader->setInt("fontTexture", 0);
    glBindVertexArray(m_vao);

    // Draw background
    m_textShader->setVec4("textColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_textShader->setVec4("bgColor", glm::vec4(0.1f, 0.1f, 0.15f, 0.95f));
    renderQuad(overlayX, overlayY, overlayWidth, overlayHeight);

    // Draw border
    m_textShader->setVec4("bgColor", glm::vec4(0.4f, 0.6f, 0.9f, 1.0f));
    float borderWidth = 2.0f;
    renderQuad(overlayX, overlayY, overlayWidth, borderWidth);
    renderQuad(overlayX, overlayY + overlayHeight - borderWidth, overlayWidth, borderWidth);
    renderQuad(overlayX, overlayY, borderWidth, overlayHeight);
    renderQuad(overlayX + overlayWidth - borderWidth, overlayY, borderWidth, overlayHeight);

    // Draw object name
    m_textShader->setVec4("bgColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_textShader->setVec4("textColor", glm::vec4(0.5f, 0.8f, 1.0f, 1.0f));
    std::string title = "Subdividing: " + objectName;
    if (title.length() * charW > overlayWidth - 2 * padding) {
        title = title.substr(0, static_cast<size_t>((overlayWidth - 2 * padding) / charW) - 3) + "...";
    }
    renderText(title, overlayX + padding, overlayY + padding, scale);

    // Draw phase name
    m_textShader->setVec4("textColor", glm::vec4(0.7f, 0.7f, 0.75f, 1.0f));
    renderText(phaseName, overlayX + padding, overlayY + padding + charH + 4.0f, scale);

    // Draw progress bar
    float barY = overlayY + padding * 2 + charH * 2;
    float barWidth = overlayWidth - 2 * padding;
    renderProgressBar(overlayX + padding, barY, barWidth, progressBarHeight, totalProgress);

    // Draw percentage
    std::ostringstream percentStr;
    percentStr << std::fixed << std::setprecision(0) << (totalProgress * 100.0f) << "%";
    m_textShader->setVec4("bgColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_textShader->setVec4("textColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    std::string pct = percentStr.str();
    float pctX = overlayX + (overlayWidth - pct.length() * charW) / 2.0f;
    renderText(pct, pctX, barY + (progressBarHeight - charH) / 2.0f, scale);

    // Draw queued count if any
    if (queuedCount > 0) {
        m_textShader->setVec4("textColor", glm::vec4(0.6f, 0.6f, 0.65f, 1.0f));
        std::ostringstream queueStr;
        queueStr << "+" << queuedCount << " queued";
        float queueY = overlayY + overlayHeight - charH - padding;
        renderText(queueStr.str(), overlayX + padding, queueY, scale);
    }

    // Restore state
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
