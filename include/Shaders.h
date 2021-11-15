//
// Created by Supakorn on 9/5/2021.
//

#pragma once
#include "common.h"

namespace Shaders
{
    std::vector<uint8_t> readBytecode(std::string const& fileName);
    std::pair<VkShaderModule, VkResult> createShaderModule(
            VkDevice const& logicalDev, std::vector<uint8_t> const& spvSource);
    std::tuple<VkShaderModule, VkResult>
            createShaderModule(VkDevice const& logicalDev, std::string const& fileName);

    VkShaderModule
    createShaderModuleChecked(VkDevice const& logicalDev, std::string const& fileName);
} // namespace shaders