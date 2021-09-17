//
// Created by jamie on 7/9/21.
//

#include "QueueFamilies.h"

QueueFamilies::QueueFamilies(VkPhysicalDevice const& dev, VkSurfaceKHR const& surf) :
    graphicsFamily(nullopt), presentationFamily(nullopt), transferFamily(nullopt)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyVec(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilyVec.data());

    std::vector<uint32_t> possibleTransferQueues;

    for (int i = 0; i < queueFamilyVec.size(); ++i)
    {
        auto const& queueFamilyProperty = queueFamilyVec[i];
        auto const& queueFlags = queueFamilyProperty.queueFlags;

        if (queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surf, &presentSupport);
        if (presentSupport)
        {
            presentationFamily = i;
        }

        if (queueFlags & VK_QUEUE_TRANSFER_BIT
            && (queueFlags & ~VK_QUEUE_GRAPHICS_BIT))
        {
            if (i != graphicsFamily && i != presentationFamily)
            {
                // ideally, a different queue family.
                transferFamily = i;
            }

            if (!transferFamily.has_value())
            {
                possibleTransferQueues.emplace_back(i);
            }
        }
    }

    if (!transferFamily.has_value() && !possibleTransferQueues.empty())
    {
        transferFamily = *possibleTransferQueues.begin();
    }
}

bool QueueFamilies::suitable() const
{
    return graphicsFamily.has_value() and
        presentationFamily.has_value();
}

uint32_t QueueFamilies::transferQueueFamily()
{
    return transferFamily.value_or(graphicsFamily.value());
}

std::set<uint32_t> QueueFamilies::queuesForTransfer()
{
    return std::set<uint32_t> { graphicsFamily.value(), transferQueueFamily() };
}
