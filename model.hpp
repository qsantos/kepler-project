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
    Mesh2(const vector<Vertex>& vertices, const vector<unsigned int>& indices, glm::vec3 color);
    void draw(void);

    unsigned int vbo;
    unsigned int ibo;
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    glm::vec3 color;
};

struct Model {
    void load(const char* path);
    void draw(void);

private:
    void do_node(aiNode* node, const aiScene* scene);
    Mesh2 do_mesh(aiMesh *mesh, const aiScene *scene);

    vector<Mesh2> meshes;
};

#endif
