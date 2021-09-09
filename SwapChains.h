//
// Created by Supakorn on 9/4/2021.
//

#pragma once
#include "common.h"


struct SwapChainsDetail
{
public:
    SwapChainsDetail() = default;
    explicit SwapChainsDetail(VkPhysicalDevice const& dev, VkSurfaceKHR const& surface);

    [[nodiscard]]
    bool adequate();

    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    [[nodiscard]]
    VkSurfaceFormatKHR selectFmt(
            VkFormat const& preferredFmt=VK_FORMAT_B8G8R8A8_SRGB,
            VkColorSpaceKHR const& preferredColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) const;

    [[nodiscard]]
    VkPresentModeKHR chooseSwapPresentMode(
            VkPresentModeKHR const& preferred=VK_PRESENT_MODE_IMMEDIATE_KHR) const;

    [[nodiscard]]
    VkExtent2D chooseSwapExtent(
            uint32_t const& preferredWidth,
            uint32_t const& preferredHeight) const
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
};