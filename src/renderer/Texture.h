#pragma once

#include <glad/glad.h>
#include <string>

namespace Sylva {

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

} // namespace Sylva 