#include "Graphics.hpp"
#include "Common.hpp"
#include "Hash.hpp"
#include "Log.hpp"
#include "Renderer.hpp"

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <cassert>
#include <fstream>

namespace Graphics {

inline auto getBoundingSphere(const MeshLOD& mesh) -> BoundingSphere {
    if (mesh.vertices.empty()) {
        return {};
    }

    vec3 minimum = mesh.vertices[0].position;
    vec3 maximum = mesh.vertices[0].position;

    for (const auto& vertex : mesh.vertices) {
        minimum.x = std::min(minimum.x, vertex.position.x);
        minimum.y = std::min(minimum.y, vertex.position.y);
        minimum.z = std::min(minimum.z, vertex.position.z);

        maximum.x = std::max(maximum.x, vertex.position.x);
        maximum.y = std::max(maximum.y, vertex.position.y);
        maximum.z = std::max(maximum.z, vertex.position.z);
    }

    vec3 center = (minimum + maximum) * 0.5f;

    float radius = 0.0f;
    for (const auto& vertex : mesh.vertices) {
        radius = std::max(radius, length(vertex.position - center));
    }

    return { center, radius };
}

inline auto internalFormat(Format format) -> GLint {
    switch (format) {
    case Format::Undefined:
        return 0;
    case Format::R8_UNORM:
        return GL_R8;
    case Format::R8G8_UNORM:
        return GL_RG8;
    case Format::R8G8B8_UNORM:
        return GL_RGB8;
    case Format::R8G8B8A8_UNORM:
        return GL_RGBA8;

    case Format::R16_FLOAT:
        return GL_R16F;
    case Format::R16G16_FLOAT:
        return GL_RG16F;
    case Format::R16G16B16_FLOAT:
        return GL_RGB16F;
    case Format::R16G16B16A16_FLOAT:
        return GL_RGBA16F;

    case Format::R32_FLOAT:
        return GL_R32F;
    case Format::R32G32_FLOAT:
        return GL_RG32F;
    case Format::R32G32B32_FLOAT:
        return GL_RGB32F;
    case Format::R32G32B32A32_FLOAT:
        return GL_RGBA32F;

    case Format::D32_FLOAT:
        return GL_DEPTH_COMPONENT32F;
    case Format::D32_UNORM:
        return GL_DEPTH_COMPONENT32;
    case Format::D24_UNORM:
        return GL_DEPTH_COMPONENT24;
    case Format::D16_UNORM:
        return GL_DEPTH_COMPONENT16;
    }

    return 0;
}

inline auto imageFormat(Format format) -> std::tuple<uint32_t, uint32_t> {
    switch (format) {
    case Format::Undefined:
        return { 0, 0 };
    case Format::R8_UNORM:
        return { GL_RED, GL_UNSIGNED_BYTE };
    case Format::R8G8_UNORM:
        return { GL_RG, GL_UNSIGNED_BYTE };
    case Format::R8G8B8_UNORM:
        return { GL_RGB, GL_UNSIGNED_BYTE };
    case Format::R8G8B8A8_UNORM:
        return { GL_RGBA, GL_UNSIGNED_BYTE };

    case Format::R16_FLOAT:
        return { GL_RED, GL_FLOAT };
    case Format::R16G16_FLOAT:
        return { GL_RG, GL_FLOAT };
    case Format::R16G16B16_FLOAT:
        return { GL_RGB, GL_FLOAT };
    case Format::R16G16B16A16_FLOAT:
        return { GL_RGBA, GL_FLOAT };

    case Format::R32_FLOAT:
        return { GL_RED, GL_FLOAT };
    case Format::R32G32_FLOAT:
        return { GL_RG, GL_FLOAT };
    case Format::R32G32B32_FLOAT:
        return { GL_RGB, GL_FLOAT };
    case Format::R32G32B32A32_FLOAT:
        return { GL_RGBA, GL_FLOAT };

    case Format::D32_FLOAT:
        return { GL_DEPTH_COMPONENT, GL_FLOAT };
    case Format::D32_UNORM:
        return { GL_DEPTH_COMPONENT, GL_FLOAT };
    case Format::D24_UNORM:
        return { GL_DEPTH_COMPONENT, GL_FLOAT };
    case Format::D16_UNORM:
        return { GL_DEPTH_COMPONENT, GL_FLOAT };
    }

    return {};
}

inline auto shaderStage(ShaderStage stage) -> GLenum {
    switch (stage) {
    case ShaderStage::Unknown:
        return GL_NONE;
    case ShaderStage::Vertex:
        return GL_VERTEX_SHADER;
    case ShaderStage::Fragment:
        return GL_FRAGMENT_SHADER;
    case ShaderStage::Geometry:
        return GL_GEOMETRY_SHADER;
    case ShaderStage::TessControl:
        return GL_TESS_CONTROL_SHADER;
    case ShaderStage::TessEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    case ShaderStage::Compute:
        return GL_COMPUTE_SHADER;
    }

    return GL_NONE;
}

inline auto applyTextureFitering(uint32_t id, const TextureFiltering filtering, const int32_t levels) {
    int32_t maxLevels = levels;

    switch (filtering) {
    case TextureFiltering::None:
        break;
    case TextureFiltering::Nearest:
        glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, maxLevels);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case TextureFiltering::Bilinear:
        glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, maxLevels);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case TextureFiltering::Trilinear:
        glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, maxLevels);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    case TextureFiltering::Anisotropic:
        glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, maxLevels);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
        break;
    default:
        break;
    }
}

