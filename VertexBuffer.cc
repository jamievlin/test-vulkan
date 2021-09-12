//
// Created by Supakorn on 9/11/2021.
//

#include "VertexBuffer.h"

uint32_t getMemoryType(VkPhysicalDevice const& physicalDev, VkMemoryPropertyFlags properties, uint32_t filter)
{
    VkPhysicalDeviceMemoryProperties physMemProperty;
    vkGetPhysicalDeviceMemoryProperties(physicalDev, &physMemProperty);

    uint32_t mask = 1;
    for (int i=0;i<physMemProperty.memoryTypeCount; ++i)
    {
        if (
                (filter & mask) &&
                ((physMemProperty.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
        mask = mask << 1;
    }

    throw std::runtime_error("Cannot find suitable memory type!");
}
