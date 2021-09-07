//
// Created by Supakorn on 9/6/2021.
//

#include "SwapchainSupport.h"

VkResult SwapchainImageSupport::createImageView(VkImage const& swapChainImage, VkFormat const& swapchainFormat)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapchainFormat;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(*logicalDev, &createInfo, nullptr, &imageView);
}

VkResult SwapchainImageSupport::createFramebuffer(VkRenderPass const& renderPass, VkExtent2D const& extent)
{
    // creating frame buffer
    VkImageView attachments[] = { imageView };

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = attachments;
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    createInfo.layers = 1;

    return vkCreateFramebuffer(*logicalDev, &createInfo, nullptr, &frameBuffer);
}

SwapchainImageSupport& SwapchainImageSupport::operator=(SwapchainImageSupport&& swpImgSupport) noexcept
{
    logicalDev = swpImgSupport.logicalDev;
    imageView = swpImgSupport.imageView;
    frameBuffer = swpImgSupport.frameBuffer;

    swpImgSupport.logicalDev = nullptr;
    swpImgSupport.imageView = VK_NULL_HANDLE;
    swpImgSupport.frameBuffer = VK_NULL_HANDLE;

    return *this;
}

SwapchainImageSupport::SwapchainImageSupport(SwapchainImageSupport&& swpImgSupport) noexcept:
        logicalDev(swpImgSupport.logicalDev)
{

    imageView = swpImgSupport.imageView;
    frameBuffer = swpImgSupport.frameBuffer;

    swpImgSupport.logicalDev = nullptr;
    swpImgSupport.imageView = VK_NULL_HANDLE;
    swpImgSupport.frameBuffer = VK_NULL_HANDLE;
}

SwapchainImageSupport::SwapchainImageSupport(VkDevice* logicalDev, VkRenderPass const& renderPass,
                                             VkExtent2D const& extent, VkImage const& swapChainImage,
                                             VkFormat const& swapchainFormat) : logicalDev(logicalDev)
{

    CHECK_VK_SUCCESS(
            createImageView(swapChainImage, swapchainFormat),
            "Cannot create image view!");
    CHECK_VK_SUCCESS(
            createFramebuffer(renderPass, extent),
            "Cannot create framebuffer!");
}

SwapchainImageSupport::~SwapchainImageSupport()
{
    if (logicalDev)
    {
        vkDestroyFramebuffer(*logicalDev, frameBuffer, nullptr);
        vkDestroyImageView(*logicalDev, imageView, nullptr);
    }
}
