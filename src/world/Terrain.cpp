#include "Terrain.h"
#include <iostream>
#include <glm/gtc/noise.hpp> // For future noise-based terrain

namespace Sylva {

Terrain::Terrain()
    : m_Resolution(0)
    , m_Width(0.0f)
    , m_Depth(0.0f)
    , m_Mesh(nullptr)
    , m_Texture(nullptr)
{
}

Terrain::~Terrain() {
    // We assume the renderer owns and will cleanup the mesh and texture
    m_Mesh = nullptr;
    m_Texture = nullptr;
}

bool Terrain::Initialize(float width, float depth, unsigned int resolution) {
    m_Width = width;
    m_Depth = depth;
    m_Resolution = resolution;
    
    // Generate flat terrain mesh
    GenerateFlatTerrain(width, depth, resolution);
    
    return true;
}

bool Terrain::InitializeFromHeightmap(const std::string& heightmapPath, 
                                     float width, float depth, 
                                     float heightScale) {
    // For MVP, we'll just create a flat terrain
    // Future implementation: load heightmap image and extract height data
    std::cout << "Heightmap loading not yet implemented, creating flat terrain instead." << std::endl;
    
    return Initialize(width, depth, 64); // Default to 64x64 grid
}

float Terrain::GetHeightAt(float x, float z) const {
    // For a flat terrain, always return 0.0f
    // Future implementation: interpolate between height values
    return 0.0f;
}

void Terrain::Render(Shader* shader) const {
    if (!m_Mesh) {
        std::cerr << "Terrain mesh not initialized!" << std::endl;
        return;
    }
    
    if (shader) {
        shader->Use();
        
        // Set transformation matrix (identity for now - no transformation)
        glm::mat4 model = glm::mat4(1.0f);
        shader->SetMat4("model", model);
        
        // Get view and projection matrices from the renderer
        // The renderer passes these from the camera
        // Note: We're assuming the renderer has set these matrices as uniforms
        // If that's not the case, we'll need to modify our Renderer class to expose these
        
        // Set terrain texture if available
        if (m_Texture) {
            m_Texture->Bind(0);
            shader->SetInt("textureSampler", 0);
        } else {
            // Debug: If no texture is available, set a flag
            shader->SetInt("textureSampler", 0); // Still set it to avoid errors
            std::cerr << "Warning: Rendering terrain without texture!" << std::endl;
        }
    }
    
    // Draw the mesh
    m_Mesh->Draw();
}

void Terrain::SetTexture(Texture* texture) {
    m_Texture = texture;
}

void Terrain::GenerateFlatTerrain(float width, float depth, unsigned int resolution) {
    // Initialize height data (all zeros for flat terrain)
    m_HeightData.clear();
    m_HeightData.resize(resolution * resolution, 0.0f);
    
    // Generate vertices and indices based on the flat height data
    GenerateVertices();
    GenerateIndices(resolution);
    
    // Create the mesh object
    if (m_Mesh) {
        // If we already have a mesh, assume it's managed elsewhere and just update data
        m_Mesh->SetVertexData(m_Vertices, m_Indices, 8, 0, 3, 6);
    } else {
        // Otherwise create a new mesh (this is a simplified version)
        // In a real implementation, this would be handled by the renderer
        m_Mesh = new Mesh();
        m_Mesh->SetVertexData(m_Vertices, m_Indices, 8, 0, 3, 6);
    }
}

void Terrain::GenerateVertices() {
    m_Vertices.clear();
    
    float halfWidth = m_Width * 0.5f;
    float halfDepth = m_Depth * 0.5f;
    
    float xStep = m_Width / (float)(m_Resolution - 1);
    float zStep = m_Depth / (float)(m_Resolution - 1);
    
    float texU = 1.0f / (float)(m_Resolution - 1);
    float texV = 1.0f / (float)(m_Resolution - 1);
    
    // Debug output
    std::cout << "Generating terrain vertices: " << m_Resolution << "x" << m_Resolution << " resolution" << std::endl;
    std::cout << "Terrain size: " << m_Width << "x" << m_Depth << " units" << std::endl;
    
    // Generate vertices with positions, normals, and texture coordinates
    for (unsigned int z = 0; z < m_Resolution; z++) {
        for (unsigned int x = 0; x < m_Resolution; x++) {
            // Calculate position
            float xPos = -halfWidth + x * xStep;
            float yPos = m_HeightData[z * m_Resolution + x]; // Always 0 for flat terrain
            float zPos = -halfDepth + z * zStep;
            
            // Calculate normal (always up for flat terrain)
            float nx = 0.0f;
            float ny = 1.0f;
            float nz = 0.0f;
            
            // Calculate texture coordinate
            float u = x * texU;
            float v = z * texV;
            
            // Add vertex data: position (3), normal (3), texcoord (2)
            m_Vertices.push_back(xPos);
            m_Vertices.push_back(yPos);
            m_Vertices.push_back(zPos);
            
            m_Vertices.push_back(nx);
            m_Vertices.push_back(ny);
            m_Vertices.push_back(nz);
            
            m_Vertices.push_back(u);
            m_Vertices.push_back(v);
        }
    }
    
    // Debug output
    std::cout << "Generated " << m_Vertices.size() / 8 << " vertices for terrain" << std::endl;
    std::cout << "Total vertex data size: " << m_Vertices.size() * sizeof(float) << " bytes" << std::endl;
}

void Terrain::GenerateIndices(unsigned int resolution) {
    m_Indices.clear();
    
    // Generate indices for triangle strips
    for (unsigned int z = 0; z < resolution - 1; z++) {
        for (unsigned int x = 0; x < resolution - 1; x++) {
            // Calculate the indices of the quad's four corners
            unsigned int topLeft = z * resolution + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * resolution + x;
            unsigned int bottomRight = bottomLeft + 1;
            
            // First triangle (top-left, bottom-left, top-right)
            m_Indices.push_back(topLeft);
            m_Indices.push_back(bottomLeft);
            m_Indices.push_back(topRight);
            
            // Second triangle (top-right, bottom-left, bottom-right)
            m_Indices.push_back(topRight);
            m_Indices.push_back(bottomLeft);
            m_Indices.push_back(bottomRight);
        }
    }
}

} // namespace Sylva 