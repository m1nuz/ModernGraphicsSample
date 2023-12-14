#pragma once

#include <string>

inline auto getFilePathExt(std::string_view filepath) -> std::string_view {
    if (auto pos = filepath.find_last_of('.'); pos != std::string_view::npos) {
        return filepath.substr(pos);
    }

    return {};
}
