#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

namespace Sylva {

// Shader implementation
Shader::Shader() : m_ID(0) {
}

Shader::~Shader() {
    glDeleteProgram(m_ID);
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    // 1. Retrieve the vertex/fragment source code from filePath
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
        std::stringstream vShaderStream, fShaderStream;
        
        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        
        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();
        
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return false;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    // 2. Compile shaders
    GLuint vertex, fragment;
    
    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    if (!CheckCompileErrors(vertex, "VERTEX")) {
        glDeleteShader(vertex);
        return false;
    }
    
    // Fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    if (!CheckCompileErrors(fragment, "FRAGMENT")) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }
    
    // Shader program
    m_ID = glCreateProgram();
    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    glLinkProgram(m_ID);
    if (!CheckCompileErrors(m_ID, "PROGRAM")) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }
    
    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return true;
}

void Shader::Use() const {
    glUseProgram(m_ID);
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::CheckCompileErrors(GLuint shader, const std::string& type) {
    int success;
    char infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    
    return true;
}

// Mesh implementation
Mesh::Mesh() : m_VAO(0), m_VBO(0), m_EBO(0), m_IndexCount(0) {
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

void Mesh::SetVertexData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    m_IndexCount = indices.size();
    
    // Create buffers/arrays
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    // Load data into vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Load data into element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::Draw() const {
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Renderer implementation
Renderer::Renderer()
    : m_ClearColor(0.2f, 0.3f, 0.3f, 1.0f)
    , m_BasicShader(nullptr)
    , m_TestTriangle(nullptr)
{
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    std::cout << "Initializing Renderer..." << std::endl;
    
    // Set OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Create a basic shader for our triangle
    m_BasicShader = new Shader();
    if (!m_BasicShader->LoadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    
    // Create a test triangle for Phase 1
    CreateTriangle();
    
    return true;
}

void Renderer::BeginFrame() {
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderScene() {
    // For Phase 1, just render our test triangle
    if (m_BasicShader && m_TestTriangle) {
        m_BasicShader->Use();
        m_TestTriangle->Draw();
    }
    
    // More complex rendering will be added in later phases
}

void Renderer::EndFrame() {
    // Currently nothing to do here, but we might add post-processing, etc. later
}

void Renderer::Shutdown() {
    delete m_TestTriangle;
    m_TestTriangle = nullptr;
    
    delete m_BasicShader;
    m_BasicShader = nullptr;
}

void Renderer::SetClearColor(const glm::vec4& color) {
    m_ClearColor = color;
}

Shader* Renderer::LoadShader(const std::string& vertexPath, const std::string& fragmentPath) {
    Shader* shader = new Shader();
    if (!shader->LoadFromFiles(vertexPath, fragmentPath)) {
        delete shader;
        return nullptr;
    }
    return shader;
}

Mesh* Renderer::CreateMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Mesh* mesh = new Mesh();
    mesh->SetVertexData(vertices, indices);
    return mesh;
}

void Renderer::CreateTriangle() {
    // Define vertices for a simple triangle with position (xyz) and color (rgb)
    std::vector<float> vertices = {
        // positions         // colors
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // bottom right
    };
    
    // Define indices
    std::vector<unsigned int> indices = {
        0, 1, 2  // first triangle
    };
    
    // Create the mesh
    m_TestTriangle = CreateMesh(vertices, indices);
}

} 