inline auto applyTextureWrap(uint32_t id, TextureWrap wrapping, bool wrapR = false) {
    switch (wrapping) {
    case TextureWrap::None:
        break;
    case TextureWrap::ClampToEdge:
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (wrapR) {
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        break;
    case TextureWrap::ClampToBorder:
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        if (wrapR) {
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        }
        break;
    case TextureWrap::MirroredRepeat:
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        if (wrapR) {
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
        }
        break;
    case TextureWrap::Repeat:
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (wrapR) {
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_REPEAT);
        }
        break;
    case TextureWrap::MirrorClampToEdge:
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE);
        if (wrapR) {
            glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_MIRROR_CLAMP_TO_EDGE);
        }
        break;
    default:
        break;
    }
}

auto findShader(Device& device, uint64_t tag) -> Shader {
    for (const auto& s : device.shaders_) {
        if (s.tag == tag) {
            return s;
        }
    }

    return {};
}

auto findPipeline(Device& device, uint64_t tag) -> Pipeline {
    for (const auto& p : device.pipelines_) {
        if (p.tag == tag) {
            return p;
        }
    }

    return {};
}

auto findTexture(Device& device, uint64_t tag) -> Texture {
    for (const auto& t : device.textures_) {
        if (t.tag == tag) {
            return t;
        }
    }

    return {};
}

auto findBuffer(Device& device, uint64_t tag) -> Buffer {
    for (const auto& b : device.buffers_) {
        if (b.tag == tag) {
            return b;
        }
    }

    return {};
}

auto findTextureHandleRef(Device& device, uint64_t handle) -> uint32_t {
    uint32_t idx = 0;
    for (const auto& h : device.textureHandles_) {
        if (handle == h) {
            return idx;
        }

        idx++;
    }

    return idx;
}

auto findModelRef(Device& device, uint64_t tag) -> uint32_t {
    uint32_t idx = 0;
    for (const auto& m : device.models_) {
        if (m.tag == tag) {
            return idx;
        }
        idx++;
    }

    return idx;
}

