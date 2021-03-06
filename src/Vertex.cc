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

std::array<VkVertexInputAttributeDescription, 3> Vertex::attributeDescription()
{
    VkVertexInputAttributeDescription atrPos = {};
    VkVertexInputAttributeDescription atrColor = {};
    VkVertexInputAttributeDescription atrTexCoord = {};

    atrPos.binding = 0;
    atrPos.location = 0;
    atrPos.format = VK_FORMAT_R32G32B32_SFLOAT;
    atrPos.offset = offsetof(Vertex, pos);

    atrColor.binding = 0;
    atrColor.location = 1;
    atrColor.format = VK_FORMAT_R32G32B32_SFLOAT;
    atrColor.offset = offsetof(Vertex, color);

    atrTexCoord.binding = 0;
    atrTexCoord.location = 2;
    atrTexCoord.format = VK_FORMAT_R32G32_SFLOAT;
    atrTexCoord.offset = offsetof(Vertex, texCoord);

    return {atrPos, atrColor, atrTexCoord};
}

VkVertexInputBindingDescription NVertex::bindingDescription()
{
    VkVertexInputBindingDescription desc = {};
    desc.binding = 0;
    desc.stride = sizeof(NVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

std::array<VkVertexInputAttributeDescription, 3> NVertex::attributeDescription()
{
    VkVertexInputAttributeDescription atrPos = {};
    VkVertexInputAttributeDescription atrNormal = {};
    VkVertexInputAttributeDescription atrTexCoord = {};

    atrPos.binding = 0;
    atrPos.location = 0;
    atrPos.format = VK_FORMAT_R32G32B32_SFLOAT;
    atrPos.offset = offsetof(NVertex, pos);

    atrNormal.binding = 0;
    atrNormal.location = 1;
    atrNormal.format = VK_FORMAT_R32G32B32_SFLOAT;
    atrNormal.offset = offsetof(NVertex, normal);

    atrTexCoord.binding = 0;
    atrTexCoord.location = 2;
    atrTexCoord.format = VK_FORMAT_R32G32_SFLOAT;
    atrTexCoord.offset = offsetof(NVertex, texCoord);

    return {atrPos, atrNormal, atrTexCoord};
}
