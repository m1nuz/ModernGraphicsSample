include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.1
    CMAKE_ARGS -DFMT_DOC=OFF -DFMT_INSTALL=OFF -DFMT_TEST=OFF
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG master
)

FetchContent_Declare(
    stb_image
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    DOWNLOAD_NO_EXTRACT true
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90
)


FetchContent_Declare(
    tiny_gltf
    URL https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h
    DOWNLOAD_NO_EXTRACT true
)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG v0.20
)

FetchContent_MakeAvailable(fmt glm stb_image imgui tiny_gltf json meshoptimizer)

add_library(stb_image INTERFACE)

add_library(stb::image
    ALIAS
        stb_image
)

target_include_directories(stb_image
    INTERFACE
    $<BUILD_INTERFACE:${stb_image_SOURCE_DIR}>
)

add_library(tiny_gltf INTERFACE)

target_include_directories(tiny_gltf
    INTERFACE
    $<BUILD_INTERFACE:${tiny_gltf_SOURCE_DIR}>
)

add_library(ImGui STATIC 
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

add_library(ImGui::ImGui
    ALIAS 
        ImGui
)

target_compile_definitions(ImGui
    PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD
)

target_include_directories(ImGui
    PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>   
)