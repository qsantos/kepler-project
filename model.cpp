#include "model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <GL/glew.h>

Mesh2::Mesh2(const vector<Vertex>& vertices_, const vector<unsigned int>& indices_, glm::vec3 color_) :
    vertices{vertices_},
    indices{indices_},
    color{color_}
{
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), this->vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &this->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), this->indices.data(), GL_STATIC_DRAW);
}

void set_color(float red, float green, float blue, float alpha=1.f);

void Mesh2::draw(void) {
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);

    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    set_color(this->color.x, this->color.y, this->color.z, 1.);

    GLint var = glGetAttribLocation(program, "v_position");
    glEnableVertexAttribArray(var);
    glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    var = glGetAttribLocation(program, "v_normal");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    }

    var = glGetAttribLocation(program, "v_texcoord");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
    }

    /*
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    */

    //glDrawElements(GL_TRIANGLES, (GLsizei) indices.size(), GL_UNSIGNED_INT, 0);
    glDrawRangeElements(GL_TRIANGLES, 0, (GLsizei) this->vertices.size() - 1, (GLsizei) indices.size(), GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Mesh2 Model::do_mesh(aiMesh *mesh, const aiScene *scene) {
    (void) scene;

    vector<Vertex> vertices;
    vector<unsigned int> indices;

    for (size_t i = 0; i < mesh->mNumVertices; i += 1) {
        Vertex vertex;

        vertex.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z,
        };

        vertex.normal = {
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z,
        };

        // texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texcoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y,
            };
        } else {
            vertex.texcoords = {0.0f, 0.0f};
        }

        vertex.tangent = {
            mesh->mTangents[i].x,
            mesh->mTangents[i].y,
            mesh->mTangents[i].z,
        };

        vertex.bitangent = {
            mesh->mBitangents[i].x,
            mesh->mBitangents[i].y,
            mesh->mBitangents[i].z,
        };

        vertices.push_back(vertex);
    }

    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (size_t i = 0; i < mesh->mNumFaces; i += 1) {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j += 1) {
            indices.push_back(face.mIndices[j]);
        }
    }

    auto material = scene->mMaterials[mesh->mMaterialIndex];

    aiColor3D diffuse;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    glm::vec3 color = {diffuse.r, diffuse.g, diffuse.b};

    // return a mesh object created from the extracted mesh data
    return Mesh2(vertices, indices, color);
}

void Model::do_node(aiNode* node, const aiScene* scene) {
    for (size_t i = 0; i < node->mNumMeshes; i += 1) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(this->do_mesh(mesh, scene));
    }

    for (size_t i = 0; i < node->mNumChildren; i += 1) {
        this->do_node(node->mChildren[i], scene);
    }
}


void Model::load(const char* path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (scene == NULL || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == NULL) {
        fprintf(stderr, "Failed to load model %s: %s\n", path, importer.GetErrorString());
        exit(EXIT_FAILURE);
    }

    this->do_node(scene->mRootNode, scene);
}

void Model::draw(void) {
    for (size_t i = 0; i < this->meshes.size(); i += 1) {
        this->meshes[i].draw();
    }
}
