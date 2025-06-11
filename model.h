#pragma once

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include "shader.h"
#include "texture.h"

#include <memory>

using node_callback_t = bool (*)(const aiNode *node, aiMatrix4x4 transform);
using light_callback_t = void (*)(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};


struct Material
{
    Material();
    ~Material();
    void Destroy();
    Texture texture;
    Texture normalMap;
    Texture roughnessMap;
    glm::vec3 diffuseColour;
    float roughness;
    float metallic;
};

struct Mesh {
    // NOTE: The vertices and indices may not need to be stored in the struct,
    // as they are stored on the GPU after initialisation.
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Index of material in Model's materials vector
    unsigned int materialIdx;
    unsigned int vao, vbo, ebo;

    void Init(std::vector<Vertex> aVertices,
              std::vector<unsigned int> aIndices,
              unsigned int aMaterialIdx);


    void Draw(ShaderProg shader, const std::vector<std::unique_ptr<Material>> &materials) const;

    Mesh();
    ~Mesh();
};

// Forward declaration to allow Model and ModelNode to refer to each other
//struct Model;

struct ModelNode {
    ~ModelNode();
    ModelNode();
    void Draw(ShaderProg shader, const std::vector<std::unique_ptr<Mesh>> &meshes, const std::vector<std::unique_ptr<Material>> &materials) const;
    void Draw(ShaderProg shader, const std::vector<std::unique_ptr<Mesh>> &meshes, const std::vector<std::unique_ptr<Material>> &materials, glm::mat4 transform) const;

    // Array of indices pointing to location of meshes in the Model's meshes
    // array
    std::vector<int> mMeshes;
    glm::mat4 mTransform;
};


struct Model {
    ~Model();
    void Draw(ShaderProg shader) const;
    void Draw(ShaderProg shader, glm::mat4 transform) const;

    void LoadSceneMaterials(const aiScene *scene);
    void LoadSceneMeshes(const aiScene *scene);
    void ProcessNode(const aiNode *node, aiMatrix4x4 accTransform, const aiScene *scene, node_callback_t NodeCallback = NULL, light_callback_t LightCallback = NULL);
    void ProcessMesh(aiMesh *mesh);
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<ModelNode>> nodes;
};


Model* LoadModel(std::string path, node_callback_t NodeCallback=NULL, light_callback_t LightCallback=NULL);

std::vector<Texture> LoadMaterialTextures(aiMaterial *mat,
                                          aiTextureType type,
                                          std::string typeName);
