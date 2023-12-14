#pragma once

#include "Math.hpp"

#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <vector>

namespace Graphics {

enum class Format {
    Undefined,
    R8_UNORM,
    R8G8_UNORM,
    R8G8B8_UNORM,
    R8G8B8A8_UNORM,

    R16_FLOAT,
    R16G16_FLOAT,
    R16G16B16_FLOAT,
    R16G16B16A16_FLOAT,

    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32_FLOAT,
    R32G32B32A32_FLOAT,

    D32_FLOAT,
    D32_UNORM,
    D24_UNORM,
    D16_UNORM,
};

enum class ShaderStage : uint32_t {
    Unknown,
    Vertex,
    TessControl,
    TessEvaluation,
    Geometry,
    Fragment,
    Compute,
};

enum class TextureFiltering {
    None,
    Nearest,
    Bilinear,
    Trilinear,
    Anisotropic,
};

enum class TextureWrap {
    None,
    ClampToEdge,
    ClampToBorder,
    MirroredRepeat,
    Repeat,
    MirrorClampToEdge,
};

struct Vertex {
    vec3 position { 0.f };
    vec3 normal { 0.f };
    vec2 uv { 0.f };
    vec3 tangent { 0.f };
};

struct BoundingSphere {
    vec3 position { 0.f };
    float radius { 0.f };
};

constexpr size_t MaxMeshLODs = 4;

struct MeshLOD {
    std::vector<Vertex> vertices;
    std::vector<uvec3> faces;
};

struct Mesh {
    mat4 local { 1.f };
    uint32_t materialRef { 0xffffffff };
    std::array<MeshLOD, MaxMeshLODs> LODs;
};

struct Model {
    uint64_t tag { 0 };

    struct SubMesh {
        uint32_t parent { 0 };
        mat4 local { 1.f };
        uint32_t materialRef { 0xffffffff };
        uint32_t meshRef { 0xffffffff };
    };

    std::vector<SubMesh> meshes;
};

struct MeshLODProperty {
    uint32_t baseVertex { 0 };
    uint32_t baseIndex { 0 };
    uint32_t indexCount { 0 };
    uint32_t padding { 0 };
};

struct MeshProperty {
    std::array<MeshLODProperty, MaxMeshLODs> LODs;
    BoundingSphere bSphere;
};

struct PBRMetallicRoughnessMaterial {
    vec4 baseColor { 1.f };
    float metallicFactor { 1.f };
    float roughnessFactor { 1.f };
    uint32_t baseColorTexture { 0xffffffff };
    uint32_t metallicRoughnessTexture { 0xffffffff };
};

struct Material {
    PBRMetallicRoughnessMaterial pbrMetallicRoughness {};
    uint32_t normalTexture { 0xffffffff };
    uint32_t occlusionTexture { 0xffffffff };
    uint32_t emissiveTexture { 0xffffffff };
    float padding { 0 };
    vec3 emissiveFactor { 0.f };
    float emissiveStrength { 0.f };
};

struct Light {
    vec3 position { 0.f };
    float intensity { 0.f };
    vec3 color { 0.f };
    float radius { 0.f };
};

struct Pipeline {
    auto is_valid() const noexcept -> bool {
        return id != 0;
    }

    operator bool() const {
        return is_valid();
    }

    uint64_t tag = 0;
    uint32_t id = 0;
};

struct Shader {
    auto is_valid() const noexcept -> bool {
        return id != 0;
    }

    operator bool() const {
        return is_valid();
    }

    uint64_t tag { 0 };
    uint32_t id { 0 };
    ShaderStage stage { ShaderStage::Unknown };
};

struct Texture {
    auto is_valid() const noexcept -> bool {
        return id != 0;
    }

    operator bool() const {
        return is_valid();
    }

    uint64_t tag = 0;
    uint32_t id = 0;
    uint32_t target = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    uint32_t mipLevels = 0;
    uint64_t handle = 0;
};

struct Buffer {
    auto is_valid() const noexcept -> bool {
        return id != 0;
    }

    operator bool() const {
        return is_valid();
    }

    uint64_t tag = 0;
    uint32_t id = 0;
    uint32_t size = 0;
};

struct Renderbuffer {
    uint64_t tag = 0;
    uint32_t id = 0;
    uint32_t target = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 0;
};

struct Framebuffer {
    auto is_valid() const noexcept -> bool {
        return id != 0;
    }

    auto is_complete() const noexcept -> bool;

    operator bool() const {
        return is_valid() && is_complete();
    }

