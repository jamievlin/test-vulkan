//
// Created by Supakorn on 9/17/2021.
//

#include "UniformObjects.h"

VkDescriptorSetLayoutBinding UniformObjects::descriptorSetLayout(uint32_t binding)
{
    VkDescriptorSetLayoutBinding uboLayout = {};
    uboLayout.binding = binding;
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayout.descriptorCount = 1;
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayout.pImmutableSamplers = nullptr;

    return uboLayout;
}