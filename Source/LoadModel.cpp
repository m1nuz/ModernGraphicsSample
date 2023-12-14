#include "Common.hpp"
#include "Graphics.hpp"
#include "Hash.hpp"
#include "Log.hpp"
#include "Renderer.hpp"

#include <glad/gl.h>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include <tiny_gltf.h>

namespace Graphics {

auto processTextures(Device& device, const tinygltf::Model& model) -> std::vector<Texture> {
    std::vector<Texture> textures;
    textures.resize(std::size(model.textures));

    for (size_t i = 0; i < std::size(model.textures); i++) {
        const auto& image = model.images[model.textures[i].source];
        const auto& sampler = model.samplers[model.textures[i].sampler];

        [[maybe_unused]] bool generateMipMaps = true;
        [[maybe_unused]] TextureFiltering filtering { TextureFiltering::Trilinear };
        [[maybe_unused]] TextureWrap wrap { TextureWrap::None };
        if (model.textures[i].sampler != -1) {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) {
                filtering = TextureFiltering::Nearest;
                generateMipMaps = false;
            } else if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR) {
                generateMipMaps = false;
                filtering = TextureFiltering::Bilinear;
            } else if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR
                && sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR) {
                generateMipMaps = true;
                filtering = TextureFiltering::Trilinear;
            }

            if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
                wrap = TextureWrap::ClampToEdge;
            } else if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
                wrap = TextureWrap::MirroredRepeat;
            } else if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT) {
                wrap = TextureWrap::Repeat;
            }
        }

        [[maybe_unused]] Format pixelFormat { Format::Undefined };
        switch (image.component) {
        case 1:
            pixelFormat = Format::R8_UNORM;
            break;
        case 2:
            pixelFormat = Format::R8G8_UNORM;
            break;
        case 3:
            pixelFormat = Format::R8G8B8_UNORM;
            break;
        case 4:
            pixelFormat = Format::R8G8B8A8_UNORM;
            break;
        }

        textures[i] = createTexture2D(device,
            { .tag = make_hash(image.name),
                .width = static_cast<uint32_t>(image.width),
                .height = static_cast<uint32_t>(image.height),
                .format = pixelFormat,
                .mipLevels = 4,
                .generateMipMaps = generateMipMaps,
                .bindless = true,
                .filter = filtering,
                .wrap = wrap,
                .pixels = image.image });
    }

    return textures;
}

auto processMaterials(Device& device, const tinygltf::Model& model, std::span<const Texture> allTextures) -> std::vector<uint32_t> {

    std::vector<uint32_t> materials;
    for (size_t i = 0; i < std::size(model.materials); i++) {
        const auto& importedMaterial = model.materials[i];

        Material m;

        if (!importedMaterial.pbrMetallicRoughness.baseColorFactor.empty()) {
            m.pbrMetallicRoughness.baseColor
                = vec4 { importedMaterial.pbrMetallicRoughness.baseColorFactor[0], importedMaterial.pbrMetallicRoughness.baseColorFactor[1],
                      importedMaterial.pbrMetallicRoughness.baseColorFactor[2], importedMaterial.pbrMetallicRoughness.baseColorFactor[3] };
        }

        m.pbrMetallicRoughness.metallicFactor = importedMaterial.pbrMetallicRoughness.metallicFactor;
        m.pbrMetallicRoughness.roughnessFactor = importedMaterial.pbrMetallicRoughness.roughnessFactor;

        if (importedMaterial.pbrMetallicRoughness.baseColorTexture.index != -1) {
            m.pbrMetallicRoughness.baseColorTexture
                = findTextureHandleRef(device, allTextures[importedMaterial.pbrMetallicRoughness.baseColorTexture.index].handle);
        }

        if (importedMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
            m.pbrMetallicRoughness.metallicRoughnessTexture
                = findTextureHandleRef(device, allTextures[importedMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index].handle);
        }

        if (importedMaterial.normalTexture.index != -1) {
            m.normalTexture = findTextureHandleRef(device, allTextures[importedMaterial.normalTexture.index].handle);
        }

        if (importedMaterial.occlusionTexture.index != -1) {
            m.occlusionTexture = findTextureHandleRef(device, allTextures[importedMaterial.occlusionTexture.index].handle);
        }

        if (importedMaterial.emissiveTexture.index != -1) {
            m.emissiveTexture = findTextureHandleRef(device, allTextures[importedMaterial.emissiveTexture.index].handle);

            if (m.emissiveFactor == vec3 { 0.f }) {
                m.emissiveFactor = vec3 { 1.f };
                m.emissiveStrength = 1.f;
            }
        }

        materials.push_back(addMaterial(device, m));
    }

    return materials;
}

