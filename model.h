#pragma once

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include "shader.h"
#include "texture.h"

using node_callback_t = bool (*)(aiNode *node, aiMatrix4x4 transform);

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};


struct Material
{
    Texture texture;
    glm::vec3 diffuseColour;
    float shininess;
};


struct Mesh {
    // NOTE: The vertices and indices may not need to be stored in the struct,
    // as they are stored on the GPU after initialisation.
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // Pointer to material held in Mesh struct
    Material *material;

    void Init(std::vector<Vertex> p_vertices,
              std::vector<unsigned int> p_indices,
              Material *material);

    void Draw(ShaderProg shader);
    unsigned int vao, vbo, ebo;
};


struct Model {
    void Draw(ShaderProg shader);
    void LoadSceneMaterials(const aiScene *scene);
    void LoadSceneMeshes(const aiScene *scene);
    void ProcessNode(aiNode *node, aiMatrix4x4 accTransform, node_callback_t callback = NULL);
    void ProcessMesh(aiMesh *mesh);
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};

Model LoadModel(std::string path, node_callback_t nodeCallback=NULL);

std::vector<Texture> LoadMaterialTextures(aiMaterial *mat,
                                          aiTextureType type,
                                          std::string typeName);
