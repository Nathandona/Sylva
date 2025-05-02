#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../renderer/Renderer.h"

namespace Sylva {

class Terrain {
public:
    Terrain();
    ~Terrain();

    // Initialize the terrain with a flat plane of given width and depth
    bool Initialize(float width, float depth, unsigned int resolution);
    
    // Initialize the terrain with a heightmap
    bool InitializeFromHeightmap(const std::string& heightmapPath, 
                                 float width, float depth, 
                                 float heightScale);
    
    // Get the height at a specific world position
    float GetHeightAt(float x, float z) const;
    
    // Render the terrain
    void Render(Shader* shader) const;
    
    // Set the material/texture for the terrain
    void SetTexture(Texture* texture);

private:
    // Generate flat terrain mesh
    void GenerateFlatTerrain(float width, float depth, unsigned int resolution);
    
    // Generate vertices based on current heightmap data
    void GenerateVertices();
    
    // Generate indices for the terrain mesh
    void GenerateIndices(unsigned int resolution);

    // Terrain data
    std::vector<float> m_HeightData;        // Height values
    std::vector<float> m_Vertices;          // Vertex data (position, normal, texcoord)
    std::vector<unsigned int> m_Indices;    // Indices for drawing
    
    // Terrain dimensions
    unsigned int m_Resolution;              // Number of vertices per side (e.g., 64x64)
    float m_Width;                          // Physical width of terrain in world units
    float m_Depth;                          // Physical depth of terrain in world units
    
    // Rendering objects
    Mesh* m_Mesh;                           // OpenGL mesh object
    Texture* m_Texture;                     // Terrain texture
};

} // namespace Sylva 