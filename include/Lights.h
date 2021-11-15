//
// Created by Supakorn on 11/9/2021.
//

#pragma once
#include "common.h"

enum LightType : uint32_t
{
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1,
};

struct Light
{
    LightType lightType;
    float intensity;

    VEC4_ALIGN
    glm::vec4 color;
    glm::vec4 position;
    glm::vec4 parameters;

    static VkShaderStageFlags stageFlags()
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    static VkDescriptorSetLayoutBinding directionalLightTransfLayoutBinding(uint32_t const& binding)
    {
        VkDescriptorSetLayoutBinding uboLayout = {};
        uboLayout.binding = binding;
        uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayout.descriptorCount = 1;
        uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayout.pImmutableSamplers = nullptr;

        return uboLayout;
    };
};

namespace Lights
{
}