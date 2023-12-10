#include "Graphics.hpp"
#include "Log.hpp"
#include "Math.hpp"
#include "Renderer.hpp"

#include <GLFW/glfw3.h>

#include <glad/gl.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

Graphics::Device device;
Graphics::Camera camera;

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    Graphics::resize(device, { width, height });
}

void cursorPositionCallback(GLFWwindow*, double xpos, double ypos) {
    static double xpos_prev = xpos;
    static double ypos_prev = ypos;

    double dx = xpos - xpos_prev;
    double dy = ypos - ypos_prev;

    xpos_prev = xpos;
    ypos_prev = ypos;

    camera.theta = camera.theta + 0.01f * float(dx);
    camera.phi = glm::clamp(camera.phi + 0.01f * float(dy), -1.5f, +1.5f);
}

void scrollCallback(GLFWwindow*, [[maybe_unused]] double xoffset, double yoffset) {
    camera.distance = glm::clamp(camera.distance + 0.3f * static_cast<float>(yoffset), 1.0f, 50.0f);
}

static void showOverlay() {
    static int location = 1;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (location >= 0) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    } else if (location == -2) {
        // Center window
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Debug Overlay", nullptr, window_flags)) {
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Separator();
        if (ImGui::IsMousePosValid())
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonMiddle)) {
            if (ImGui::MenuItem("Custom", NULL, location == -1))
                location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2))
                location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3))
                location = 3;

            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

static auto showRendererOptions() {
    ImGui::Begin("Options");
    ImGui::Checkbox("Instance culling", &device.culling);
    ImGui::TextUnformatted(fmt::format("Draw instances: {}", device.drawInstances).c_str());
    ImGui::TextUnformatted(fmt::format("Visible instances: {}", device.visibleInstances).c_str());
    ImGui::End();
}

int main() {
    bool fullscreen = false;
    bool debugContext = true;
    [[maybe_unused]] bool vsync = true;
    int32_t windowWidth = 1920;
    int32_t windowHeight = 1080;

    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback([](int error, const char* description) { fmt::println("ERROR: {} {}", error, description); });

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debugContext);
    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE, false);

    auto monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    auto window = glfwCreateWindow(windowWidth, windowHeight, "Sample", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoaderLoadGL()) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSwapInterval(vsync ? GLFW_TRUE : GLFW_FALSE);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glfwShowWindow(window);

    const char* glsl_version = "#version 150";

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    constexpr int N = 10;

    if (!Graphics::initialize(device, { .window = window, .numObjects = N * N * N })) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    Graphics::addDirectionalLight(device, { .direction = { 0.f, 10.f, 10.f }, .color = vec3 { 1.f, 1.f, 1.f }, .intensity = 1.f });

    uint32_t cubeRef = Graphics::createMesh(device, { .sphere = 5 });

    //
    // http://devernay.free.fr/cours/opengl/materials.html
    //
    //  gold
    uint32_t goldRef = Graphics::createMaterial(device,
        { .Kd = vec3 { 0.75164, 0.60648, 0.22648 },
            .Ks = vec3 { 0.628281, 0.555802, 0.366065 },
            .KdMap = "../../Assets/Textures/texture_09_orange.png" });
    // copper
    uint32_t copperRef = Graphics::createMaterial(device,
        { .Kd = vec3 { 0.7038, 0.27048, 0.0828 },
            .Ks = vec3 { 0.256777, 0.137622, 0.086014 },
            .KdMap = "../../Assets/Textures/texture_09_green.png" });
    // silver
    uint32_t silverRef = Graphics::createMaterial(device,
        { .Kd = vec3 { 0.50754, 0.50754, 0.50754 },
            .Ks = vec3 { 0.508273, 0.508273, 0.508273 },
            .KdMap = "../../Assets/Textures/texture_07.png" });
    // chrome
    uint32_t chromeRef = Graphics::createMaterial(device,
        { .Kd = vec3 { 0.4, 0.4, 0.4 }, .Ks = vec3 { 0.774597, 0.774597, 0.774597 }, .KdMap = "../../Assets/Textures/texture_08.png" });

    std::vector<Graphics::Object> objects;
    for (int x = -N; x <= N; x++) {
        for (int y = -N; y <= N; y++) {
            for (int z = -N; z <= N; z++) {
                Graphics::Object object;
                object.transform = glm::translate(mat4 { 1.f }, vec3(x * 1.5f, y * 1.5f, z * 1.5f));
                object.transform = glm::scale(object.transform, vec3 { 1.f });
                object.meshRef = cubeRef;

                switch ((x + N) % 4) {
                case 0:
                    object.materialRef = goldRef;
                    break;
                case 1:
                    object.materialRef = silverRef;
                    break;
                case 2:
                    object.materialRef = copperRef;
                    break;
                case 3:
                    object.materialRef = chromeRef;
                    break;
                }

                objects.push_back(object);
            }
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (int state = glfwGetKey(window, GLFW_KEY_ESCAPE); state == GLFW_PRESS) {
            break;
        }

        Graphics::present(device, camera, objects);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        showOverlay();
        showRendererOptions();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    Graphics::cleanup(device);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return EXIT_SUCCESS;
}