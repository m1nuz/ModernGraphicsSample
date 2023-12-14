#include "Renderer.hpp"
#include "Graphics.hpp"
#include "Hash.hpp"
#include "Log.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>

namespace Graphics {

void debugMessageOutput(
    GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);

struct DrawElementsIndirectCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t baseInstance;
};

constexpr std::array<std::string_view, 2> MeshShaderNames = { RESOURCE_PATH "/Shaders/Mesh.vert", RESOURCE_PATH "/Shaders/Mesh.frag" };
constexpr std::array<std::string_view, 2> PostProcessingShaderNames
    = { RESOURCE_PATH "/Shaders/PostProcessing.vert", RESOURCE_PATH "/Shaders/PostProcessing.frag" };
constexpr std::string_view CullingShaderName = RESOURCE_PATH "/Shaders/Culling.comp";
constexpr std::array<std::string_view, 2> EnvironmentShaderNames
    = { RESOURCE_PATH "/Shaders/Environment.vert", RESOURCE_PATH "/Shaders/Environment.frag" };
constexpr std::array<std::string_view, 2> EquirectangularToCubemapShaderNames { RESOURCE_PATH "/Shaders/Cubemap.vert",
    RESOURCE_PATH "/Shaders/EquirectangularToCubemap.frag" };
constexpr std::array<std::string_view, 2> PrefilterShaderNames { RESOURCE_PATH "/Shaders/Cubemap.vert",
    RESOURCE_PATH "/Shaders/Prefilter.frag" };
constexpr std::array<std::string_view, 2> IrradianceConvolutionShaderNames { RESOURCE_PATH "/Shaders/Cubemap.vert",
    RESOURCE_PATH "/Shaders/IrradianceConvolution.frag" };
constexpr std::array<std::string_view, 2> BRDFShaderNames { RESOURCE_PATH "/Shaders/PostProcessing.vert",
    RESOURCE_PATH "/Shaders/BRDF.frag" };

constexpr std::string_view EnvironmentTextureName = RESOURCE_PATH "/Textures/kloppenheim_02_4k.hdr";

constexpr uint64_t MeshPipelineTag = 1;
constexpr uint64_t CullingPipelineTag = 2;
constexpr uint64_t PostProcessingPipelineTag = 3;
constexpr uint64_t EnvironmentPipelineTag = 4;
constexpr uint64_t EquirectangularToCubemapPipelineTag = 5;
constexpr uint64_t IrradianceConvolutionPipelineTag = 6;
constexpr uint64_t PrefilterPipelineTag = 7;
constexpr uint64_t BRDFPipelineTag = 8;

constexpr uint64_t VertexBufferTag = 1;
constexpr uint64_t IndexBufferTag = 2;
constexpr uint64_t InstanceBufferTag = 3;
constexpr uint64_t IndirectBufferTag = 4;
constexpr uint64_t MaterialBufferTag = 5;
constexpr uint64_t TextureHandleBufferTag = 6;
constexpr uint64_t DrawableBufferTag = 7;
constexpr uint64_t MeshPropertyBufferTag = 8;
constexpr uint64_t LightBufferTag = 9;
constexpr uint64_t LightIndicesBufferTag = 10;

constexpr uint64_t SceneDepthBufferTag = 1;
constexpr uint64_t SceneColorTextureTag = 1;
constexpr uint64_t EnvironmentCubemapTag = 2;
constexpr uint64_t IrradianceCubemapTag = 3;
constexpr uint64_t PrefilterCubemapTag = 4;
constexpr uint64_t brdfLUTTextureTag = 5;

constexpr uint64_t PostProcessingFramebufferTag = 1;

