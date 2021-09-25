//
// Created by Supakorn on 9/4/2021.
//

#include "SwapChains.h"

SwapChainsDetail::SwapChainsDetail(VkPhysicalDevice const& dev, VkSurfaceKHR const& surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
    formats.resize(formatCount);
    if (formatCount > 0)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, nullptr);
    presentModes.resize(presentModeCount);
    if (presentModeCount > 0)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, presentModes.data());
    }
}

[[nodiscard]]
bool SwapChainsDetail::adequate() const
{
    return not (formats.empty() or presentModes.empty());
}

VkSurfaceFormatKHR SwapChainsDetail::selectFmt(
        VkFormat const& preferredFmt,
        VkColorSpaceKHR const& preferredColorSpace) const
{
    for (auto const& fmt : formats)
    {
        if (fmt.format == preferredFmt and fmt.colorSpace == preferredColorSpace)
        {
            return fmt;
        }
    }
    return formats[0];
}

VkPresentModeKHR SwapChainsDetail::chooseSwapPresentMode(
        VkPresentModeKHR const& preferred) const
{
    for (auto const& mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; // default
}

VkExtent2D SwapChainsDetail::chooseSwapExtent(uint32_t const& preferredWidth, uint32_t const& preferredHeight) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    // else
    VkExtent2D actualExtent = {};
    actualExtent.width = std::clamp(
            preferredWidth,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

    actualExtent.height = std::clamp(
            preferredHeight,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

    return actualExtent;
}
