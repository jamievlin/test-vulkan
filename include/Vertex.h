//
// Created by Supakorn on 9/11/2021.
//

#pragma once
#include "common.h"

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    // relevant static functions

    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription,2> attributeDescription();
};



