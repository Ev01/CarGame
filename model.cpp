#include "model.h"
#include "texture.h"
#include "shader.h"
#include "convert.h"
#include "glad/glad.h"

#include <SDL3/SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>

#include <memory>


void Mesh::Init(std::vector<Vertex> aVertices,
                std::vector<unsigned int> aIndices,
                unsigned int aMaterialIdx)
{
    vertices = aVertices;
    indices = aIndices;
    materialIdx = aMaterialIdx;


    // Initialise all opengl vertex array stuff

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0],
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
            (void*) (offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*) (offsetof(Vertex, texCoords)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*) (offsetof(Vertex, tangent)));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*) (offsetof(Vertex, bitangent)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
}



Mesh::~Mesh()
{
    //SDL_Log("Deleting Mesh");
    
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
    }
}

Mesh::Mesh()
{
    //SDL_Log("Creating Mesh");
}

Model::~Model()
{
    //SDL_Log("Deleting Model");
    // Unload all textures
    for (size_t i = 0; i < materials.size(); i++) {
        materials[i]->Destroy();
    }
}


Material::~Material()
{
    //glDeleteTextures(1, &(texture.id));
    //SDL_Log("Deleting Material");
}

Material::Material()
{
    //SDL_Log("Creating Material");
}

void Material::Destroy()
{
    texture.Destroy();
    normalMap.Destroy();
    roughnessMap.Destroy();
}


ModelNode::~ModelNode()
{
    //SDL_Log("Deleting Node");
}

ModelNode::ModelNode()
{
    //SDL_Log("Creating Node");
}


void Mesh::Draw(ShaderProg shader, const std::vector<std::unique_ptr<Material>> &materials) const
{
    glm::vec3 diffuseColour = glm::vec3(1.0f);
    float roughness = 1.0;

    const Material &material = *materials[materialIdx];
    
    unsigned int texId = material.texture.id;
    unsigned int normalMapId = material.normalMap.id;
    unsigned int roughnessMapId = material.roughnessMap.id;
    diffuseColour = material.diffuseColour;
    roughness = material.roughness;

    texId = texId == 0 ? gDefaultTexture.id : texId;
    normalMapId = normalMapId == 0 ? gDefaultNormalMap.id : normalMapId;
    roughnessMapId = roughnessMapId == 0 ? gDefaultTexture.id : roughnessMapId;

    /*
    if (normalMapId != 0) {
        SDL_Log("Normal map id: %d", normalMapId);
    }
    */

    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, roughnessMapId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMapId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    shader.SetInt((char*)"material.texture_diffuse", 0);
    shader.SetInt((char*)"material.normalMap", 1);
    shader.SetInt((char*)"material.roughnessMap", 2);

    shader.SetVec3((char*)"material.baseColour",
                    glm::value_ptr(diffuseColour));
    shader.SetFloat((char*)"material.roughness", roughness);
    shader.SetFloat((char*)"material.metallic", material.metallic);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


void ModelNode::Draw(ShaderProg shader, const std::vector<std::unique_ptr<Mesh>> &meshes, const std::vector<std::unique_ptr<Material>> &materials, glm::mat4 transform) const
{
    glm::mat4 newTrans = transform * mTransform;
    newTrans = ToGlmMat4(ToJoltMat4(newTrans));
    shader.SetMat4fv((char*)"model", glm::value_ptr(newTrans));

    for (size_t i = 0; i < mMeshes.size(); i++) {
        meshes[mMeshes[i]]->Draw(shader, materials);
    }
}


void ModelNode::Draw(ShaderProg shader, const std::vector<std::unique_ptr<Mesh>> &meshes, const std::vector<std::unique_ptr<Material>> &materials) const
{
    Draw(shader, meshes, materials, glm::mat4(1.0f));
}


void Model::Draw(ShaderProg shader, glm::mat4 transform) const
{
    for (unsigned int i = 0; i < nodes.size(); i++) {
        nodes[i]->Draw(shader, meshes, materials, transform);
    }
}


void Model::Draw(ShaderProg shader) const
{
    for (unsigned int i = 0; i < nodes.size(); i++) {
        nodes[i]->Draw(shader, meshes, materials);
    }
}


void Model::LoadSceneMaterials(const aiScene *scene)
{
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *aiMat = scene->mMaterials[i];
        //std::vector<Texture> diffuseMaps = LoadMaterialTextures(
        //        material, aiTextureType_DIFFUSE, "texture_diffuse");
        //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::unique_ptr<Material> newMat = std::make_unique<Material>();
        // Prevent garbage values
        newMat->texture.id = 0;
        newMat->normalMap.id = 0;

        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

            char filepath[256];
            SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
            SDL_Log("Loading texture %s", filepath);
            Texture texture = CreateTextureFromFile(filepath);
            //Texture texture = gDefaultTexture;
            newMat->texture = texture;
        }
        if (aiMat->GetTextureCount(aiTextureType_NORMALS) > 0) {
            aiString str;
            aiMat->GetTexture(aiTextureType_NORMALS, 0, &str);

            char filepath[256];
            SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
            SDL_Log("Loading texture %s", filepath);
            Texture normalMap = CreateTextureFromFile(filepath, false);
            //Texture normalMap = gDefaultNormalMap;
            newMat->normalMap = normalMap;
        }
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0) {
            aiString str;
            aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &str);

            char filepath[256];
            SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
            SDL_Log("Loading texture %s", filepath);
            Texture roughnessMap = CreateTextureFromFile(filepath, false);
            //Texture roughnessMap = gDefaultTexture;
            newMat->roughnessMap = roughnessMap;
        }

        aiColor3D colour;
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
        newMat->diffuseColour.x = colour.r;
        newMat->diffuseColour.y = colour.g;
        newMat->diffuseColour.z = colour.b;

        float roughness;
        float metallic;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS) {
            SDL_Log("Couldn't get roughness");
            roughness = 1.0;
        }
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != aiReturn_SUCCESS) {
            SDL_Log("Couldn't get metallic");
            metallic = 0.0;
        }
        newMat->roughness = roughness;
        newMat->metallic = metallic;

        materials.push_back(std::move(newMat));
    }
}


