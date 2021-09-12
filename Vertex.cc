//
// Created by Supakorn on 9/11/2021.
//

#include "Vertex.h"

VkVertexInputBindingDescription Vertex::bindingDescription()
{
    VkVertexInputBindingDescription desc = {};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::attributeDescription()
{
    VkVertexInputAttributeDescription atrPos = {};
    VkVertexInputAttributeDescription atrColor = {};

    atrPos.binding = 0;
    atrPos.location = 0;
    atrPos.format = VK_FORMAT_R32G32_SFLOAT;
    atrPos.offset = offsetof(Vertex, pos);

    atrColor.binding = 0;
    atrColor.location = 1;
    atrColor.format = VK_FORMAT_R32G32B32_SFLOAT;
    atrColor.offset = offsetof(Vertex, color);

    return {atrPos, atrColor};
}
