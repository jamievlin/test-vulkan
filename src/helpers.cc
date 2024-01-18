//
// Created by Supakorn on 9/25/2021.
//

#include "helpers.h"

constexpr char const* SEARCH_PATHS_ENV = "SEARCH_PATHS";

namespace helpers
{
std::string searchPath(std::string const& file)
{
    if (fileExists(".", file))
    {
        return file;
    }

        // strtok_s is not C99 standard in Linux

#if defined(_WIN32)
    size_t requiredSize;
    getenv_s(&requiredSize, nullptr, 0, SEARCH_PATHS_ENV);
    if (requiredSize > 0)
    {
        std::vector<char> buffer;
        buffer.resize(requiredSize);
        getenv_s(&requiredSize, buffer.data(), requiredSize, SEARCH_PATHS_ENV);

        char* nextToken;
        char* out = strtok_s(buffer.data(), ";", &nextToken);
        while (out)
        {
            if (fileExists(out, file))
            {
                return std::string(out) + "/" + file;
            }
            out = strtok_s(nullptr, ";", &nextToken);
        }
    }
#else
    char* base_buffer = getenv(SEARCH_PATHS_ENV);
    auto base_buffer_data = std::make_unique<char[]>(strlen(base_buffer) + 1);

    char* out = strtok(base_buffer_data.get(), ";");
    out = strtok(nullptr, out);
    while (out)
    {
        if (fileExists(out, file))
        {
            return std::string(out) + "/" + file;
        }
        out = strtok(nullptr, out);
    }
#endif
    return {};
}

bool fileExists(std::string const& prefix, std::string const& file)
{
    std::ifstream fil(prefix + "/" + file);
    return fil.operator bool();
}

img_r8g8b8a8 fromPng(std::string const& file)
{
    std::vector<uint32_t> im;
    png::image<png::rgba_pixel> inputIm(file);

    for (size_t i = 0; i < inputIm.get_width(); ++i)
    {
        for (size_t j = 0; j < inputIm.get_height(); ++j)
        {

            auto colorData = inputIm.get_pixel(i, j);
            uint32_t pixelData = colorData.red | colorData.green << (sizeof(uint8_t) * 8)
                                 | colorData.blue << (2 * sizeof(uint8_t) * 8)
                                 | colorData.alpha << (3 * sizeof(uint8_t) * 8);
            im.emplace_back(pixelData);
        }
    }

    return {inputIm.get_width(), inputIm.get_height(), im};
}
} // namespace helpers
