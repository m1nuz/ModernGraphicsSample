#pragma once

#include <xxhash.h>

static inline auto make_hash(std::string_view s) -> uint64_t {
    return XXH64(std::data(s), std::size(s), 0);
}
