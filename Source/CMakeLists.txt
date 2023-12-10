find_package(Threads REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

set(APP_NAME "Sample")

add_executable(${APP_NAME}
    Renderer.cpp
    Graphics.cpp
    DebugOutput.cpp
    Main.cpp
)

target_compile_features(${APP_NAME}
    PUBLIC
    	cxx_std_20
)

target_compile_options(${APP_NAME} 
    PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-pedantic -Wall -Wextra -Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /WX>
)

target_compile_definitions(${APP_NAME} 
    PUBLIC 
        GLFW_INCLUDE_NONE 
        RESOURCE_PATH="../../Assets"
)

target_include_directories(${APP_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../External>"
)

target_link_libraries(${APP_NAME}
    PRIVATE
        glfw
        glm
        stb_image
        ImGui
        xxhash
        fmt::fmt
        Threads::Threads
)

function(add_shaders)
    if(NOT ARGN)
        return()
    endif()

    foreach(SHADER ${ARGN})
        get_filename_component(FILE_NAME ${SHADER} NAME)
        configure_file("${SHADER}" "${CMAKE_BINARY_DIR}/../Assets/Shaders/${FILE_NAME}" COPYONLY)
    endforeach()
endfunction()


add_shaders(
    Mesh.vert
    Mesh.frag
    Culling.comp
)