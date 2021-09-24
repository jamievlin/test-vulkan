//
// Created by Supakorn on 9/6/2021.
//

#pragma once
#include "common.h"

class SwapchainImageSupport
{
public:
    VkImageView imageView = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    VkFence imagesInFlight = VK_NULL_HANDLE;

    SwapchainImageSupport() = default;
    SwapchainImageSupport(
            VkDevice* logicalDev,
            VkRenderPass const& renderPass, VkExtent2D const& extent,
            VkImage const& swapChainImage, VkFormat const& swapchainFormat);

    SwapchainImageSupport(SwapchainImageSupport const&) = delete;
    SwapchainImageSupport& operator=(SwapchainImageSupport const&) = delete;

    SwapchainImageSupport(SwapchainImageSupport&& swpImgSupport) noexcept;
    SwapchainImageSupport& operator=(SwapchainImageSupport&& swpImgSupport) noexcept;

    ~SwapchainImageSupport();

protected:
    VkResult createFramebuffer(
            VkRenderPass const& renderPass,
            VkExtent2D const& extent);

    VkResult createImageView(
            VkImage const& swapChainImage,
            VkFormat const& swapchainFormat);

private:
    VkDevice* logicalDev = nullptr;
};