auto getNodeLocalTransformMatrix(const tinygltf::Node& node) -> mat4 {
    vec3 translation { 0.f };
    if (!node.translation.empty()) {
        translation = vec3 { static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
            static_cast<float>(node.translation[2]) };
    }

    vec3 scale { 1.f };
    if (!node.scale.empty()) {
        scale = vec3 { static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]) };
    }

    quat rotation;
    if (!node.rotation.empty()) {
        rotation = quat { static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]),
            static_cast<float>(node.rotation[3]) };
    }

    auto translationMatrix = glm::translate(mat4 { 1.f }, translation);
    auto rotationMatrix = glm::mat4_cast(rotation);
    auto scaleMatrix = glm::scale(mat4 { 1.f }, scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

auto convertVertexBufferFormat(const tinygltf::Model& model, const tinygltf::Primitive& primitive) -> std::vector<Vertex> {

    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> texcoords;

    for (const auto& [name, accessorIndex] : primitive.attributes) {
        const auto& accessor = model.accessors[accessorIndex];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        const size_t totalByteOffset = accessor.byteOffset + bufferView.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);

        if (name == "POSITION") {
            positions.resize(accessor.count);

            if (accessor.type == TINYGLTF_TYPE_VEC3) {
                for (size_t i = 0; i < accessor.count; i++) {
                    positions[i] = *reinterpret_cast<const vec3*>(std::data(buffer.data) + totalByteOffset + i * stride);
                }
            }
        } else if (name == "NORMAL") {
            normals.resize(accessor.count);

            if (accessor.type == TINYGLTF_TYPE_VEC3) {
                for (size_t i = 0; i < accessor.count; i++) {
                    normals[i] = *reinterpret_cast<const vec3*>(std::data(buffer.data) + totalByteOffset + i * stride);
                }
            }
        } else if (name == "TEXCOORD_0") {
            texcoords.resize(accessor.count);

            if (accessor.type == TINYGLTF_TYPE_VEC2) {
                for (size_t i = 0; i < accessor.count; i++) {
                    texcoords[i] = *reinterpret_cast<const vec2*>(std::data(buffer.data) + totalByteOffset + i * stride);
                }
            }
        }
    }

    std::vector<Vertex> vertices;
    vertices.resize(std::size(positions));

    for (size_t i = 0; i < std::size(positions); i++) {
        vertices[i].position = positions[i];
        vertices[i].normal = normals[i];
        vertices[i].uv = texcoords[i];
    }

    return vertices;
}

auto convertIndexBufferFormat(const tinygltf::Model& model, const tinygltf::Primitive& primitive) -> std::vector<uint32_t> {

    const int accessorIndex = primitive.indices;
    const auto& accessor = model.accessors[accessorIndex];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    const size_t totalByteOffset = accessor.byteOffset + bufferView.byteOffset;
    const auto stride = accessor.ByteStride(bufferView);

    std::vector<uint32_t> indices;
    indices.resize(accessor.count);

    if (accessor.componentType == GL_UNSIGNED_INT) {
        for (size_t i = 0; i < accessor.count; i++) {
            indices[i] = *reinterpret_cast<const uint32_t*>(std::data(buffer.data) + totalByteOffset + i * stride);
        }
    } else if (accessor.componentType == GL_UNSIGNED_SHORT) {
        for (size_t i = 0; i < accessor.count; i++) {
            indices[i] = *reinterpret_cast<const uint16_t*>(std::data(buffer.data) + totalByteOffset + i * stride);
        }
    }

    return indices;
}

