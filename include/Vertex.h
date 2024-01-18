//
// Created by Supakorn on 9/11/2021.
//

#pragma once
#include "common.h"

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    // relevant static functions

    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> attributeDescription();
};

struct NVertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    // relevant static functions
    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> attributeDescription();
};
