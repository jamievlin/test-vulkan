//
// Created by Supakorn on 9/5/2021.
//
#include "common.h"
#include <fstream>
#include <sstream>
#include "Shaders.h"

namespace Shaders
{
std::vector<uint8_t> readBytecode(std::string const& fileName)
{
    std::ifstream file;
    file.open(fileName, std::ifstream::in | std::ifstream::binary);
    std::stringstream byteSrc;

    if (file)
    {
        byteSrc << file.rdbuf();
        file.close();
    }
    else
    {
        throw std::runtime_error("Cannot load shader file!");
    }

    std::vector<uint8_t> outchr;
    std::string const& outchrTmp = byteSrc.str();
    outchr.reserve(outchrTmp.size());
    outchr.assign(outchrTmp.begin(), outchrTmp.end());

    return outchr;
}

std::pair<VkShaderModule, VkResult> createShaderModule(
    VkDevice const& logicalDev, std::vector<uint8_t> const& spvSource
)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spvSource.size();
    createInfo.pCode = reinterpret_cast<uint32_t const*>(spvSource.data());

    VkShaderModule shaderModule;
    VkResult ret = vkCreateShaderModule(logicalDev, &createInfo, nullptr, &shaderModule);
    return std::make_pair(shaderModule, ret);
}

std::tuple<VkShaderModule, VkResult> createShaderModule(
    VkDevice const& logicalDev, std::string const& fileName
)
{
    return createShaderModule(logicalDev, readBytecode(fileName));
}

VkShaderModule createShaderModuleChecked(VkDevice const& logicalDev, std::string const& fileName)
{
    auto [module, ret] = createShaderModule(logicalDev, readBytecode(fileName));
    CHECK_VK_SUCCESS(ret, "Cannot create shader module!");
    return module;
}
}; // namespace Shaders
