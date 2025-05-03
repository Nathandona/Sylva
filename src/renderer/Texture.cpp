#include "Texture.h"
#include <iostream>

// Include stb_image from vendor directory
#include "../../vendor/stb/stb_image.h"

namespace Sylva {

Texture::Texture() : m_ID(0), m_Width(0), m_Height(0), m_Channels(0) {
}

Texture::~Texture() {
    glDeleteTextures(1, &m_ID);
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

} // namespace Sylva 