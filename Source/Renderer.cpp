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
constexpr std::string_view CullingShaderName = RESOURCE_PATH "/Shaders/Culling.comp";

constexpr uint64_t MeshPipelineTag = 1;
constexpr uint64_t CullingPipelineTag = 2;

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

    if (conf.numObjects) {
        device.modelMatrices_.reserve(conf.numObjects);
        device.drawables_.reserve(conf.numObjects);
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

    glCreateVertexArrays(1, &device.meshVertexArray);
    glVertexArrayElementBuffer(device.meshVertexArray, indexBuffer.id);

    // per-vertex attributes
    glVertexArrayAttribFormat(device.meshVertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 2, 2, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayVertexBuffer(device.meshVertexArray, 0, vertexBuffer.id, offsetof(Vertex, position), sizeof(Vertex));
    glVertexArrayVertexBuffer(device.meshVertexArray, 1, vertexBuffer.id, offsetof(Vertex, normal), sizeof(Vertex));
    glVertexArrayVertexBuffer(device.meshVertexArray, 2, vertexBuffer.id, offsetof(Vertex, uv), sizeof(Vertex));

    glEnableVertexArrayAttrib(device.meshVertexArray, 0);
    glEnableVertexArrayAttrib(device.meshVertexArray, 1);
    glEnableVertexArrayAttrib(device.meshVertexArray, 2);

    // per-instance attributes
    glVertexArrayAttribFormat(device.meshVertexArray, 3, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 4, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 5, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(device.meshVertexArray, 6, 4, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayVertexBuffer(device.meshVertexArray, 3, instanceBuffer.id, sizeof(vec4) * 0, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 4, instanceBuffer.id, sizeof(vec4) * 1, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 5, instanceBuffer.id, sizeof(vec4) * 2, sizeof(mat4));
    glVertexArrayVertexBuffer(device.meshVertexArray, 6, instanceBuffer.id, sizeof(vec4) * 3, sizeof(mat4));

    glEnableVertexArrayAttrib(device.meshVertexArray, 3);
    glEnableVertexArrayAttrib(device.meshVertexArray, 4);
    glEnableVertexArrayAttrib(device.meshVertexArray, 5);
    glEnableVertexArrayAttrib(device.meshVertexArray, 6);

    glVertexArrayBindingDivisor(device.meshVertexArray, 3, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 4, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 5, 1);
    glVertexArrayBindingDivisor(device.meshVertexArray, 6, 1);

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

    for (auto& fb : device.framebuffers_) {
        if (fb.id == 0) {
            continue;
        }

        glDeleteFramebuffers(1, &fb.id);
    }
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

    glBindFramebuffer(GL_FRAMEBUFFER, device.framebuffers_[0].id);
    glViewport(0, 0, device.framebuffers_[0].width, device.framebuffers_[0].height);
    glClearNamedFramebufferfv(device.framebuffers_[0].id, GL_COLOR, 0, std::data(clearColor));
    glClearNamedFramebufferfv(device.framebuffers_[0].id, GL_DEPTH, 0, &clearDepth);

    if (auto pipeline = findPipeline(device, MeshPipelineTag); pipeline) {
        glBindProgramPipeline(pipeline.id);
        glBindVertexArray(device.meshVertexArray);

        const vec3 viewPos = camera.position();

        auto vs = findShader(device, make_hash(MeshShaderNames[0]));
        auto fs = findShader(device, make_hash(MeshShaderNames[1]));

        auto indirectBuffer = findBuffer(device, IndirectBufferTag);
        auto drawableBuffer = findBuffer(device, DrawableBufferTag);
        auto materialBuffer = findBuffer(device, MaterialBufferTag);
        auto textureHandleBuffer = findBuffer(device, TextureHandleBufferTag);
        auto lightBuffer = findBuffer(device, LightBufferTag);

        glProgramUniformMatrix4fv(vs.id, 0, 1, false, &projection[0][0]);
        glProgramUniformMatrix4fv(vs.id, 1, 1, false, &view[0][0]);
        glProgramUniform3fv(fs.id, 1, 1, &viewPos[0]);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, drawableBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, materialBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, textureHandleBuffer.id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, lightBuffer.id);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer.id);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, instanceCount, sizeof(DrawElementsIndirectCommand));
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        glBindVertexArray(0);
        glBindProgramPipeline(0);
    } else {
        loadPipeline(device, MeshPipelineTag, MeshShaderNames);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

auto resize(Device& device, const ivec2& framebufferSize) -> void {
    device.framebuffers_[0].width = framebufferSize.x;
    device.framebuffers_[0].height = framebufferSize.y;
}

} // namespace Graphics