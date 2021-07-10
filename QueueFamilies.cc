//
// Created by jamie on 7/9/21.
//

#include "QueueFamilies.h"

QueueFamilies::QueueFamilies(VkPhysicalDevice const& dev) : graphicsFamily(nullopt)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyVec(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilyVec.data());

    int i = 0;

    for (auto const& queueFamilyProperty : queueFamilyVec)
    {
        if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
        }
        i++;
    }
}

bool QueueFamilies::suitable() const
{
    return graphicsFamily.has_value();
}
