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

    static VkShaderStageFlags stageFlags();
    static VkDescriptorSetLayoutBinding directionalLightTransfLayoutBinding(uint32_t const& binding);;
};

namespace Lights
{
    VkDescriptorSetLayoutBinding shadowmapImgViewLayoutBinding(uint32_t const& binding);
}