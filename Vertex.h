//
// Created by Supakorn on 9/11/2021.
//

#pragma once
#include "common.h"

struct Vertex
{
    glm::vec2 pos = glm::vec2(0,0);
    glm::vec3 color = glm::vec3(0,0,0);

    // relevant static functions

    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription,2> attributeDescription();
};