    uint64_t tag = 0;
    uint32_t id = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mask = 0;
    uint32_t status = 0;
    uint32_t numDrawBuffers = 1;
};

struct TextureConfiguration {
    uint64_t tag;
    uint32_t width { 0 };
    uint32_t height { 0 };
    Format format { Format::Undefined };
    uint32_t samples { 0 };
    uint32_t mipLevels { 0 };
    bool generateMipMaps { false };
    bool bindless { false };
    TextureFiltering filter = TextureFiltering::Nearest;
    TextureWrap wrap = TextureWrap::None;
    std::span<const uint8_t> pixels {};
};

struct TextureCubeConfiguration {
    uint64_t tag;
    uint32_t width { 0 };
    uint32_t height { 0 };
    uint32_t depth { 0 };
    Format format { Format::Undefined };
    uint32_t samples { 0 };
    uint32_t mipLevels { 0 };
    bool generateMipMaps { false };
    bool bindless { false };
    TextureFiltering filter = TextureFiltering::Nearest;
    TextureWrap wrap = TextureWrap::None;
    std::span<std::span<const uint8_t>> pixels {};
};

struct ShaderConfiguration {
    uint64_t tag;
    ShaderStage stage { 0 };
    std::string filename {};
    std::string source;
};

struct PipelineConfiguration {
    uint64_t tag;
    std::span<const Shader> stages;
};

struct BufferConfiguration {
    uint64_t tag;
    std::span<const uint8_t> data {};
    size_t emptySize = 0;
};

struct RenderBufferConfiguration {
    uint64_t tag = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    Format format = Format::Undefined;
    uint32_t samples = 0;
};

struct FramebufferAttachment {
    uint32_t attachment = 0;
    uint32_t attachmentTarget = 0;
    uint32_t renderTarget = 0;
};

struct FramebufferConfiguration {
    uint64_t tag = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mask = 0;

    std::span<const FramebufferAttachment> attachments = {};
    std::span<const uint32_t> drawBuffers {};
    std::optional<uint32_t> drawBuffer {};
    std::optional<uint32_t> readBuffer {};
};

struct CreateMeshConfiguration {
    std::optional<bool> cube { std::nullopt };
    std::optional<uint32_t> sphere { std::nullopt };
};

struct CreateMaterialConfiguration {
    vec3 Kd { 0.f };
    vec3 Ks { 0.f };
    float Ns { 0.f };
    float d { 0.f };
    std::string_view KdMapName {};
};

struct PointLightConfiguration {
    vec3 position { 0.f };
    vec3 color { 0.f };
    float intensity { 0.f };
    float radius { 0.f };
};

struct DirectionalLightConfiguration {
    vec3 direction { 0.f };
    vec3 color { 0.f };
    float intensity { 0.f };
};

struct Device;

auto createTexture2D(Device& device, const TextureConfiguration& conf) -> Texture;
auto createTextureCube(Device& device, const TextureCubeConfiguration& info) -> Texture;
auto createShader(Device& device, const ShaderConfiguration& conf) -> Shader;
auto createGraphicsPipeline(Device& device, const PipelineConfiguration& conf) -> Pipeline;
auto createBuffer(Device& device, const BufferConfiguration& conf) -> Buffer;
auto createRenderbuffer(Device& device, const RenderBufferConfiguration& conf) -> Renderbuffer;
auto createFramebuffer(Device& device, const FramebufferConfiguration& conf) -> Framebuffer;

auto loadShader(Device& device, std::string_view filepath) -> void;
auto loadPipeline(Device& device, uint64_t tag, std::span<const std::string_view> shaderNames) -> void;
auto loadTexture(Device& device, std::string_view filepath) -> void;
auto loadModel(Device& device, std::string_view filepath) -> void;

auto addMesh(Device& device, const Mesh& mesh) -> uint32_t;
auto addMaterial(Device& device, const Material& material) -> uint32_t;
auto addLight(Device& device, const Light& light) -> uint32_t;
auto addDirectionalLight(Device& device, const DirectionalLightConfiguration& conf) -> uint32_t;

auto createMesh(Device& device, const CreateMeshConfiguration& conf) -> uint32_t;
auto createMaterial(Device& device, const CreateMaterialConfiguration& conf) -> uint32_t;

auto findShader(Device& device, uint64_t tag) -> Shader;
auto findPipeline(Device& device, uint64_t tag) -> Pipeline;
auto findTexture(Device& device, uint64_t tag) -> Texture;
auto findBuffer(Device& device, uint64_t tag) -> Buffer;
auto findTextureHandleRef(Device& device, uint64_t handle) -> uint32_t;
auto findModelRef(Device& device, uint64_t tag) -> uint32_t;

auto drawQuad(Device& device) -> void;
auto drawCube(Device& device) -> void;

} // namespace Graphics