auto getFaces(std::span<const uint32_t> indices) -> std::vector<uvec3> {
    std::vector<uvec3> faces;
    faces.resize(std::size(indices) / 3);
    for (size_t i = 0; i < std::size(faces); i++) {
        faces[i] = uvec3 { indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2] };
    }

    return faces;
}

struct MeshOptimizationConf {
    float overdrawThreshold { 1.05f };
    bool simplify { false };
    float simplifyThreshold { 0.2f };
    float targetError { 0.01f };
};

static auto optimizeMesh(const std::vector<Vertex>& meshVertices, const std::vector<uint32_t>& meshIndices,
    const MeshOptimizationConf& conf) -> std::tuple<std::vector<Vertex>, std::vector<uint32_t>> {

    const size_t numIndices = std::size(meshIndices);
    const size_t numVertices = std::size(meshVertices);

    std::vector<uint32_t> remap;
    remap.resize(numIndices);

    const size_t optVertexCount = meshopt_generateVertexRemap(
        std::data(remap), std::data(meshIndices), numIndices, std::data(meshVertices), numVertices, sizeof(Vertex));

    std::vector<uint32_t> optIndices;
    std::vector<Vertex> optVertices;

    optIndices.resize(numIndices);
    optVertices.resize(optVertexCount);
    optVertices = meshVertices;

    meshopt_remapIndexBuffer(std::data(optIndices), std::data(meshIndices), numIndices, std::data(remap));
    meshopt_remapVertexBuffer(std::data(optVertices), std::data(meshVertices), numVertices, sizeof(Vertex), std::data(remap));

    meshopt_optimizeVertexCache(std::data(optIndices), std::data(optIndices), numIndices, optVertexCount);

    meshopt_optimizeOverdraw(std::data(optIndices), std::data(optIndices), numIndices, &optVertices[0].position.x, optVertexCount,
        sizeof(Vertex), conf.overdrawThreshold);

    meshopt_optimizeVertexFetch(
        std::data(optVertices), std::data(optIndices), numIndices, std::data(optVertices), optVertexCount, sizeof(Vertex));

    if (!conf.simplify) {
        return { optVertices, optIndices };
    }

    size_t targetIndexCount = static_cast<size_t>(std::size(optIndices) * conf.simplifyThreshold);
    float resultError = 0.f;

    std::vector<uint32_t> simplifiedIndices;
    std::vector<Vertex> simplifiedVertices;

    simplifiedIndices.resize(std::size(optIndices));
    simplifiedIndices.resize(meshopt_simplify(&simplifiedIndices[0], &optIndices[0], std::size(optIndices), &optVertices[0].position.x,
        std::size(optVertices), sizeof(Vertex), targetIndexCount, conf.targetError, 0, &resultError));

    simplifiedVertices.resize(
        std::size(simplifiedIndices) < std::size(optVertices) ? std::size(simplifiedIndices) : std::size(optVertices));
    simplifiedVertices.resize(meshopt_optimizeVertexFetch(&simplifiedVertices[0], &simplifiedIndices[0], std::size(simplifiedIndices),
        &optVertices[0], std::size(optVertices), sizeof(Vertex)));

    LOG_DEBUG("{} triangles -> triangles {} ({} deviation) {}", std::size(optIndices) / 3, std::size(simplifiedIndices) / 3,
        resultError * 100, conf.simplifyThreshold);

    return { simplifiedVertices, simplifiedIndices };
}

