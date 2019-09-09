#ifndef MODEL_HPP
#define MODEL_HPP

#include <vector>

#include <assimp/scene.h>
#include <glm/glm.hpp>

using std::vector;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct Mesh2 {
    Mesh2(const vector<Vertex>& vertices, const vector<unsigned int>& indices, unsigned int diffuse_map);
    void draw(void);

private:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    unsigned int diffuse_map;
    unsigned int vbo;
    unsigned int ibo;
};

struct Model {
    void load(const std::string& path);
    void draw(void);

private:
    void do_node(aiNode* node, const aiScene* scene);
    Mesh2 do_mesh(aiMesh *mesh, const aiScene *scene);
    unsigned int load_material_texture(aiMaterial* mat, aiTextureType type);

    vector<Mesh2> meshes;
    std::string base_path;
};

#endif
