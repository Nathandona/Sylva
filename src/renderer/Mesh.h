#pragma once

#include <glad/glad.h>
#include <vector>

namespace Sylva {

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

} // namespace Sylva 