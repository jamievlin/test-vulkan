//
// Created by Supakorn on 9/25/2021.
//

#pragma once
#include "common.h"
#include <fstream>
#include <vector>
#include <utility>
#include <cstdlib>

#include <png++/color.hpp>
#include <png++/image.hpp>

namespace helpers
{
bool fileExists(std::string const& prefix, std::string const& file);
std::string searchPath(std::string const& file);

template <typename PixelFmt> struct img
{
    uint32_t width, height;
    std::vector<PixelFmt> imgData;

    [[nodiscard]]
    uint64_t totalSize() const
    {
        return width * height * sizeof(PixelFmt);
    }

    [[nodiscard]]
    std::pair<uint32_t, uint32_t> size() const
    {
        return {width, height};
    }
};
typedef img<uint32_t> img_r8g8b8a8;

img_r8g8b8a8 fromPng(std::string const& file);
} // namespace helpers
