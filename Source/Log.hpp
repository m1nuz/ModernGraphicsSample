#pragma once

#include <fmt/core.h>

#define LOG_ERROR(...)                                                                                                                     \
    do {                                                                                                                                   \
        fmt::println("ERROR: " __VA_ARGS__);                                                                                               \
    } while (0)
#define LOG_DEBUG(...)                                                                                                                     \
    do {                                                                                                                                   \
        fmt::println("DEBUG: " __VA_ARGS__);                                                                                               \
    } while (0)

#define LOG_INFO(...)                                                                                                                      \
    do {                                                                                                                                   \
        fmt::println("INFO: " __VA_ARGS__);                                                                                                \
    } while (0)