static auto calculateTangentSpace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Get the vertices of the triangle
        vec3& v0 = vertices[indices[i]].position;
        vec3& v1 = vertices[indices[i + 1]].position;
        vec3& v2 = vertices[indices[i + 2]].position;

        // Get the texture coordinates of the triangle
        vec2& uv0 = vertices[indices[i]].uv;
        vec2& uv1 = vertices[indices[i + 1]].uv;
        vec2& uv2 = vertices[indices[i + 2]].uv;

        // Calculate the edges of the triangle
        vec3 deltaPos1 = v1 - v0;
        vec3 deltaPos2 = v2 - v0;

        // Calculate the UV differences
        vec2 deltaUV1 = uv1 - uv0;
        vec2 deltaUV2 = uv2 - uv0;

        // Calculate the tangent and bitangent vectors
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        vec3 tangent;
        tangent.x = f * (deltaUV2.y * deltaPos1.x - deltaUV1.y * deltaPos2.x);
        tangent.y = f * (deltaUV2.y * deltaPos1.y - deltaUV1.y * deltaPos2.y);
        tangent.z = f * (deltaUV2.y * deltaPos1.z - deltaUV1.y * deltaPos2.z);
        tangent = normalize(tangent);

        vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * deltaPos1.x + deltaUV1.x * deltaPos2.x);
        bitangent.y = f * (-deltaUV2.x * deltaPos1.y + deltaUV1.x * deltaPos2.y);
        bitangent.z = f * (-deltaUV2.x * deltaPos1.z + deltaUV1.x * deltaPos2.z);
        bitangent = normalize(bitangent);

        // Update the vertices with tangent and bitangent information
        vertices[indices[i]].tangent += tangent;
        vertices[indices[i + 1]].tangent += tangent;
        vertices[indices[i + 2]].tangent += tangent;

        vertices[indices[i]].tangent += bitangent;
        vertices[indices[i + 1]].tangent += bitangent;
        vertices[indices[i + 2]].tangent += bitangent;
    }

    // Normalize the tangent vectors for each vertex
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].tangent = normalize(vertices[i].tangent);
    }
}

static auto processScene(Device& device, const tinygltf::Model& importedModel, [[maybe_unused]] std::span<const uint32_t> allMaterials,
    [[maybe_unused]] size_t sceneIndex) -> Model {

    Model model;
    for (size_t i = 0; i < std::size(importedModel.meshes); i++) {
        auto& importedMesh = importedModel.meshes[i];
        for (const auto& primitive : importedMesh.primitives) {
            auto vertices = convertVertexBufferFormat(importedModel, primitive);
            auto indices = convertIndexBufferFormat(importedModel, primitive);

            calculateTangentSpace(vertices, indices);

            Mesh m;

            bool simplify = false;
            float targetError = 0.01f;
            std::array<float, MaxMeshLODs> thresholds { 0.7, 0.5, 0.2, 0.01 };

            for (size_t j = 0; j < MaxMeshLODs; j++) {
                if (j == MaxMeshLODs - 1) {
                    targetError = 0.01f;
                }

                auto [optVertices, optIndices] = optimizeMesh(
                    vertices, indices, { .simplify = simplify, .simplifyThreshold = thresholds[j], .targetError = targetError });
                m.LODs[j].vertices = optVertices;
                m.LODs[j].faces = getFaces(optIndices);

                simplify = true;

                LOG_DEBUG("LOD{} triangles {}", j, std::size(optIndices) / 3);
            }

            Model::SubMesh mesh;
            mesh.meshRef = addMesh(device, m);
            mesh.materialRef = allMaterials[i];
            model.meshes.push_back(mesh);
        }
    }

    return model;
}

auto loadModel(Device& device, std::string_view filepath) -> void {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = false;
    auto ext = getFilePathExt(filepath);
    if (ext == ".glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, std::string { filepath });
    } else if (ext == ".gltf") {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, std::string { filepath });
    }

    if (!ret || !err.empty() || !warn.empty()) {
        LOG_ERROR("{}: {} {} {}", filepath, ret, err, warn);
    }

    auto textures = processTextures(device, model);
    auto materials = processMaterials(device, model, textures);
    auto sceneModel = processScene(device, model, materials, model.defaultScene);
    sceneModel.tag = make_hash(filepath);

    device.models_.push_back(sceneModel);
}

} // namespace Graphics