auto loadPipeline(Device& device, uint64_t tag, std::span<const std::string_view> shaderNames) -> void {

    if (auto pipeline = findPipeline(device, tag); pipeline) {
        return;
    }

    std::vector<Shader> shaders;

    for (auto shaderName : shaderNames) {
        if (auto shader = findShader(device, make_hash(shaderName)); shader) {
            shaders.push_back(shader);
        } else {
            loadShader(device, shaderName);
            return;
        }
    }

    createGraphicsPipeline(device, { .tag = tag, .stages = shaders });
}

auto Framebuffer::is_complete() const noexcept -> bool {
    return status == GL_FRAMEBUFFER_COMPLETE;
}

auto createTexture2D(Device& device, const TextureConfiguration& conf) -> Texture {
    uint32_t id = 0u;
    glCreateTextures(GL_TEXTURE_2D, 1, &id);

    const uint32_t mipLevels = conf.mipLevels != 0 ? conf.mipLevels : std::max(1.0, std::log2(std::max(conf.width, conf.height)));

    applyTextureFitering(id, conf.filter, mipLevels);
    applyTextureWrap(id, conf.wrap, false);

    if (conf.samples > 1) {
        glTextureStorage2DMultisample(id, conf.samples, internalFormat(conf.format), conf.width, conf.height, true);
    } else {
        glTextureStorage2D(id, mipLevels, internalFormat(conf.format), conf.width, conf.height);
    }

    if (!conf.pixels.empty()) {
        auto [format, type] = imageFormat(conf.format);
        glTextureSubImage2D(id, 0, 0, 0, conf.width, conf.height, format, type, std::data(conf.pixels));
    }

    if (conf.generateMipMaps) {
        glGenerateTextureMipmap(id);
    }

    uint64_t handle = 0;
    if (conf.bindless && device.useBindlessTextures) {
        handle = glGetTextureHandleARB(id);
        glMakeTextureHandleResidentARB(handle);

        device.textureHandles_.push_back(handle);
    }

    return device.textures_.emplace_back(conf.tag, id, GL_TEXTURE_2D, conf.width, conf.height, 0, mipLevels, handle);
}

auto createTextureCube(Device& device, const TextureCubeConfiguration& conf) -> Texture {
    uint32_t id = 0u;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

    const uint32_t mipLevels = conf.mipLevels != 0 ? conf.mipLevels : std::max(1.0, std::log2(std::max(conf.width, conf.height)));

    applyTextureFitering(id, conf.filter, mipLevels);
    applyTextureWrap(id, conf.wrap, true);

    glTextureStorage2D(id, mipLevels, internalFormat(conf.format), conf.width, conf.height);
    for (size_t face = 0; face < conf.depth; ++face) {
        auto [format, type] = imageFormat(conf.format);
        const auto pixels
            = conf.pixels.empty() ? nullptr : (conf.pixels[face].empty() ? nullptr : reinterpret_cast<const void*>(&conf.pixels[face][0]));
        glTextureSubImage3D(id, 0, 0, 0, face, conf.width, conf.height, 1, format, type, pixels);
    }

    if (conf.generateMipMaps) {
        glGenerateTextureMipmap(id);
    }

    return device.textures_.emplace_back(conf.tag, id, GL_TEXTURE_2D, conf.width, conf.height, 0, mipLevels, 0);
}

auto createShader(Device& device, const ShaderConfiguration& conf) -> Shader {
    const char* sources[] = { std::data(conf.source), nullptr };

    const auto id = glCreateShaderProgramv(shaderStage(conf.stage), 1, sources);

    GLint lenght = 0;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &lenght);

    std::string programLog;
    programLog.resize(static_cast<std::string::size_type>(lenght));

    GLsizei written = 0;
    glGetProgramInfoLog(id, lenght, &written, std::data(programLog));
    if (written > 0) {
        LOG_ERROR("Failed to compile({}): {}", conf.filename, programLog);
        exit(EXIT_FAILURE);
    }

    return device.shaders_.emplace_back(conf.tag, id, conf.stage);
}