static auto buildEnvironmentCubemap(Device& device) {
    const auto clearColor = std::array { 0.1f, 0.1f, 0.1f, 1.f };
    const auto clearDepth = 1.f;

    static const mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    static const mat4 captureViews[] = { glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f)) };

    if (!device.buildedEnvCubemap) {
        if (auto pipeline = findPipeline(device, EquirectangularToCubemapPipelineTag); pipeline) {

            auto environmentCubemap = findTexture(device, EnvironmentCubemapTag);
            if (!environmentCubemap) {
                return;
            }

            auto hdrTexture = findTexture(device, make_hash(EnvironmentTextureName));
            if (!hdrTexture) {
                return;
            }

            auto vs = findShader(device, make_hash(EquirectangularToCubemapShaderNames[0]));

            uint32_t captureFBO = 0;
            uint32_t captureRBO = 0;
            glCreateFramebuffers(1, &captureFBO);
            glCreateRenderbuffers(1, &captureRBO);

            glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, environmentCubemap.width, environmentCubemap.height);

            glBindProgramPipeline(pipeline.id);

            glProgramUniformMatrix4fv(vs.id, 1, 1, false, &captureProjection[0][0]);

            glBindTextureUnit(0, hdrTexture.id);

            for (uint32_t i = 0; i < 6; ++i) {
                glProgramUniformMatrix4fv(vs.id, 2, 1, false, &captureViews[i][0][0]);

                glNamedFramebufferTextureLayer(captureFBO, GL_COLOR_ATTACHMENT0, environmentCubemap.id, 0, i);

                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

                glClearNamedFramebufferfv(captureFBO, GL_COLOR, 0, std::data(clearColor));
                glClearNamedFramebufferfv(captureFBO, GL_DEPTH, 0, &clearDepth);

                glViewport(0, 0, environmentCubemap.width, environmentCubemap.height);

                drawCube(device);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            glBindTextureUnit(0, 0);

            glBindProgramPipeline(0);

            glGenerateTextureMipmap(environmentCubemap.id);

            glDeleteRenderbuffers(1, &captureRBO);
            glDeleteFramebuffers(1, &captureFBO);

            device.buildedEnvCubemap = true;
            return;
        } else {
            loadPipeline(device, EquirectangularToCubemapPipelineTag, EquirectangularToCubemapShaderNames);
        }
    }

    if (!device.buildedIrradianceCubemap && device.buildedEnvCubemap) {
        if (auto pipeline = findPipeline(device, IrradianceConvolutionPipelineTag); pipeline) {

            auto environmentCubemap = findTexture(device, EnvironmentCubemapTag);
            if (!environmentCubemap) {
                return;
            }

            auto irradianceCubemap = findTexture(device, IrradianceCubemapTag);
            if (!irradianceCubemap) {
                return;
            }

            uint32_t captureFBO = 0;
            uint32_t captureRBO = 0;
            glCreateFramebuffers(1, &captureFBO);
            glCreateRenderbuffers(1, &captureRBO);

            glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, irradianceCubemap.width, irradianceCubemap.height);

            glBindProgramPipeline(pipeline.id);

            glBindTextureUnit(0, environmentCubemap.id);

            auto vs = findShader(device, make_hash(IrradianceConvolutionShaderNames[0]));

            glProgramUniformMatrix4fv(vs.id, 1, 1, false, &captureProjection[0][0]);

            for (unsigned int i = 0; i < 6; ++i) {
                glNamedFramebufferTextureLayer(captureFBO, GL_COLOR_ATTACHMENT0, irradianceCubemap.id, 0, i);

                glProgramUniformMatrix4fv(vs.id, 2, 1, false, &captureViews[i][0][0]);

                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

                glClearNamedFramebufferfv(captureFBO, GL_COLOR, 0, std::data(clearColor));
                glClearNamedFramebufferfv(captureFBO, GL_DEPTH, 0, &clearDepth);

                glViewport(0, 0, irradianceCubemap.width, irradianceCubemap.height);

                drawCube(device);
            }

            glBindTextureUnit(0, 0);

            glBindProgramPipeline(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glDeleteRenderbuffers(1, &captureRBO);
            glDeleteFramebuffers(1, &captureFBO);

            device.buildedIrradianceCubemap = true;
            return;

        } else {
            loadPipeline(device, IrradianceConvolutionPipelineTag, IrradianceConvolutionShaderNames);
        }
    }

    if (!device.buildPrefilterCubemap && device.buildedEnvCubemap && device.buildedIrradianceCubemap) {
        if (auto pipeline = findPipeline(device, PrefilterPipelineTag); pipeline) {

            auto environmentCubemap = findTexture(device, EnvironmentCubemapTag);
            if (!environmentCubemap) {
                return;
            }

            auto prefilterCubemap = findTexture(device, PrefilterCubemapTag);
            if (!prefilterCubemap) {
                return;
            }

            uint32_t captureFBO = 0;
            uint32_t captureRBO = 0;
            glCreateFramebuffers(1, &captureFBO);
            glCreateRenderbuffers(1, &captureRBO);

            glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, prefilterCubemap.width, prefilterCubemap.height);

            glBindProgramPipeline(pipeline.id);

            auto vs = findShader(device, make_hash(PrefilterShaderNames[0]));
            auto fs = findShader(device, make_hash(PrefilterShaderNames[1]));

            glProgramUniformMatrix4fv(vs.id, 1, 1, false, &captureProjection[0][0]);

            glBindTextureUnit(0, environmentCubemap.id);

            const uint32_t maxMipLevels = prefilterCubemap.mipLevels;
            for (uint32_t mip = 0; mip < maxMipLevels; ++mip) {
                const uint32_t mipWidth = static_cast<uint32_t>(prefilterCubemap.width * std::pow(0.5, mip));
                const uint32_t mipHeight = static_cast<uint32_t>(prefilterCubemap.height * std::pow(0.5, mip));

                const float roughness = (float)mip / (float)(maxMipLevels - 1);

                glProgramUniform1f(fs.id, 1, roughness);

                glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);

                for (uint32_t i = 0; i < 6; ++i) {
                    glProgramUniformMatrix4fv(vs.id, 2, 1, false, &captureViews[i][0][0]);

                    glNamedFramebufferTextureLayer(captureFBO, GL_COLOR_ATTACHMENT0, prefilterCubemap.id, mip, i);

                    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

                    glClearNamedFramebufferfv(captureFBO, GL_COLOR, 0, std::data(clearColor));
                    glClearNamedFramebufferfv(captureFBO, GL_DEPTH, 0, &clearDepth);

                    glViewport(0, 0, mipWidth, mipHeight);

                    drawCube(device);
                }
            }

            glBindTextureUnit(0, 0);

            glBindProgramPipeline(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glDeleteRenderbuffers(1, &captureRBO);
            glDeleteFramebuffers(1, &captureFBO);

            device.buildPrefilterCubemap = true;
            return;
        } else {
            loadPipeline(device, PrefilterPipelineTag, PrefilterShaderNames);
        }
    }

    if (!device.buildBRDFLUTTexture) {
        if (auto pipeline = findPipeline(device, BRDFPipelineTag); pipeline) {

            auto brdfLUTTexture = findTexture(device, brdfLUTTextureTag);
            if (!brdfLUTTexture) {
                return;
            }

            uint32_t captureFBO = 0;
            uint32_t captureRBO = 0;
            glCreateFramebuffers(1, &captureFBO);
            glCreateRenderbuffers(1, &captureRBO);

            glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, brdfLUTTexture.width, brdfLUTTexture.height);
            glNamedFramebufferTexture(captureFBO, GL_COLOR_ATTACHMENT0, brdfLUTTexture.id, 0);
            glNamedFramebufferRenderbuffer(captureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            glClearNamedFramebufferfv(captureFBO, GL_COLOR, 0, std::data(clearColor));
            glClearNamedFramebufferfv(captureFBO, GL_DEPTH, 0, &clearDepth);

            glViewport(0, 0, brdfLUTTexture.width, brdfLUTTexture.height);
            glBindProgramPipeline(pipeline.id);

            drawQuad(device);

            glBindProgramPipeline(0);

            device.buildBRDFLUTTexture = true;
            return;
        } else {
            loadPipeline(device, BRDFPipelineTag, BRDFShaderNames);
        }
    }
}

