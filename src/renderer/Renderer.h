#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "../platform/Platform.h"
#include "Camera.h"
#include "CameraController.h"

namespace Sylva {

// Forward declarations
class Platform;
class Camera;
class CameraController;

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

// Texture class for handling image loading and OpenGL texture creation
class Texture {
public:
    Texture();
    ~Texture();
    
    bool LoadFromFile(const std::string& path);
    void Bind(unsigned int slot = 0) const;
    
    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }
    inline int GetChannels() const { return m_Channels; }
    inline const std::string& GetPath() const { return m_Path; }
    
private:
    GLuint m_ID;
    int m_Width;
    int m_Height;
    int m_Channels;
    std::string m_Path;
};

// Simple mesh class
class Mesh {
public:
    Mesh();
    ~Mesh();
    
    // Updated to handle texture coordinates
    void SetVertexData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, int stride, int posOffset, int normalOffset = -1, int texCoordOffset = -1, int colorOffset = -1);
    void Draw() const;
    
private:
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
    unsigned int m_IndexCount;
};

// Model class to hold multiple meshes loaded from an .obj file
class Model {
public:
    Model();
    ~Model();
    
    bool LoadFromFile(const std::string& path);
    void Draw(Shader* shader) const;
    
private:
    std::vector<Mesh*> m_Meshes;
    std::vector<Texture*> m_Textures;
    std::string m_Directory;
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
    
    // Camera methods
    Camera* GetCamera() const { return m_Camera; }
    CameraController* GetCameraController() const { return m_CameraController; }
    void SetupCamera(Platform* platform);
    
    // In a larger engine, these would be in separate classes/systems
    Shader* LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    Mesh* CreateMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    Texture* LoadTexture(const std::string& path);
    Model* LoadModel(const std::string& path);
    
private:
    void CreateTriangle(); // Temporary test function for Phase 1
    
    // Renderer state
    glm::vec4 m_ClearColor;
    
    // Test objects for Phase 1
    Shader* m_BasicShader;
    Mesh* m_TestTriangle;
    
    // Model rendering objects
    Shader* m_TexturedShader;
    Model* m_TestModel;
    
    // Camera
    Camera* m_Camera;
    CameraController* m_CameraController;
};

} 