auto createGraphicsPipeline(Device& device, const PipelineConfiguration& conf) -> Pipeline {
    auto pipelineID = 0u;
    glCreateProgramPipelines(1, &pipelineID);

    for (const auto& shader : conf.stages) {
        switch (shader.stage) {
        case ShaderStage::Unknown:
            break;
        case ShaderStage::Vertex:
            glUseProgramStages(pipelineID, GL_VERTEX_SHADER_BIT, shader.id);
            break;
        case ShaderStage::TessControl:
            glUseProgramStages(pipelineID, GL_TESS_CONTROL_SHADER_BIT, shader.id);
            break;
        case ShaderStage::TessEvaluation:
            glUseProgramStages(pipelineID, GL_TESS_EVALUATION_SHADER_BIT, shader.id);
            break;
        case ShaderStage::Geometry:
            glUseProgramStages(pipelineID, GL_GEOMETRY_SHADER_BIT, shader.id);
            break;
        case ShaderStage::Fragment:
            glUseProgramStages(pipelineID, GL_FRAGMENT_SHADER_BIT, shader.id);
            break;
        case ShaderStage::Compute:
            glUseProgramStages(pipelineID, GL_COMPUTE_SHADER_BIT, shader.id);
            break;
        }
    }

    return device.pipelines_.emplace_back(conf.tag, pipelineID);
}

auto createComputePipeline(Device& device, const PipelineConfiguration& conf) -> Pipeline {
    return createGraphicsPipeline(device, conf);
}

auto createBuffer(Device& device, const BufferConfiguration& conf) -> Buffer {
    auto id = 0u;
    glCreateBuffers(1, &id);

    if (!conf.data.empty()) {
        glNamedBufferData(id, std::size(conf.data), std::data(conf.data), GL_DYNAMIC_DRAW);
    } else if (conf.emptySize != 0) {
        glNamedBufferData(id, conf.emptySize, nullptr, GL_DYNAMIC_DRAW);
    }

    return device.buffers_.emplace_back(conf.tag, id, conf.data.empty() ? conf.emptySize : std::size(conf.data));
}

auto createRenderbuffer(Device& device, const RenderBufferConfiguration& conf) -> Renderbuffer {
    auto id = 0u;
    glCreateRenderbuffers(1, &id);

    auto iFormat = internalFormat(conf.format);

    if (conf.samples == 0) {
        glNamedRenderbufferStorage(id, static_cast<GLenum>(iFormat), static_cast<GLsizei>(conf.width), static_cast<GLsizei>(conf.height));
    } else {
        glNamedRenderbufferStorageMultisample(id, static_cast<GLsizei>(conf.samples), static_cast<GLenum>(iFormat),
            static_cast<GLsizei>(conf.width), static_cast<GLsizei>(conf.height));
    }

    return device.renderbuffers_.emplace_back(conf.tag, id, GL_RENDERBUFFER, conf.width, conf.height, conf.samples);
}

