#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, rotate, etc.
#include <unordered_map> // For the vertex deduplication in the Model class

// Define STB_IMAGE_IMPLEMENTATION before including stb_image.h to create the implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Define TINYOBJLOADER_IMPLEMENTATION before including tiny_obj_loader.h
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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

// Texture implementation
Texture::Texture() : m_ID(0), m_Width(0), m_Height(0), m_Channels(0) {
}

Texture::~Texture() {
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
    }
}

bool Texture::LoadFromFile(const std::string& path) {
    // Save the path
    m_Path = path;
    
    // Generate texture
    glGenTextures(1, &m_ID);
    
    // Load image data using stb_image
    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    // Determine format based on number of channels
    GLenum format;
    if (m_Channels == 1)
        format = GL_RED;
    else if (m_Channels == 3)
        format = GL_RGB;
    else if (m_Channels == 4)
        format = GL_RGBA;
    else {
        std::cerr << "Unsupported number of channels: " << m_Channels << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    // Bind and set texture data
    glBindTexture(GL_TEXTURE_2D, m_ID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Free image data
    stbi_image_free(data);
    
    return true;
}

void Texture::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

// Mesh implementation
Mesh::Mesh() : m_VAO(0), m_VBO(0), m_EBO(0), m_IndexCount(0) {
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

void Mesh::SetVertexData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, 
                         int stride, int posOffset, int normalOffset, int texCoordOffset, int colorOffset) {
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
    if (posOffset >= 0) {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(posOffset * sizeof(float)));
        glEnableVertexAttribArray(0);
    }
    
    // Normal attribute
    if (normalOffset >= 0) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(normalOffset * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    
    // Texture coordinate attribute
    if (texCoordOffset >= 0) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(texCoordOffset * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    
    // Color attribute
    if (colorOffset >= 0) {
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(colorOffset * sizeof(float)));
        glEnableVertexAttribArray(3);
    }
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::Draw() const {
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Model implementation
Model::Model() {
}

Model::~Model() {
    // Clean up meshes
    for (auto mesh : m_Meshes) {
        delete mesh;
    }
    
    // Clean up textures
    for (auto texture : m_Textures) {
        delete texture;
    }
}

bool Model::LoadFromFile(const std::string& path) {
    // Extract directory path for loading material textures
    m_Directory = path.substr(0, path.find_last_of('/') + 1);
    if (m_Directory == path) {
        // Try with backslash
        m_Directory = path.substr(0, path.find_last_of('\\') + 1);
    }
    
    // Load .obj file using tinyobjloader
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), m_Directory.c_str());
    
    if (!warn.empty()) {
        std::cout << "TinyObjLoader Warning: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "TinyObjLoader Error: " << err << std::endl;
    }
    
    if (!success) {
        return false;
    }
    
    // Process shapes (meshes)
    for (const auto& shape : shapes) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<std::string, int> uniqueVertices;
        
        // For each face
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // For each vertex in the face
            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                
                // Get position, normal, and texture coordinates
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                
                float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }
                
                float tx = 0.0f, ty = 0.0f;
                if (idx.texcoord_index >= 0) {
                    tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                }
                
                // Create a unique vertex by combining position, normal, and texcoord indices
                std::string vertexKey = 
                    std::to_string(idx.vertex_index) + ":" + 
                    std::to_string(idx.normal_index) + ":" + 
                    std::to_string(idx.texcoord_index);
                
                if (uniqueVertices.count(vertexKey) == 0) {
                    // Add vertex data
                    vertices.push_back(vx);
                    vertices.push_back(vy);
                    vertices.push_back(vz);
                    
                    vertices.push_back(nx);
                    vertices.push_back(ny);
                    vertices.push_back(nz);
                    
                    vertices.push_back(tx);
                    vertices.push_back(ty);
                    
                    uniqueVertices[vertexKey] = vertices.size() / 8 - 1;
                }
                
                indices.push_back(uniqueVertices[vertexKey]);
            }
        }
        
        // Create a mesh and add it to the model
        Mesh* mesh = new Mesh();
        // Stride: 3 (pos) + 3 (normal) + 2 (texcoord) = 8 floats per vertex
        mesh->SetVertexData(vertices, indices, 8, 0, 3, 6);
        m_Meshes.push_back(mesh);
    }
    
    // Load material textures (if any)
    for (const auto& material : materials) {
        if (!material.diffuse_texname.empty()) {
            std::string texturePath = m_Directory + material.diffuse_texname;
            
            // Check if texture was already loaded
            bool textureAlreadyLoaded = false;
            for (const auto& texture : m_Textures) {
                // This is a simplistic check - in a real engine, you'd have a more robust texture cache
                if (texturePath == texture->GetPath()) {
                    textureAlreadyLoaded = true;
                    break;
                }
            }
            
            if (!textureAlreadyLoaded) {
                Texture* texture = new Texture();
                if (texture->LoadFromFile(texturePath)) {
                    m_Textures.push_back(texture);
                } else {
                    delete texture;
                }
            }
        }
    }
    
    return true;
}

