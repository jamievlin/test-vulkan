//
// Created by jamie on 7/9/21.
//

#include "QueueFamilies.h"

QueueFamilies::QueueFamilies(VkPhysicalDevice const& dev, VkSurfaceKHR const& surf) :
    graphicsFamily(nullopt), presentationFamily(nullopt)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyVec(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilyVec.data());

    for (int i = 0; i < queueFamilyVec.size(); ++i)
    {
        auto const& queueFamilyProperty = queueFamilyVec[i];

        if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surf, &presentSupport);
        if (presentSupport)
        {
            presentationFamily = i;
        }
    }
}

bool QueueFamilies::suitable() const
{
    return graphicsFamily.has_value() and
        presentationFamily.has_value();
}