auto createFramebuffer(Device& device, const FramebufferConfiguration& conf) -> Framebuffer {
    auto id = 0u;
    glCreateFramebuffers(1, &id);

    uint32_t mask = 0;

    for (const auto& attachment : conf.attachments) {
        switch (attachment.attachment) {
        case GL_COLOR_ATTACHMENT0:
        case GL_COLOR_ATTACHMENT1:
        case GL_COLOR_ATTACHMENT2:
        case GL_COLOR_ATTACHMENT3:
        case GL_COLOR_ATTACHMENT4:
        case GL_COLOR_ATTACHMENT5:
        case GL_COLOR_ATTACHMENT6:
        case GL_COLOR_ATTACHMENT7:
        case GL_COLOR_ATTACHMENT8:
        case GL_COLOR_ATTACHMENT9:
        case GL_COLOR_ATTACHMENT10:
        case GL_COLOR_ATTACHMENT11:
        case GL_COLOR_ATTACHMENT12:
        case GL_COLOR_ATTACHMENT13:
        case GL_COLOR_ATTACHMENT14:
        case GL_COLOR_ATTACHMENT15:
        case GL_COLOR_ATTACHMENT16:
        case GL_COLOR_ATTACHMENT17:
        case GL_COLOR_ATTACHMENT18:
        case GL_COLOR_ATTACHMENT19:
        case GL_COLOR_ATTACHMENT20:
        case GL_COLOR_ATTACHMENT21:
        case GL_COLOR_ATTACHMENT22:
        case GL_COLOR_ATTACHMENT23:
        case GL_COLOR_ATTACHMENT24:
        case GL_COLOR_ATTACHMENT25:
        case GL_COLOR_ATTACHMENT26:
        case GL_COLOR_ATTACHMENT27:
        case GL_COLOR_ATTACHMENT28:
        case GL_COLOR_ATTACHMENT29:
        case GL_COLOR_ATTACHMENT30:
        case GL_COLOR_ATTACHMENT31:
            mask |= GL_COLOR_BUFFER_BIT;
            break;
        case GL_DEPTH_ATTACHMENT:
            mask |= GL_DEPTH_BUFFER_BIT;
            break;
        }

        switch (attachment.attachmentTarget) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            glNamedFramebufferTexture(id, attachment.attachment, attachment.renderTarget, 0);
            break;
        case GL_RENDERBUFFER:
            glNamedFramebufferRenderbuffer(id, attachment.attachment, GL_RENDERBUFFER, attachment.renderTarget);
            break;
        }
    }

    const auto status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);

    if (!conf.drawBuffers.empty()) {
        glNamedFramebufferDrawBuffers(id, std::size(conf.drawBuffers), std::data(conf.drawBuffers));
    }

    uint32_t numDrawBuffers = conf.drawBuffers.empty() ? 1 : std::size(conf.drawBuffers);

    if (conf.drawBuffer) {
        glNamedFramebufferDrawBuffer(id, *conf.drawBuffer);
    }

    if (conf.readBuffer) {
        glNamedFramebufferReadBuffer(id, *conf.readBuffer);
    }

    return device.framebuffers_.emplace_back(conf.tag, id, conf.width, conf.height, mask, status, numDrawBuffers);
}

static auto getShaderStage(std::string_view name) -> ShaderStage {
    if (name.find(".vert") != std::string_view::npos) {
        return ShaderStage::Vertex;
    } else if (name.find(".tesc") != std::string_view::npos) {
        return ShaderStage::TessControl;
    } else if (name.find(".tese") != std::string_view::npos) {
        return ShaderStage::TessEvaluation;
    } else if (name.find(".geom") != std::string_view::npos) {
        return ShaderStage::Geometry;
    } else if (name.find(".frag") != std::string_view::npos) {
        return ShaderStage::Fragment;
    } else if (name.find(".comp") != std::string_view::npos) {
        return ShaderStage::Compute;
    }

    return ShaderStage::Unknown;
}

auto loadShader(Device& device, std::string_view filepath) -> void {
    using namespace std;

    ifstream fs(filepath.data(), ios::in | ios::binary);
    if (!fs.is_open()) {
        LOG_ERROR("load shader '{}'", filepath);
        return;
    }

    std::string buf;
    fs.seekg(0, ios::end);
    const auto sz = fs.tellg();
    buf.resize(static_cast<size_t>(sz));
    fs.seekg(0, ios::beg);

    fs.read(reinterpret_cast<char*>(std::data(buf)), static_cast<std::streamsize>(std::size(buf)));
    fs.close();

    createShader(
        device, { .tag = make_hash(filepath), .stage = getShaderStage(filepath), .filename = std::string { filepath }, .source = buf });
}