void Model::LoadSceneMeshes(const aiScene *scene)
{
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        ProcessMesh(scene->mMeshes[i]);
    }
}


void Model::ProcessNode(const aiNode *node, aiMatrix4x4 accTransform, const aiScene *scene, node_callback_t NodeCallback, light_callback_t LightCallback) 
{
    /* Recursively add all the meshes from this node to the Model */

    // Get global transform of node
    aiMatrix4x4 transform;
    transform = node->mTransformation * accTransform;

    //glm::mat4 glmTrans = ToGlmMat4(transform);
    if (NodeCallback != NULL && !NodeCallback(node, transform)) {
        // Don't process this node
        return;
    }

    // Check if this node is a light source
    if (LightCallback != NULL) {
        for (unsigned int i = 0; i < scene->mNumLights; i++) {
            aiLight *light = scene->mLights[i];
            if (light->mName == node->mName) {
                LightCallback(light, node, transform);
            }
        }
    }

    
    aiQuaternion rotation;
    aiVector3D position;
    aiVector3D scale;
    //transform.DecomposeNoScaling(rotation, position);
    transform.Decompose(scale, rotation, position);



    //SDL_Log("Process Node %s", node->mName.C_Str());
    /*
    if (node->mMetaData != nullptr) {
        SDL_Log("Has metadata");
        SDL_Log("Num properties: %d", node->mMetaData->mNumProperties);
        aiString key("Wheel");
        aiString value;
        if (node->mMetaData->Get(key, value)) {
            SDL_Log("Wheel: %s", value.C_Str());
        }

    }
    */
    
    std::unique_ptr<ModelNode> modelNode = std::make_unique<ModelNode>();

    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // index in Model mesh array should be same as index in aiScene's 
        // mesh array
        modelNode->mMeshes.push_back(node->mMeshes[i]);
    }
    
    // Only add the model node if it contains meshes
    if (modelNode->mMeshes.size() > 0) {
        modelNode->mTransform = ToGlmMat4(transform);
        nodes.push_back(std::move(modelNode));
    }
    
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], transform, scene, NodeCallback, LightCallback);
    }
}


