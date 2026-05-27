#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace Sylva {

/**
 * @brief Shader management class
 *
 * Handles loading, compiling, and using OpenGL shaders.
 * Allows setting uniform variables for shaders.
 */
class Shader {
public:
    /**
     * @brief Constructor to load and compile shaders from files
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Destructor releases the GL program.
     */
    ~Shader();

    // Non-copyable — owns a GL handle.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Movable.
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Get the program ID
     * @return The OpenGL shader program ID
     */
    unsigned int getID() const { return m_ID; }

    /**
     * @brief Activate the shader program
     */
    void use() const;

    /**
     * @brief Set a boolean uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setBool(const std::string& name, bool value) const;

    /**
     * @brief Set an integer uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setInt(const std::string& name, int value) const;

    /**
     * @brief Set a float uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setFloat(const std::string& name, float value) const;

    /**
     * @brief Set a vec2 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setVec2(const std::string& name, const glm::vec2& value) const;

    /**
     * @brief Set a vec3 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setVec3(const std::string& name, const glm::vec3& value) const;

    /**
     * @brief Set a vec4 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setVec4(const std::string& name, const glm::vec4& value) const;

    /**
     * @brief Set a mat2 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setMat2(const std::string& name, const glm::mat2& mat) const;

    /**
     * @brief Set a mat3 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setMat3(const std::string& name, const glm::mat3& mat) const;

    /**
     * @brief Set a mat4 uniform variable
     * @param name Name of the uniform variable
     * @param value Value to set
     */
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    /**
     * @brief Check for compilation or linking errors
     * @param shader The shader or program ID
     * @param type The type of check ("VERTEX", "FRAGMENT", or "PROGRAM")
     */
    void checkCompileErrors(unsigned int shader, const std::string& type);

    /// Cached uniform location lookup; misses fall through to glGetUniformLocation.
    GLint getUniformLocation(const std::string& name) const;

    // OpenGL program ID (0 == invalid/moved-from)
    unsigned int m_ID = 0;

    // Uniform-name -> location cache. mutable so const setters can populate it.
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
};

} // namespace Sylva