auto addMesh(Device& device, const Mesh& mesh) -> uint32_t {

    device.reloadMeshBuffers_ = true;

    MeshProperty meshProperty;

    size_t idx = 0;
    for (const auto& lod : mesh.LODs) {
        uint32_t elementCount = std::size(lod.faces) * 3;

        meshProperty.LODs[idx].baseVertex = std::size(device.vertices_);
        meshProperty.LODs[idx].baseIndex = std::size(device.indices_);
        meshProperty.LODs[idx].indexCount = elementCount;

        if (elementCount == 0) {
            continue;
        }

        device.vertices_.insert(std::end(device.vertices_), std::begin(lod.vertices), std::end(lod.vertices));

        for (const auto& face : lod.faces) {
            device.indices_.push_back(face.x);
            device.indices_.push_back(face.y);
            device.indices_.push_back(face.z);
        }

        idx++;
    }

    meshProperty.bSphere = getBoundingSphere(mesh.LODs[0]);

    device.meshProperties_.push_back(meshProperty);

    return std::size(device.meshProperties_) - 1;
}

auto addMaterial(Device& device, const Material& material) -> uint32_t {
    device.reloadMaterialBuffers_ = true;

    device.materials_.push_back(material);

    return std::size(device.materials_) - 1;
}

auto addLight(Device& device, const Light& light) -> uint32_t {
    device.reloadLightBuffers_ = true;

    device.lights_.push_back(light);

    return std::size(device.lights_) - 1;
}

auto addDirectionalLight(Device& device, const DirectionalLightConfiguration& conf) -> uint32_t {
    device.reloadLightBuffers_ = true;

    Light light;
    light.position = conf.direction;
    light.color = conf.color;
    light.radius = 0.f;
    light.intensity = conf.intensity;

    device.lights_.push_back(light);

    return std::size(device.lights_) - 1;
}

