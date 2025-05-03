#include "Mesh.h"

namespace Sylva {

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

} // namespace Sylva 