//
// Created by Supakorn on 11/14/2021.
//

#include "common.h"
#include "Lights.h"

VkDescriptorSetLayoutBinding Light::directionalLightTransfLayoutBinding(uint32_t const& binding)
{
    VkDescriptorSetLayoutBinding uboLayout = {};
    uboLayout.binding = binding;
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayout.descriptorCount = 1;
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayout.pImmutableSamplers = nullptr;

    return uboLayout;
}

VkShaderStageFlags Light::stageFlags()
{
    return VK_SHADER_STAGE_FRAGMENT_BIT;
}

namespace Lights
{
VkDescriptorSetLayoutBinding shadowmapImgViewLayoutBinding(uint32_t const& binding)
{
    VkDescriptorSetLayoutBinding imgBindingData = {};
    imgBindingData.binding = binding;
    imgBindingData.descriptorCount = 1;
    imgBindingData.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    imgBindingData.pImmutableSamplers = nullptr;
    imgBindingData.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    return imgBindingData;
}
} // namespace Lights
