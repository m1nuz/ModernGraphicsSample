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

struct Object {
    mat4 transform { 1.f };
    uint32_t meshRef { 0xffffffff };
    uint32_t materialRef { 0xffffffff };
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
    std::vector<Framebuffer> framebuffers_;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Material> materials_;
    std::vector<MeshProperty> meshProperties_;
    std::vector<Light> lights_;
    std::vector<uint64_t> textureHandles_;

    std::vector<mat4> modelMatrices_;
    std::vector<Drawable> drawables_;

    uint32_t meshVertexArray { 0 };

    bool culling { true };
    int32_t visibleInstances { 0 };
    int32_t drawInstances { 0 };

    bool reloadMeshBuffers_ { true };
    bool reloadMaterialBuffers_ { true };
    bool reloadLightBuffers_ { true };

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
    size_t numObjects { 0 };
};

auto initialize(Device& device, const DeviceConfiguration& conf) -> bool;
auto cleanup(Device& device) -> void;
auto present(Device& device, Camera& camera, std::span<const Object> objects) -> void;
auto resize(Device& device, const ivec2& framebufferSize) -> void;

} // namespace Graphics