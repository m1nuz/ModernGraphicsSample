#include "Renderer.hpp"

#include <fmt/core.h>
#include <glad/gl.h>

namespace Graphics {

void debugMessageOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, [[maybe_unused]] GLsizei length, const char* message,
    [[maybe_unused]] const void* userParam) {
    auto params = reinterpret_cast<const DebugOutputParams*>(userParam);
    if (params && !params->showNotifications) {
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;
        }
    }

    if (params && !params->showPerformance) {
        if (type == GL_DEBUG_TYPE_PERFORMANCE) {
            return;
        }
    }

    const auto sourceString = [source]() {
        switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "OTHER";
        }

        return "";
    }();

    const auto typeString = [type]() {
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";
        }

        return "";
    }();

    const auto severityString = [severity]() {
        switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "NOTIFICATION";
        case GL_DEBUG_SEVERITY_LOW:
            return "LOW";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "MEDIUM";
        case GL_DEBUG_SEVERITY_HIGH:
            return "HIGH";
        }

        return "";
    }();

    fmt::println("{} {} {} {} {}", sourceString, typeString, severityString, id, message);
}

} // namespace Graphics