#pragma once

#include "Graphics.hpp"

typedef struct GLFWwindow GLFWwindow;

namespace Graphics {

using Window = GLFWwindow;

struct Camera {
    float distance = 10.0f;
    float theta = 0.0f;
    float phi = 0.3f;

    float fieldOfView = 90.f;
    float nearPlane = 0.1f;
    float farPlane = 1000.f;

    vec3 position() const {
        return vec3 { distance * glm::cos(phi) * glm::cos(theta), distance * glm::sin(phi), distance * glm::cos(phi) * glm::sin(theta) };
    }

    mat4 view() const {
        return glm::lookAt(position(), vec3 { 0.f }, vec3 { 0.f, 1.f, 0.f });
    }

    mat4 projection(float aspectRation) const {
        return glm::perspective(glm::radians(fieldOfView), aspectRation, nearPlane, farPlane);
    }
};
struct Entity {
    mat4 transform { 1.f };
    uint32_t modelRef { 0xffffffff };
};

struct Drawable {
    uint32_t materialRef { 0xffffffff };
    uint32_t meshRef { 0xffffffff };
};

struct DebugOutputParams {
    bool showNotifications = false;
    bool showPerformance = false;
};

struct Device {
    std::vector<Texture> textures_;
    std::vector<Shader> shaders_;
    std::vector<Pipeline> pipelines_;
    std::vector<Buffer> buffers_;
    std::vector<Renderbuffer> renderbuffers_;
    std::vector<Framebuffer> framebuffers_;

    std::vector<Model> models_;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Material> materials_;
    std::vector<uint64_t> textureHandles_;
    std::vector<MeshProperty> meshProperties_;
    std::vector<Light> lights_;

    std::vector<mat4> modelMatrices_;
    std::vector<Drawable> drawables_;

    uint32_t meshVertexArray { 0 };
    uint32_t fullscreenQuadVertexArray { 0 };

    float gamma { 2.2f };
    float exposure { 1.f };
    bool culling { true };
    bool useBindlessTextures { true };
    int32_t visibleInstances { 0 };
    int32_t drawInstances { 0 };

    bool reloadMeshBuffers_ { true };
    bool reloadMaterialBuffers_ { true };
    bool reloadLightBuffers_ { true };

    bool buildedEnvCubemap { false };
    bool buildedIrradianceCubemap { false };
    bool buildPrefilterCubemap { false };
    bool buildBRDFLUTTexture { false };

    DebugOutputParams debugOutputParams_;
};

struct DeviceConfiguration {
    Window* window { nullptr };

    size_t numTextures { 0 };
    size_t numShaders { 0 };
    size_t numPipelines { 0 };
    size_t numBuffers { 0 };
    size_t numFramebuffers { 0 };

    size_t numVertices { 0 };
    size_t numIndices { 0 };

    size_t numMaterials { 0 };
    size_t numMeshes { 0 };
    size_t numLights { 0 };
    size_t numModels { 0 };
    size_t numEntities { 0 };
};

auto initialize(Device& device, const DeviceConfiguration& conf) -> bool;
auto cleanup(Device& device) -> void;
auto present(Device& device, Camera& camera, std::span<const Entity> entities) -> void;
auto resize(Device& device, const ivec2& framebufferSize) -> void;

} // namespace Graphics