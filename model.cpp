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

std::vector<Material> loadedMaterials;
std::vector<Mesh> loadedMeshes;

void Mesh::Init(std::vector<Vertex> p_vertices,
                std::vector<unsigned int> p_indices,
                Material *p_material) 
{
    this->vertices = p_vertices;
    this->indices = p_indices;
    this->material = p_material;

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
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
            (void*) (offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*) (offsetof(Vertex, texCoords)));
    glEnableVertexAttribArray(2);
}


void Mesh::Draw(ShaderProg shader)
{
    glActiveTexture(GL_TEXTURE0);
    
    // TODO: support more than one material
    if (material != NULL) {
        shader.SetInt((char*)"material.texture_diffuse", GL_TEXTURE0);
        unsigned int texId = material->texture.id;
        
        if (texId == 0) {
            texId = gDefaultTexture.id;
        }
        glBindTexture(GL_TEXTURE_2D, texId);
        shader.SetVec3((char*)"material.diffuseColour",
                        glm::value_ptr(material->diffuseColour));
        shader.SetFloat((char*)"material.shininess", material->shininess);
    }
    else {
        // Don't draw with any texture
        SDL_Log("Null material");
        shader.SetInt((char*)"material.texture_diffuse", GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gDefaultTexture.id);
        glm::vec3 col = glm::vec3(1.0f);
        shader.SetVec3((char*)"material.diffuseColour", glm::value_ptr(col));
    }

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


void ModelNode::Draw(ShaderProg shader, Mesh* meshes, glm::mat4 transform)
{
    glm::mat4 newTrans = transform * mTransform;
    newTrans = ToGlmMat4(ToJoltMat4(newTrans));
    shader.SetMat4fv((char*)"model", glm::value_ptr(newTrans));
    for (size_t i = 0; i < mMeshes.size(); i++) {
        meshes[mMeshes[i]].Draw(shader);
    }
}


void ModelNode::Draw(ShaderProg shader, Mesh* meshes)
{
    Draw(shader, meshes, glm::mat4(1.0f));
}


void Model::Draw(ShaderProg shader, glm::mat4 transform)
{
    for (unsigned int i = 0; i < nodes.size(); i++) {
        nodes[i].Draw(shader, &meshes[0], transform);
    }
}


void Model::Draw(ShaderProg shader)
{
    for (unsigned int i = 0; i < nodes.size(); i++) {
        nodes[i].Draw(shader, &meshes[0]);
    }
}


void Model::LoadSceneMaterials(const aiScene *scene)
{
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *aiMat = scene->mMaterials[i];
        //std::vector<Texture> diffuseMaps = LoadMaterialTextures(
        //        material, aiTextureType_DIFFUSE, "texture_diffuse");
        //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        Material newMat;
        // Prevent garbage values
        newMat.texture.id = 0;
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

            char filepath[256];
            SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
            SDL_Log("Loading texture %s", filepath);
            Texture texture = CreateTextureFromFile(filepath);
            newMat.texture = texture;
        }
        aiColor3D colour;
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
        newMat.diffuseColour.x = colour.r;
        newMat.diffuseColour.y = colour.g;
        newMat.diffuseColour.z = colour.b;

        float shininess;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, shininess) != aiReturn_SUCCESS) {
            SDL_Log("Failed to get roughness Factor");
            shininess = 32.0f; // Set to a reasonable default
        }
        else {
            shininess = 16.0f / shininess;
        }
        SDL_Log("roughness = %f", shininess);
        SDL_Log("shininess = %f", shininess);
        newMat.shininess = shininess;

        materials.push_back(newMat);
    }
}


void Model::LoadSceneMeshes(const aiScene *scene)
{
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        ProcessMesh(scene->mMeshes[i]);
    }
}


void Model::ProcessNode(aiNode *node, aiMatrix4x4 accTransform, node_callback_t callback) 
{
    /* Recursively add all the meshes from this node to the Model */

    // Get global transform of node
    aiMatrix4x4 transform;
    transform = node->mTransformation * accTransform;

    //glm::mat4 glmTrans = ToGlmMat4(transform);
    if (callback != NULL && !callback(node, transform)) {
        // Don't process this node
        return;
    }
    
    aiQuaternion rotation;
    aiVector3D position;
    aiVector3D scale;
    //transform.DecomposeNoScaling(rotation, position);
    transform.Decompose(scale, rotation, position);



    SDL_Log("Process Node %s", node->mName.C_Str());
    SDL_Log("Scale: %f, %f, %f", scale.x, scale.y, scale.z);
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
    
    ModelNode modelNode;
    modelNode.mTransform = ToGlmMat4(transform);

    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // index in Model mesh array should be same as index in aiScene's 
        // mesh array
        modelNode.mMeshes.push_back(node->mMeshes[i]);
    }

    nodes.push_back(modelNode);
    
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], transform, callback);
    }
}


void Model::ProcessMesh(aiMesh *mesh)
{
    /* Convert an aiMesh object to a Mesh object */
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material *meshMaterial = NULL;

    // Process all vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vec;

        // mVertices holds vertex positions
        vec.x = mesh->mVertices[i].x;
        vec.y = mesh->mVertices[i].y;
        vec.z = mesh->mVertices[i].z;
        vertex.position = vec;

        vec.x = mesh->mNormals[i].x;
        vec.y = mesh->mNormals[i].y;
        vec.z = mesh->mNormals[i].z;
        vertex.normal = vec;

        
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

    // Process all materials (todo)
    
        /*
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        //std::vector<Texture> diffuseMaps = LoadMaterialTextures(
        //        material, aiTextureType_DIFFUSE, "texture_diffuse");
        //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        Material newMat;
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &str);

            char filepath[256];
            SDL_snprintf(filepath, 256, "models/%s", str.C_Str());
            SDL_Log("Loading texture %s", filepath);
            Texture texture = CreateTextureFromFile(filepath);
            newMat.texture = texture;
        }
        aiColor3D colour;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
        newMat.diffuseColour.x = colour.r;
        newMat.diffuseColour.y = colour.g;
        newMat.diffuseColour.z = colour.b;
        */
        // If this assertion fails, the material has not been loaded into the
        // materials vector. LoadSceneMaterials must be called before this
        // method.
        //meshMaterials.push_back(materials[mesh->mMaterialIndex]);

    
    // NOTE: There will always be at least one material
    SDL_assert(materials.size() > mesh->mMaterialIndex);
    meshMaterial = &materials[mesh->mMaterialIndex];
    

    Mesh returnMesh;
    returnMesh.Init(vertices, indices, meshMaterial);
    meshes.push_back(returnMesh);
}


Model LoadModel(std::string path, node_callback_t nodeCallback)
{
    Model model;
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        SDL_Log("Assimp Error: %s", importer.GetErrorString());
        return model;
    }

    model.LoadSceneMaterials(scene);
    model.LoadSceneMeshes(scene);

    // Global transform of root, which is identity matrix
    aiMatrix4x4 transform;
    model.ProcessNode(scene->mRootNode, transform, nodeCallback);
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
