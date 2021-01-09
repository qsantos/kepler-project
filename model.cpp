#include "model.hpp"

extern "C" {
#include "texture.h"
#include "logging.h"
}

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <GL/glew.h>

Mesh2::Mesh2(const vector<Vertex>& vertices_, const vector<unsigned int>& indices_, GLuint diffuse_map_) :
    vertices{vertices_},
    indices{indices_},
    diffuse_map{diffuse_map_}
{
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), this->vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &this->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), this->indices.data(), GL_STATIC_DRAW);
}

void Mesh2::draw(void) {
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);

    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

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

    var = glGetAttribLocation(program, "v_tangent");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    }

    var = glGetAttribLocation(program, "v_bitangent");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    }

    glBindTexture(GL_TEXTURE_2D, this->diffuse_map);
    glDrawRangeElements(GL_TRIANGLES, 0, (GLsizei) this->vertices.size() - 1, (GLsizei) indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint Model::load_material_texture(aiMaterial *mat, aiTextureType type) {
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString filename;
        mat->GetTexture(type, i, &filename);

        std::string path = this->base_path + "/" + filename.C_Str();
        return load_texture(path.c_str());
    }
    return 0;
}

Mesh2 Model::do_mesh(aiMesh *mesh, const aiScene *scene) {
    (void) scene;

    vector<Vertex> vertices;
    vector<unsigned int> indices;

    // vertices
    for (size_t i = 0; i < mesh->mNumVertices; i += 1) {
        Vertex vertex;

        // position
        if (mesh->HasPositions()) {
            vertex.position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z,
            };
        }

        // normals
        if (mesh->HasNormals()) {
            vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z,
            };
        }

        // texcoords
        if (mesh->HasTextureCoords(0)) {
            vertex.texcoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y,
            };
        }

        // tangent and bitangent
        if (mesh->HasTangentsAndBitangents()) {
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
        }

        vertices.push_back(vertex);
    }

    // indices
    for (size_t i = 0; i < mesh->mNumFaces; i += 1) {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j += 1) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // material
    auto material = scene->mMaterials[mesh->mMaterialIndex];

    GLuint diffuse_map = this->load_material_texture(material, aiTextureType_DIFFUSE);

    // return a mesh object created from the extracted mesh data
    return Mesh2(vertices, indices, diffuse_map);
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


void Model::load(const std::string& path) {
    DEBUG("Model %s loading", path.c_str());
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (scene == NULL || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == NULL) {
        CRITICAL("Failed to load model %s: %s", path.c_str(), importer.GetErrorString());
        exit(EXIT_FAILURE);
    }

    std::string s(path);
    this->base_path = s.substr(0, s.find_last_of('/'));

    this->do_node(scene->mRootNode, scene);
    DEBUG("Model %s loaded", path.c_str());
}

void Model::draw(void) {
    for (size_t i = 0; i < this->meshes.size(); i += 1) {
        this->meshes[i].draw();
    }
}
