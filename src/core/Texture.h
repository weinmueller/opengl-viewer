#pragma once

#include <glad/gl.h>
#include <string>

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string& path);
    void bind(GLuint unit = 0) const;

    GLuint getID() const { return m_textureID; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isValid() const { return m_textureID != 0; }

private:
    GLuint m_textureID{0};
    int m_width{0};
    int m_height{0};
};
