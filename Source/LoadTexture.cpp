#include "Common.hpp"
#include "Graphics.hpp"
#include "Hash.hpp"
#include "Log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Graphics {

auto loadTexture(Device& device, std::string_view filepath) -> void {

    int width = 0, height = 0, channels = 0;
    const int req_comp = STBI_default;

    const auto ext = getFilePathExt(filepath);
    const bool isHDRimage = ext == ".hdr";

    Format format = Format::Undefined;
    size_t componentSize = 1;

    if (isHDRimage) {
        stbi_set_flip_vertically_on_load(true);
        auto ptr = stbi_loadf(std::data(filepath), &width, &height, &channels, req_comp);
        assert(ptr);

        componentSize = 4;
        if (channels == 3) {
            format = Format::R32G32B32_FLOAT;
        } else if (channels == 4) {
            format = Format::R16G16B16A16_FLOAT;
        }

        const size_t size = width * height * channels * componentSize;

        createTexture2D(device,
            { .tag = make_hash(filepath),
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .format = format,
                .mipLevels = 4,
                .generateMipMaps = true,
                .bindless = false,
                .pixels = std::span { reinterpret_cast<uint8_t*>(ptr), size } });

        stbi_image_free(ptr);
    } else {
        stbi_set_flip_vertically_on_load(false);
        auto ptr = stbi_load(std::data(filepath), &width, &height, &channels, req_comp);
        assert(ptr);

        componentSize = 1;
        if (channels == 2) {
            format = Format::R8G8_UNORM;
        } else if (channels == 3) {
            format = Format::R8G8B8_UNORM;
        } else if (channels == 4) {
            format = Format::R8G8B8A8_UNORM;
        }

        const size_t size = width * height * channels * componentSize;

        createTexture2D(device,
            { .tag = make_hash(filepath),
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .format = format,
                .mipLevels = 4,
                .generateMipMaps = true,
                .bindless = true,
                .pixels = std::span { ptr, size } });

        stbi_image_free(ptr);
    }

    stbi_set_flip_vertically_on_load(false);
}

} // namespace Graphics
