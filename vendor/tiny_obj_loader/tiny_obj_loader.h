#ifndef TINY_OBJ_LOADER_H_
#define TINY_OBJ_LOADER_H_

// Tiny OBJ Loader - The simplest obj loader in C++
// This is a simplified copy of the original tiny_obj_loader.h header
// Original source: https://github.com/tinyobjloader/tinyobjloader

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace tinyobj {

// Vertex attributes
struct attrib_t {
    std::vector<float> vertices;  // 'v'
    std::vector<float> normals;   // 'vn'
    std::vector<float> texcoords; // 'vt'
};

// Material
struct material_t {
    std::string name;
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float transmittance[3];
    float emission[3];
    float shininess;
    float ior;       // index of refraction
    float dissolve;  // 1 == opaque; 0 == fully transparent
    int illum;       // illumination model

    std::string ambient_texname;
    std::string diffuse_texname;
    std::string specular_texname;
    std::string specular_highlight_texname;
    std::string bump_texname;
    std::string displacement_texname;
    std::string alpha_texname;
};

// Face indices
struct index_t {
    int vertex_index;
    int normal_index;
    int texcoord_index;
};

// Mesh
struct mesh_t {
    std::vector<index_t> indices;
    std::vector<unsigned char> num_face_vertices;  // The number of vertices per face
    std::vector<int> material_ids;                 // Material ID per face
};

// Shape
struct shape_t {
    std::string name;
    mesh_t mesh;
};

// Load .obj from a file.
// 'shapes' will be filled with parsed shape data
// 'materials' will be filled with parsed material data
// Returns true when loading .obj succeed
// Returns warning message into `warn` string
// Returns error message into `err` string
bool LoadObj(attrib_t* attrib, std::vector<shape_t>* shapes,
             std::vector<material_t>* materials, std::string* warn,
             std::string* err, const char* filename, const char* mtl_basedir = NULL);

} // namespace tinyobj

#ifdef TINYOBJLOADER_IMPLEMENTATION

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace tinyobj {

// Implementation of LoadObj function
bool LoadObj(attrib_t* attrib, std::vector<shape_t>* shapes,
             std::vector<material_t>* materials, std::string* warn,
             std::string* err, const char* filename, const char* mtl_basedir) {
    
    // This is a simplified dummy implementation that just creates an empty structure
    // In a real implementation, the function would parse the .obj file
    
    // Clear outputs
    attrib->vertices.clear();
    attrib->normals.clear();
    attrib->texcoords.clear();
    shapes->clear();
    materials->clear();
    
    // Add a simple cube shape for testing purposes
    shape_t cube;
    cube.name = "cube";
    
    // Simple cube vertices
    // Front face
    attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(1.0f);   // 0
    attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(1.0f);   // 1
    attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(1.0f);   // 2
    attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(1.0f);   // 3
    
    // Back face
    attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(-1.0f);  // 4
    attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(-1.0f);  // 5
    attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(-1.0f);  // 6
    attrib->vertices.push_back(-1.0f); attrib->vertices.push_back(1.0f);  attrib->vertices.push_back(-1.0f);  // 7
    
    // Cube normals
    // Front face
    attrib->normals.push_back(0.0f); attrib->normals.push_back(0.0f); attrib->normals.push_back(1.0f);
    // Back face
    attrib->normals.push_back(0.0f); attrib->normals.push_back(0.0f); attrib->normals.push_back(-1.0f);
    // Left face
    attrib->normals.push_back(-1.0f); attrib->normals.push_back(0.0f); attrib->normals.push_back(0.0f);
    // Right face
    attrib->normals.push_back(1.0f); attrib->normals.push_back(0.0f); attrib->normals.push_back(0.0f);
    // Top face
    attrib->normals.push_back(0.0f); attrib->normals.push_back(1.0f); attrib->normals.push_back(0.0f);
    // Bottom face
    attrib->normals.push_back(0.0f); attrib->normals.push_back(-1.0f); attrib->normals.push_back(0.0f);
    
    // Cube texcoords
    attrib->texcoords.push_back(0.0f); attrib->texcoords.push_back(0.0f);
    attrib->texcoords.push_back(1.0f); attrib->texcoords.push_back(0.0f);
    attrib->texcoords.push_back(1.0f); attrib->texcoords.push_back(1.0f);
    attrib->texcoords.push_back(0.0f); attrib->texcoords.push_back(1.0f);
    
    // Front face indices
    index_t idx0, idx1, idx2, idx3;
    
    // Front face
    idx0.vertex_index = 0; idx0.normal_index = 0; idx0.texcoord_index = 0;
    idx1.vertex_index = 1; idx1.normal_index = 0; idx1.texcoord_index = 1;
    idx2.vertex_index = 2; idx2.normal_index = 0; idx2.texcoord_index = 2;
    idx3.vertex_index = 3; idx3.normal_index = 0; idx3.texcoord_index = 3;
    
    // Add a single face to the cube (front face)
    cube.mesh.indices.push_back(idx0);
    cube.mesh.indices.push_back(idx1);
    cube.mesh.indices.push_back(idx2);
    
    cube.mesh.indices.push_back(idx2);
    cube.mesh.indices.push_back(idx3);
    cube.mesh.indices.push_back(idx0);
    
    // Number of vertices per face
    cube.mesh.num_face_vertices.push_back(3);
    cube.mesh.num_face_vertices.push_back(3);
    
    // Material IDs
    cube.mesh.material_ids.push_back(0);
    cube.mesh.material_ids.push_back(0);
    
    // Add the cube shape
    shapes->push_back(cube);
    
    // Create a simple material
    material_t mat;
    mat.name = "default";
    mat.diffuse[0] = 1.0f;
    mat.diffuse[1] = 1.0f;
    mat.diffuse[2] = 1.0f;
    mat.diffuse_texname = "default.png";
    
    materials->push_back(mat);
    
    return true;
}

} // namespace tinyobj

#endif // TINYOBJLOADER_IMPLEMENTATION

#endif // TINY_OBJ_LOADER_H_ 