auto initialize(Device& device, const DeviceConfiguration& conf) -> bool {
    assert(conf.window);

    if (conf.numTextures) {
        device.textures_.reserve(conf.numTextures);
        device.textureHandles_.reserve(conf.numTextures);
    }
    if (conf.numShaders) {
        device.shaders_.reserve(conf.numShaders);
    }
    if (conf.numPipelines) {
        device.pipelines_.reserve(conf.numPipelines);
    }
    if (conf.numBuffers) {
        device.buffers_.reserve(conf.numBuffers);
    }
    if (conf.numBuffers) {
        device.framebuffers_.reserve(conf.numFramebuffers);
    }

    if (conf.numVertices) {
        device.vertices_.reserve(conf.numVertices);
    }
    if (conf.numIndices) {
        device.indices_.reserve(conf.numIndices);
    }
    if (conf.numMaterials) {
        device.materials_.reserve(conf.numMaterials);
    }
    if (conf.numMeshes) {
        device.meshProperties_.reserve(conf.numMeshes);
    }
    if (conf.numLights) {
        device.lights_.reserve(conf.numLights);
    }
    if (conf.numModels) {
        device.models_.reserve(conf.numModels);
    }
    if (conf.numEntities) {
        device.modelMatrices_.reserve(conf.numEntities);
        device.drawables_.reserve(conf.numEntities);
    }

    int32_t framebufferWidth = 0, framebufferHeight = 0;
    glfwGetFramebufferSize(conf.window, &framebufferWidth, &framebufferHeight);

    // default Frambuffer
    device.framebuffers_.emplace_back(0, 0, framebufferWidth, framebufferHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, 0, 1);

    GLint contextFlags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
    if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(debugMessageOutput, &device.debugOutputParams_);
    }

    loadShader(device, MeshShaderNames[0]);
    loadShader(device, MeshShaderNames[1]);
    loadShader(device, CullingShaderName);
    loadTexture(device, EnvironmentTextureName);

    auto vertexBuffer = createBuffer(device, { .tag = VertexBufferTag });
    auto indexBuffer = createBuffer(device, { .tag = IndexBufferTag });
    auto instanceBuffer = createBuffer(device, { .tag = InstanceBufferTag });
    createBuffer(device, { .tag = IndirectBufferTag });
    createBuffer(device, { .tag = MaterialBufferTag });
    createBuffer(device, { .tag = LightBufferTag });
    createBuffer(device, { .tag = LightIndicesBufferTag });
    createBuffer(device, { .tag = TextureHandleBufferTag });
    createBuffer(device, { .tag = DrawableBufferTag });
    createBuffer(device, { .tag = MeshPropertyBufferTag });

    auto sceneColorTexture = createTexture2D(device,
        { .tag = SceneColorTextureTag,
            .width = static_cast<uint32_t>(framebufferWidth),
            .height = static_cast<uint32_t>(framebufferHeight),
            .format = Graphics::Format::R16G16B16_FLOAT,
            .mipLevels = 1,
            .generateMipMaps = false,
            .filter = Graphics::TextureFiltering::Bilinear,
            .wrap = Graphics::TextureWrap::ClampToEdge });

    auto sceneDepthBuffer = createRenderbuffer(device,
        { .tag = SceneDepthBufferTag,
            .width = static_cast<uint32_t>(framebufferWidth),
            .height = static_cast<uint32_t>(framebufferHeight),
            .format = Graphics::Format::D24_UNORM });

    auto postProcessingFramebuffer = createFramebuffer(device,
        { .tag = PostProcessingFramebufferTag,
            .width = static_cast<uint32_t>(framebufferWidth),
            .height = static_cast<uint32_t>(framebufferHeight),
            .mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
            .attachments = std::array { Graphics::FramebufferAttachment { .attachment = GL_COLOR_ATTACHMENT0,
                                            .attachmentTarget = GL_TEXTURE_2D,
                                            .renderTarget = sceneColorTexture.id },
                Graphics::FramebufferAttachment {
                    .attachment = GL_DEPTH_ATTACHMENT, .attachmentTarget = GL_RENDERBUFFER, .renderTarget = sceneDepthBuffer.id } },
            .drawBuffers = std::array<GLenum, 1> { GL_COLOR_ATTACHMENT0 } });

    assert(postProcessingFramebuffer.is_complete());

    createTextureCube(device,
        { .tag = EnvironmentCubemapTag,
            .width = 2048,
            .height = 2048,
            .format = Graphics::Format::R16G16B16_FLOAT,
            .mipLevels = 5,
            .generateMipMaps = true,
            .filter = TextureFiltering::Trilinear,
            .wrap = TextureWrap::ClampToEdge });

    createTextureCube(device,
        { .tag = IrradianceCubemapTag,
            .width = 32,
            .height = 32,
            .format = Format::R16G16B16_FLOAT,
            .mipLevels = 1,
            .generateMipMaps = false,
            .filter = TextureFiltering::Bilinear,
            .wrap = TextureWrap::ClampToEdge });

    createTextureCube(device,
        { .tag = PrefilterCubemapTag,
            .width = 256,
            .height = 256,
            .format = Format::R16G16B16_FLOAT,
            .mipLevels = 5,
            .generateMipMaps = true,
            .filter = TextureFiltering::Trilinear });

    createTexture2D(device,
        { .tag = brdfLUTTextureTag,
            .width = 512,
            .height = 512,
            .format = Format::R16G16_FLOAT,
            .mipLevels = 1,
            .generateMipMaps = false,
            .filter = TextureFiltering::Bilinear,
            .wrap = TextureWrap::ClampToEdge });

    glCreateVertexArrays(1, &device.meshVertexArray);
    glVertexArrayElementBuffer(device.meshVertexArray, indexBuffer.id);

    // per-vertex attributes
    glVertexArrayAttribFormat(device.meshVertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 2, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 3, 3, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayVertexBuffer(device.meshVertexArray, 0, vertexBuffer.id, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(device.meshVertexArray, 1, vertexBuffer.id, offsetof(Vertex, normal), sizeof(Vertex));
    glVertexArrayVertexBuffer(device.meshVertexArray, 2, vertexBuffer.id, offsetof(Vertex, uv), sizeof(Vertex));
    glVertexArrayVertexBuffer(device.meshVertexArray, 3, vertexBuffer.id, offsetof(Vertex, tangent), sizeof(Vertex));

    glEnableVertexArrayAttrib(device.meshVertexArray, 0);
    glEnableVertexArrayAttrib(device.meshVertexArray, 1);
    glEnableVertexArrayAttrib(device.meshVertexArray, 2);
    glEnableVertexArrayAttrib(device.meshVertexArray, 3);

    // per-instance attributes
    glVertexArrayAttribFormat(device.meshVertexArray, 4, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 5, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 6, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 7, 4, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayVertexBuffer(device.meshVertexArray, 4, instanceBuffer.id, sizeof(vec4) * 0, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 5, instanceBuffer.id, sizeof(vec4) * 1, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 6, instanceBuffer.id, sizeof(vec4) * 2, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 7, instanceBuffer.id, sizeof(vec4) * 3, sizeof(mat4));

    glEnableVertexArrayAttrib(device.meshVertexArray, 4);
    glEnableVertexArrayAttrib(device.meshVertexArray, 5);
    glEnableVertexArrayAttrib(device.meshVertexArray, 6);
    glEnableVertexArrayAttrib(device.meshVertexArray, 7);

    glVertexArrayBindingDivisor(device.meshVertexArray, 4, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 5, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 6, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 7, 1);

    glCreateVertexArrays(1, &device.fullscreenQuadVertexArray);

    return true;
}

auto cleanup(Device& device) -> void {
    for (const auto t : device.textureHandles_) {
        if (t != 0) {
            glMakeTextureHandleNonResidentARB(t);
        }
    }
    device.textureHandles_.clear();

    for (auto& t : device.textures_) {
        glDeleteTextures(1, &t.id);
    }
    device.textures_.clear();

    for (auto& s : device.shaders_) {
        glDeleteProgram(s.id);
    }
    device.shaders_.clear();

    for (auto& p : device.pipelines_) {
        glDeleteProgramPipelines(1, &p.id);
    }
    device.pipelines_.clear();

    for (auto& b : device.buffers_) {
        glDeleteBuffers(1, &b.id);
    }
    device.buffers_.clear();

    for (auto& rb : device.renderbuffers_) {
        glDeleteRenderbuffers(1, &rb.id);
    }
    device.renderbuffers_.clear();

    for (auto& fb : device.framebuffers_) {
        if (fb.id == 0) {
            continue;
        }

        glDeleteFramebuffers(1, &fb.id);
    }
    device.framebuffers_.clear();
}

static auto updateMaterialBuffers(Device& device) {
    if (!device.reloadMaterialBuffers_) {
        return;
    }

    device.reloadMaterialBuffers_ = false;

    auto materialBuffer = findBuffer(device, MaterialBufferTag);
    auto textureHandleBuffer = findBuffer(device, TextureHandleBufferTag);

    glNamedBufferData(materialBuffer.id, std::size(device.materials_) * sizeof(Material), std::data(device.materials_), GL_DYNAMIC_DRAW);

    if (!device.textureHandles_.empty()) {
        glNamedBufferData(textureHandleBuffer.id, std::size(device.textureHandles_) * sizeof(uint64_t), std::data(device.textureHandles_),
            GL_DYNAMIC_DRAW);
    }
}

static auto updateMeshBuffers(Device& device) {
    if (!device.reloadMeshBuffers_) {
        return;
    }

    device.reloadMeshBuffers_ = false;

    auto vertexBuffer = findBuffer(device, VertexBufferTag);
    auto indexBuffer = findBuffer(device, IndexBufferTag);
    auto meshPropertyBuffer = findBuffer(device, MeshPropertyBufferTag);

    glNamedBufferData(vertexBuffer.id, std::size(device.vertices_) * sizeof(Vertex), std::data(device.vertices_), GL_DYNAMIC_DRAW);
    glNamedBufferData(indexBuffer.id, std::size(device.indices_) * sizeof(uint32_t), std::data(device.indices_), GL_DYNAMIC_DRAW);
    glNamedBufferData(meshPropertyBuffer.id, std::size(device.meshProperties_) * sizeof(MeshProperty), std::data(device.meshProperties_),
        GL_DYNAMIC_DRAW);
}

static auto updateLightBuffer(Device& device) {
    if (!device.reloadLightBuffers_) {
        return;
    }

    device.reloadLightBuffers_ = false;

    auto lightBuffer = findBuffer(device, LightBufferTag);

    glNamedBufferData(lightBuffer.id, std::size(device.lights_) * sizeof(Light), std::data(device.lights_), GL_DYNAMIC_DRAW);
}

auto present(Device& device, Camera& camera, std::span<const Entity> entities) -> void {
    assert(!device.framebuffers_.empty());

    if (!device.buildedEnvCubemap || !device.buildedIrradianceCubemap || !device.buildPrefilterCubemap || !device.buildBRDFLUTTexture) {
        buildEnvironmentCubemap(device);
    }

    const float aspectRation = static_cast<float>(device.framebuffers_[0].width) / static_cast<float>(device.framebuffers_[0].height);

    mat4 projection = camera.projection(aspectRation);
    mat4 view = camera.view();

    updateMaterialBuffers(device);
    updateMeshBuffers(device);
    updateLightBuffer(device);

    device.modelMatrices_.clear();
    device.drawables_.clear();

    for (size_t i = 0; i < std::size(entities); i++) {
        const auto& model = device.models_[entities[i].modelRef];
        for (size_t j = 0; j < std::size(model.meshes); j++) {
            device.modelMatrices_.push_back(entities[i].transform);
            device.drawables_.emplace_back(model.meshes[j].meshRef, model.meshes[j].materialRef);
        }
    }

    int32_t instanceCount = static_cast<int32_t>(std::size(device.drawables_));
    device.drawInstances = instanceCount;

    auto instanceBuffer = findBuffer(device, InstanceBufferTag);
    auto indirectBuffer = findBuffer(device, IndirectBufferTag);
    auto drawableBuffer = findBuffer(device, DrawableBufferTag);

    glNamedBufferData(
        instanceBuffer.id, std::size(device.modelMatrices_) * sizeof(mat4), std::data(device.modelMatrices_), GL_DYNAMIC_DRAW);
    glNamedBufferData(indirectBuffer.id, instanceCount * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
    glNamedBufferData(drawableBuffer.id, std::size(device.drawables_) * sizeof(Drawable), std::data(device.drawables_), GL_DYNAMIC_DRAW);

    //
    // cull invisible objects
    //
    if (auto pipeline = findPipeline(device, CullingPipelineTag); pipeline) {

        int workgroupCount = instanceCount / 1024;
        if (instanceCount % 1024) {
            workgroupCount++;
        }

        glBindProgramPipeline(pipeline.id);

        auto cs = findShader(device, make_hash(CullingShaderName));

        glProgramUniform1f(cs.id, 0, glm::radians(camera.fieldOfView));
        glProgramUniform1f(cs.id, 1, aspectRation);
        glProgramUniform1f(cs.id, 2, camera.nearPlane);
        glProgramUniform1f(cs.id, 3, camera.farPlane);
        glProgramUniformMatrix4fv(cs.id, 4, 1, false, &view[0][0]);
        glProgramUniform1i(cs.id, 5, device.culling ? 0 : 1);

        auto instanceBuffer = findBuffer(device, InstanceBufferTag);
        auto indirectBuffer = findBuffer(device, IndirectBufferTag);
        auto drawableBuffer = findBuffer(device, DrawableBufferTag);
        auto meshPropertyBuffer = findBuffer(device, MeshPropertyBufferTag);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, instanceBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indirectBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, drawableBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, meshPropertyBuffer.id);

        glDispatchCompute(workgroupCount, 1, 1);

        glBindProgramPipeline(0);

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    } else {
        loadPipeline(device, CullingPipelineTag, std::array { CullingShaderName });
    }

    static int visibleInstances = 0;
    static float timeToShowCulledInstances = 0.0f;
    timeToShowCulledInstances += 0.016f;
    if (timeToShowCulledInstances >= 1.0f) {
        timeToShowCulledInstances = 0.f;

        visibleInstances = 0;

        std::vector<DrawElementsIndirectCommand> cmds;
        cmds.resize(instanceCount);

        glGetNamedBufferSubData(indirectBuffer.id, 0, std::size(cmds) * sizeof(DrawElementsIndirectCommand), std::data(cmds));

        for (const auto& cmd : cmds) {
            visibleInstances += cmd.instanceCount;
        }

        device.visibleInstances = visibleInstances;
    }

    //
    // render objects
    //

    const auto clearColor = std::array { 0.1f, 0.1f, 0.1f, 1.f };
    const auto clearDepth = 1.f;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, device.framebuffers_[1].id);
    glViewport(0, 0, device.framebuffers_[1].width, device.framebuffers_[1].height);
    glClearNamedFramebufferfv(device.framebuffers_[1].id, GL_COLOR, 0, std::data(clearColor));
    glClearNamedFramebufferfv(device.framebuffers_[1].id, GL_DEPTH, 0, &clearDepth);

    if (auto pipeline = findPipeline(device, MeshPipelineTag); pipeline) {
        glBindProgramPipeline(pipeline.id);
        glBindVertexArray(device.meshVertexArray);

        const vec3 viewPos = camera.position();

        auto vs = findShader(device, make_hash(MeshShaderNames[0]));
        auto fs = findShader(device, make_hash(MeshShaderNames[1]));

        auto irradianceCubemap = findTexture(device, IrradianceCubemapTag);
        auto prefilterCubemap = findTexture(device, PrefilterCubemapTag);
        auto brdfLUTTexture = findTexture(device, brdfLUTTextureTag);

        auto indirectBuffer = findBuffer(device, IndirectBufferTag);
        auto drawableBuffer = findBuffer(device, DrawableBufferTag);
        auto materialBuffer = findBuffer(device, MaterialBufferTag);
        auto textureHandleBuffer = findBuffer(device, TextureHandleBufferTag);
        auto lightBuffer = findBuffer(device, LightBufferTag);

        glProgramUniformMatrix4fv(vs.id, 0, 1, false, &projection[0][0]);
        glProgramUniformMatrix4fv(vs.id, 1, 1, false, &view[0][0]);
        glProgramUniform3fv(fs.id, 1, 1, &viewPos[0]);
        glProgramUniform1i(fs.id, 2, true);

        glBindTextureUnit(10, irradianceCubemap.id);
        glBindTextureUnit(11, prefilterCubemap.id);
        glBindTextureUnit(12, brdfLUTTexture.id);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, drawableBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, materialBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, textureHandleBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, lightBuffer.id);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer.id);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, instanceCount, sizeof(DrawElementsIndirectCommand));
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        glBindTextureUnit(10, 0);
        glBindTextureUnit(11, 0);
        glBindTextureUnit(12, 0);

        glBindVertexArray(0);
        glBindProgramPipeline(0);
    } else {
        loadPipeline(device, MeshPipelineTag, MeshShaderNames);
    }

    glCullFace(GL_FRONT);

    //
    // render environment
    //
    if (auto pipeline = findPipeline(device, EnvironmentPipelineTag); pipeline) {
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);

        glBindProgramPipeline(pipeline.id);

        auto vs = findShader(device, make_hash(EnvironmentShaderNames[0]));
        auto envTexture = findTexture(device, EnvironmentCubemapTag);

        glBindTextureUnit(0, envTexture.id);

        glProgramUniformMatrix4fv(vs.id, 1, 1, false, &projection[0][0]);
        glProgramUniformMatrix4fv(vs.id, 2, 1, false, &view[0][0]);

        drawCube(device);

        glBindTextureUnit(0, 0);

        glBindProgramPipeline(0);

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    } else {
        loadPipeline(device, EnvironmentPipelineTag, EnvironmentShaderNames);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    //
    // postprocessing
    //
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (auto pipeline = findPipeline(device, PostProcessingPipelineTag); pipeline) {
        glBindProgramPipeline(pipeline.id);

        auto fs = findShader(device, make_hash(PostProcessingShaderNames[1]));
        auto sceneColorTexture = findTexture(device, SceneColorTextureTag);

        glProgramUniform1f(fs.id, 1, device.exposure);
        glProgramUniform1f(fs.id, 2, device.gamma);

        glBindTextureUnit(0, sceneColorTexture.id);

        glBindVertexArray(device.fullscreenQuadVertexArray);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glBindTextureUnit(0, 0);

        glBindProgramPipeline(0);
    } else {
        loadPipeline(device, PostProcessingPipelineTag, PostProcessingShaderNames);
    }
}

auto resize(Device& device, const ivec2& framebufferSize) -> void {
    device.framebuffers_[0].width = framebufferSize.x;
    device.framebuffers_[0].height = framebufferSize.y;
}

} // namespace Graphics