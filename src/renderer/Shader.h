#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace Sylva {

// Simple shader class
class Shader {
public:
    Shader();
    ~Shader();
    
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void Use() const;
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    
private:
    GLuint m_ID;
    bool CheckCompileErrors(GLuint shader, const std::string& type);
};

} // namespace Sylva 