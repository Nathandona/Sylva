#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "../platform/Platform.h"

namespace Sylva {

// Forward declarations
class Platform;

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

// Simple mesh class
class Mesh {
public:
    Mesh();
    ~Mesh();
    
    void SetVertexData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void Draw() const;
    
private:
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
    unsigned int m_IndexCount;
};

// Main renderer class
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize();
    void BeginFrame();
    void RenderScene();
    void EndFrame();
    void Shutdown();
    
    void SetClearColor(const glm::vec4& color);
    
    // In a larger engine, these would be in separate classes/systems
    Shader* LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    Mesh* CreateMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    
private:
    void CreateTriangle(); // Temporary test function for Phase 1
    
    // Renderer state
    glm::vec4 m_ClearColor;
    
    // Test objects for Phase 1
    Shader* m_BasicShader;
    Mesh* m_TestTriangle;
};

} 