auto drawQuad(Device& device) -> void {
    glBindVertexArray(device.fullscreenQuadVertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

auto drawCube([[maybe_unused]] Device& device) -> void {
    static uint32_t cubeVAO = 0;
    static uint32_t cubeVBO = 0;

    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// static auto createCube() -> MeshLOD {
//     MeshLOD mesh;

//     mesh.vertices = {
//         // +x
//         { { +0.5f, -0.5f, -0.5f }, { +1, 0, 0 }, { 1, 1 } },
//         { { +0.5f, +0.5f, -0.5f }, { +1, 0, 0 }, { 0, 1 } },
//         { { +0.5f, +0.5f, +0.5f }, { +1, 0, 0 }, { 0, 0 } },
//         { { +0.5f, -0.5f, +0.5f }, { +1, 0, 0 }, { 1, 0 } },

//         // -x
//         { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 1, 1 } },
//         { { -0.5f, +0.5f, -0.5f }, { -1, 0, 0 }, { 0, 1 } },
//         { { -0.5f, +0.5f, +0.5f }, { -1, 0, 0 }, { 0, 0 } },
//         { { -0.5f, -0.5f, +0.5f }, { -1, 0, 0 }, { 1, 0 } },

//         // +y

//         { { -0.5f, +0.5f, -0.5f }, { 0, +1, 0 }, { 1, 1 } },
//         { { +0.5f, +0.5f, -0.5f }, { 0, +1, 0 }, { 0, 1 } },
//         { { +0.5f, +0.5f, +0.5f }, { 0, +1, 0 }, { 0, 0 } },
//         { { -0.5f, +0.5f, +0.5f }, { 0, +1, 0 }, { 1, 0 } },

//         // -y
//         { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 1 } },
//         { { +0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 0, 1 } },
//         { { +0.5f, -0.5f, +0.5f }, { 0, -1, 0 }, { 0, 0 } },
//         { { -0.5f, -0.5f, +0.5f }, { 0, -1, 0 }, { 1, 0 } },

//         // +z
//         { { -0.5f, -0.5f, +0.5f }, { 0, 0, +1 }, { 1, 1 } },
//         { { +0.5f, -0.5f, +0.5f }, { 0, 0, +1 }, { 0, 1 } },
//         { { +0.5f, +0.5f, +0.5f }, { 0, 0, +1 }, { 0, 0 } },
//         { { -0.5f, +0.5f, +0.5f }, { 0, 0, +1 }, { 1, 0 } },

//         // -z
//         { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 1 } },
//         { { +0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1 } },
//         { { +0.5f, +0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
//         { { -0.5f, +0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0 } },

//     };

//     mesh.faces = {
//         // +x
//         { 0, 1, 2 },
//         { 0, 2, 3 },

//         // -x
//         { 4, 6, 5 },
//         { 4, 7, 6 },

//         // +y
//         { 8, 10, 9 },
//         { 8, 11, 10 },

//         // -y
//         { 12, 13, 14 },
//         { 12, 14, 15 },

//         // +z
//         { 16, 17, 18 },
//         { 16, 18, 19 },

//         // -z
//         { 20, 22, 21 },
//         { 20, 23, 22 },
//     };

//     return mesh;
// }

// static auto createSphere(uint32_t n) -> MeshLOD {
//     MeshLOD mesh;

//     uint32_t sectors = pow(2, n);
//     uint32_t slices = pow(2, n);

//     for (uint j = 0; j <= slices; j++) {
//         for (uint i = 0; i <= sectors; i++) {
//             float phi = glm::pi<float>() * (-0.5f + float(j) / slices);
//             float theta = glm::pi<float>() * 2 * float(i) / sectors;

//             float factor = 0.75f;

//             float x = cos(phi) * cos(-theta) * factor;
//             float y = sin(phi) * factor;
//             float z = cos(phi) * sin(-theta) * factor;

//             float u = float(i) / sectors;
//             float v = 1 - float(j) / slices;

//             mesh.vertices.push_back({ { x, y, z }, { x, y, z }, { u, v } });

//             if (i < sectors && j < slices) {
//                 uint32_t index_A = (j + 0) * (sectors + 1) + (i + 0);
//                 uint32_t index_B = (j + 0) * (sectors + 1) + (i + 1);
//                 uint32_t index_C = (j + 1) * (sectors + 1) + (i + 1);
//                 uint32_t index_D = (j + 1) * (sectors + 1) + (i + 0);

//                 mesh.faces.push_back({ index_A, index_B, index_C });
//                 mesh.faces.push_back({ index_A, index_C, index_D });
//             }
//         }
//     }

//     return mesh;
// }

// static auto createMesh(const CreateMeshConfiguration& conf) -> Mesh {
//     if (conf.cube) {
//         Mesh mesh;
//         mesh.LODs[0] = createCube();

//         return mesh;
//     } else if (conf.sphere) {
//         Mesh sphere;

//         for (size_t i = 0; i < MaxMeshLODs; i++) {
//             sphere.LODs[i] = createSphere(*conf.sphere - i);
//         }

//         return sphere;
//     }

//     return {};
// }

// auto createMesh(Device& device, const CreateMeshConfiguration& conf) -> uint32_t {
//     auto mesh = createMesh(conf);
//     return addMesh(device, mesh);
// }

// static auto createMaterial(const CreateMaterialConfiguration& conf) -> Material {
//     Material material;
//     material.setKd(conf.Kd);
//     material.setKs(conf.Ks);
//     material.Ns = conf.Ns;
//     material.d = conf.d;

//     return material;
// }

// auto createMaterial(Device& device, const CreateMaterialConfiguration& conf) -> uint32_t {
//     auto material = createMaterial(conf);

//     if (!conf.KdMapName.empty()) {
//         loadTexture(device, conf.KdMapName);

//         auto KdMap = findTexture(device, make_hash(conf.KdMapName));

//         // uint32_t idx = 0;
//         // for (auto& KdHandle : device.textureHandles_) {
//         //     if (KdHandle == KdMap.handle) {
//         //         break;
//         //     }

//         //     idx++;
//         // }

//         material.mapKd = findTextureHandleRef(device, KdMap.handle);
//     }

//     return addMaterial(device, material);
// }

} // namespace Graphics