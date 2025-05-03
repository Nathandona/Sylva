#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"

namespace Sylva {

// Model class to hold multiple meshes loaded from an .obj file
class Model {
public:
    Model();
    ~Model();
    
    bool LoadFromFile(const std::string& path);
    void Draw(Shader* shader) const;
    
private:
    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::shared_ptr<Texture>> m_Textures;
    std::string m_Directory;
};

} // namespace Sylva 