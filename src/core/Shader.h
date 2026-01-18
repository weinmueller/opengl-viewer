#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLuint getProgram() const { return m_program; }

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    std::string loadFile(const std::string& path) const;
    GLuint compileShader(GLenum type, const std::string& source) const;
    void checkCompileErrors(GLuint shader, const std::string& type) const;
    void checkLinkErrors(GLuint program) const;
    GLint getUniformLocation(const std::string& name) const;

    GLuint m_program;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
};