void Model::Draw(Shader* shader) const {
    // Bind textures (if any)
    for (size_t i = 0; i < m_Textures.size(); i++) {
        m_Textures[i]->Bind(i);
        // Set sampler uniform in shader
        shader->SetInt("texture" + std::to_string(i), i);
    }
    
    // Draw all meshes
    for (const auto& mesh : m_Meshes) {
        mesh->Draw();
    }
}

// Renderer implementation
Renderer::Renderer()
    : m_ClearColor(0.53f, 0.81f, 0.92f, 1.0f)  // Light blue sky color
    , m_BasicShader(nullptr)
    , m_TestTriangle(nullptr)
    , m_TexturedShader(nullptr)
    , m_TestModel(nullptr)
    , m_Camera(nullptr)
    , m_CameraController(nullptr)
{
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    std::cout << "Initializing Renderer..." << std::endl;
    
    // Set OpenGL state
    glEnable(GL_DEPTH_TEST);
    // Temporarily disable face culling for debugging visibility issues
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    
    // Create a basic shader for our triangle
    m_BasicShader = new Shader();
    if (!m_BasicShader->LoadFromFiles("assets/shaders/basic.vert", "assets/shaders/basic.frag")) {
        std::cerr << "Failed to load basic shaders" << std::endl;
        return false;
    }
    
    // Create shader for textured models
    m_TexturedShader = new Shader();
    if (!m_TexturedShader->LoadFromFiles("assets/shaders/textured.vert", "assets/shaders/textured.frag")) {
        std::cerr << "Failed to load textured shaders" << std::endl;
        return false;
    }
    
    // Load a test model
    m_TestModel = LoadModel("assets/models/cube.obj");
    if (!m_TestModel) {
        std::cerr << "Failed to load cube model" << std::endl;
        // Continue anyway - this isn't fatal
    }
    
    // Initialize camera (platform-specific setup is done later)
    m_Camera = new Camera(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    
    return true;
}

void Renderer::SetupCamera(Platform* platform) {
    if (!m_Camera) {
        std::cerr << "Camera not initialized" << std::endl;
        return;
    }
    
    // Update aspect ratio based on window size
    float aspectRatio = static_cast<float>(platform->GetWidth()) / static_cast<float>(platform->GetHeight());
    m_Camera->SetAspectRatio(aspectRatio);
    
    // Create the camera controller
    if (!m_CameraController) {
        m_CameraController = new CameraController(m_Camera, platform);
        m_CameraController->SetControlMode(CameraController::ControlMode::ThirdPerson);
        m_CameraController->SetOrbitDistance(8.0f); // Better distance for Cube World style
        
        // Set initial camera rotation to look from behind the player
        m_Camera->SetRotation(-15.0f, -180.0f); // Slight downward angle
    }
}

void Renderer::BeginFrame() {
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderScene() {
    // If we have a model, shader, and camera, render the model
    if (m_TexturedShader && m_TestModel && m_Camera) {
        m_TexturedShader->Use();
        
        // Set up transformation matrices
        static float rotation = 0.0f;
        rotation += 0.01f; // Simple rotation
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, 0.0f, -2.0f)); // Move the cube to the right and away
        model = glm::rotate(model, rotation, glm::vec3(0.5f, 1.0f, 0.0f)); // Rotate around an axis
        
        // Use camera matrices
        glm::mat4 view = m_Camera->GetViewMatrix();
        glm::mat4 projection = m_Camera->GetProjectionMatrix();
        
        m_TexturedShader->SetMat4("model", model);
        m_TexturedShader->SetMat4("view", view);
        m_TexturedShader->SetMat4("projection", projection);
        m_TexturedShader->SetBool("hasTexture", false); // No texture for now
        
        m_TestModel->Draw(m_TexturedShader);
    }
}

void Renderer::EndFrame() {
    // Currently nothing to do here
}

void Renderer::Shutdown() {
    delete m_TestTriangle;
    m_TestTriangle = nullptr;
    
    delete m_BasicShader;
    m_BasicShader = nullptr;
    
    delete m_TexturedShader;
    m_TexturedShader = nullptr;
    
    delete m_TestModel;
    m_TestModel = nullptr;
    
    delete m_CameraController;
    m_CameraController = nullptr;
    
    delete m_Camera;
    m_Camera = nullptr;
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
    mesh->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
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
    m_TestTriangle = new Mesh();
    m_TestTriangle->SetVertexData(vertices, indices, 6, 0, -1, -1, 3);
}

// New renderer functions for textures and models
Texture* Renderer::LoadTexture(const std::string& path) {
    Texture* texture = new Texture();
    if (!texture->LoadFromFile(path)) {
        delete texture;
        return nullptr;
    }
    return texture;
}

Model* Renderer::LoadModel(const std::string& path) {
    Model* model = new Model();
    if (!model->LoadFromFile(path)) {
        delete model;
        return nullptr;
    }
    return model;
}

} 