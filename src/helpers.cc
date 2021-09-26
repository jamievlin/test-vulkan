//
// Created by Supakorn on 9/25/2021.
//

#include "helpers.h"

namespace helpers
{
    std::string searchPath(std::string const& file)
    {
        if (fileExists(".", file))
        {
            return file;
        }

        size_t requiredSize;
        getenv_s(&requiredSize, nullptr, 0, "SEARCH_PATHS");

        if (requiredSize > 0)
        {
            std::vector<char> buffer;
            buffer.resize(requiredSize);
            getenv_s(&requiredSize, buffer.data(), requiredSize, "SEARCH_PATHS");

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
            for (size_t j=0; j < inputIm.get_height(); ++j)
            {

                auto colorData = inputIm.get_pixel(i,j);
                uint32_t pixelData =
                        colorData.red |
                        colorData.green << (sizeof(uint8_t) * 8) |
                        colorData.blue << (2 * sizeof(uint8_t) * 8) |
                        colorData.alpha << (3 * sizeof(uint8_t) * 8);
                im.emplace_back(pixelData);
            }
        }

        return { inputIm.get_width(), inputIm.get_height(), im };
    }
}