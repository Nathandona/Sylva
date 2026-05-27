#include "shader.h"
#include "../core/logger.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

namespace Sylva {

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    Logger::logInfo("Loading shader: " + std::string(vertexPath) + " and " + std::string(fragmentPath));

    // 1. Retrieve vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Ensure ifstream objects can throw exceptions
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // Read file's buffer contents into streams
        std::stringstream vShaderStream;
        std::stringstream fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();

        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        Logger::logError("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " + std::string(e.what()));
        throw std::runtime_error("Shader file read failed: " + std::string(e.what()));
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders. checkCompileErrors throws on failure; we clean up
    // partially-created GL objects so a thrown ctor leaves no leaks.
    GLuint const vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    try {
        checkCompileErrors(vertex, "VERTEX");

        GLuint const fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        try {
            checkCompileErrors(fragment, "FRAGMENT");

            m_ID = glCreateProgram();
            glAttachShader(m_ID, vertex);
            glAttachShader(m_ID, fragment);
            glLinkProgram(m_ID);
            try {
                checkCompileErrors(m_ID, "PROGRAM");
            } catch (...) {
                glDeleteProgram(m_ID);
                m_ID = 0;
                throw;
            }
        } catch (...) {
            glDeleteShader(fragment);
            throw;
        }
        glDeleteShader(fragment);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }
    glDeleteShader(vertex);

    Logger::logInfo("Shader compiled successfully: ID " + std::to_string(m_ID));
}

Shader::~Shader() {
    if (m_ID != 0) {
        glDeleteProgram(m_ID);
        m_ID = 0;
    }
}

Shader::Shader(Shader&& other) noexcept : m_ID(other.m_ID) {
    other.m_ID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_ID != 0)
            glDeleteProgram(m_ID);
        m_ID = other.m_ID;
        other.m_ID = 0;
    }
    return *this;
}

void Shader::use() const {
    glUseProgram(m_ID);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }
    GLint const loc = glGetUniformLocation(m_ID, name.c_str());
    m_uniformCache.emplace(name, loc); // cache misses too (-1) to avoid repeat lookups
    return loc;
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) const {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success = 0;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            const std::string msg = "Shader compile error (" + type + "): " + infoLog;
            Logger::logError(msg);
            throw std::runtime_error(msg);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            const std::string msg = "Shader program link error: " + std::string(infoLog);
            Logger::logError(msg);
            throw std::runtime_error(msg);
        }
    }
}

} // namespace Sylva