void Model::ProcessMesh(aiMesh *mesh)
{
    /* Convert an aiMesh object to a Mesh object */
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    //Material *meshMaterial = NULL;
    
    //SDL_Log("Mesh vertices size: %d", 
    //        mesh->mNumVertices);

    // Process all vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vec;

        // mesh->mVertices holds vertex positions
        vec.x = mesh->mVertices[i].x;
        vec.y = mesh->mVertices[i].y;
        vec.z = mesh->mVertices[i].z;
        vertex.position = vec;

        vec.x = mesh->mNormals[i].x;
        vec.y = mesh->mNormals[i].y;
        vec.z = mesh->mNormals[i].z;
        vertex.normal = vec;

        vec.x = mesh->mTangents[i].x;
        vec.y = mesh->mTangents[i].y;
        vec.z = mesh->mTangents[i].z;
        vertex.tangent = vec;

        vec.x = mesh->mBitangents[i].x;
        vec.y = mesh->mBitangents[i].y;
        vec.z = mesh->mBitangents[i].z;
        vertex.bitangent = vec;

        
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }
        
        vertices.push_back(vertex);
    }

    // Process all indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // Assimp stores all indices inside aiFace objects. We know each face
        // has 3 indices (the mesh is triangulated) so we can just add the 
        // indices without worrying about what face they belong to.
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // NOTE: There will always be at least one material
    SDL_assert(materials.size() > mesh->mMaterialIndex);
    
    std::unique_ptr<Mesh> returnMesh = std::make_unique<Mesh>();
    returnMesh->Init(vertices, indices, mesh->mMaterialIndex);
    meshes.push_back(std::move(returnMesh));
}


Model* LoadModel(std::string path, node_callback_t NodeCallback, light_callback_t LightCallback)
{
    //Assimp::Logger *logger = Assimp::DefaultLogger::create(
    //        ASSIMP_DEFAULT_LOG_NAME, Assimp::Logger::NORMAL, 
    //        aiDefaultLogStream_STDOUT, nullptr);


    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        SDL_Log("Assimp Error: %s", importer.GetErrorString());
        return nullptr;
    }
    /*
    if (!(scene->mFlags & AI_SCENE_FLAGS_VALIDATED)) {
        SDL_Log("Assimp Error, scene is not validated: %s", importer.GetErrorString());
        return model;
    }
    */
    Model *model = new Model;

    model->LoadSceneMaterials(scene);
    model->LoadSceneMeshes(scene);

    SDL_Log("Num lights: %d", scene->mNumLights);
    if (scene->mNumLights > 0) {
        aiString lightName = scene->mLights[0]->mName;
        aiNode *lightNode = scene->mRootNode->FindNode(lightName);

        aiQuaternion rotation;
        aiVector3D position;
        lightNode->mTransformation.DecomposeNoScaling(rotation, position);

        /*
        float range;
        if (lightNode->mMetaData->Get("PBR_LightRange", range)) {
            SDL_Log("Range: %f", range);
        } else {
            SDL_Log("Couldn't get range");
        }
        */

        aiColor3D col = scene->mLights[0]->mColorDiffuse;
        /*
        SDL_Log("Colour: %f, %f, %f", col.r, col.b, col.g);
        SDL_Log("Name: %s", lightName.C_Str());
        SDL_Log("Pos x: %f", position.x);
        SDL_Log("Constant: %f", scene->mLights[0]->mAttenuationConstant);
        SDL_Log("Quadratic: %f", scene->mLights[0]->mAttenuationQuadratic);
        SDL_Log("Linear: %f", scene->mLights[0]->mAttenuationLinear);
        */
    }

    // Global transform of root, which is identity matrix
    aiMatrix4x4 transform;
    model->ProcessNode(scene->mRootNode, transform, scene, NodeCallback, LightCallback);
    return model;
}


std::vector<Texture> LoadMaterialTextures(aiMaterial *mat,
                                          aiTextureType type,
                                          std::string typeName) 
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        char filepath[256];
        SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
        Texture texture = CreateTextureFromFile(filepath);

        textures.push_back(texture);
    }

    return textures;
}
