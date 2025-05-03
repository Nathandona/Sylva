#include "Model.h"
#include <iostream>
#include <unordered_map>

// Define TINYOBJLOADER_IMPLEMENTATION before including tiny_obj_loader.h
#define TINYOBJLOADER_IMPLEMENTATION
#include "../../vendor/tiny_obj_loader/tiny_obj_loader.h"

namespace Sylva {

Model::Model() {
}

Model::~Model() {
    // Smart pointers will handle cleanup automatically
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
        
        // Create a mesh and add it to the model using unique_ptr
        auto mesh = std::make_unique<Mesh>();
        // Stride: 3 (pos) + 3 (normal) + 2 (texcoord) = 8 floats per vertex
        mesh->SetVertexData(vertices, indices, 8, 0, 3, 6);
        m_Meshes.push_back(std::move(mesh));
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
                auto texture = std::make_shared<Texture>();
                if (texture->LoadFromFile(texturePath)) {
                    m_Textures.push_back(texture);
                }
                // No need to delete on failure as shared_ptr handles it
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

} // namespace Sylva 