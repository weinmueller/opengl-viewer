#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Texture.h"
#include <iostream>

Texture::~Texture() {
    if (m_textureID) {
        glDeleteTextures(1, &m_textureID);
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID)
    , m_width(other.m_width)
    , m_height(other.m_height)
{
    other.m_textureID = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_textureID) {
            glDeleteTextures(1, &m_textureID);
        }
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;

        other.m_textureID = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool Texture::load(const std::string& path) {
    // Clean up any previously loaded texture
    if (m_textureID) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }

    // Load image data
    stbi_set_flip_vertically_on_load(true);
    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }

    // Determine format based on channels
    GLenum internalFormat, format;
    switch (channels) {
        case 1:
            internalFormat = GL_R8;
            format = GL_RED;
            break;
        case 2:
            internalFormat = GL_RG8;
            format = GL_RG;
            break;
        case 3:
            internalFormat = GL_RGB8;
            format = GL_RGB;
            break;
        case 4:
            internalFormat = GL_RGBA8;
            format = GL_RGBA;
            break;
        default:
            std::cerr << "Unsupported channel count: " << channels << std::endl;
            stbi_image_free(data);
            return false;
    }

    // Calculate mipmap levels
    int levels = static_cast<int>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;

    // Create texture using DSA
    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
    glTextureStorage2D(m_textureID, levels, internalFormat, m_width, m_height);
    glTextureSubImage2D(m_textureID, 0, 0, 0, m_width, m_height, format, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps
    glGenerateTextureMipmap(m_textureID);

    // Set filtering parameters
    glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);

    std::cout << "Loaded texture: " << path << " (" << m_width << "x" << m_height
              << ", " << channels << " channels)" << std::endl;

    return true;
}

void Texture::bind(GLuint unit) const {
    glBindTextureUnit(unit, m_textureID);
}
