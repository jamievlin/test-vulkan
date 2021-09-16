//
// Created by Supakorn on 9/8/2021.
//

#include "SwapchainComponent.h"

VkResult
SwapchainComponents::initSwapChain(
        VkPhysicalDevice const& physDevice,
        std::pair <size_t, size_t> const& windowHeight,
        VkSurfaceKHR const& surface)
{

    if (!detail.adequate())
    {
        throw std::runtime_error(ErrorMessages::FAILED_CREATE_SWAP_CHAIN);
    }

    swapchainFormat=detail.selectFmt();
    swapchainExtent=detail.chooseSwapExtent(windowHeight.first, windowHeight.second);

    uint32_t imgCount=std::min(
            detail.capabilities.minImageCount + 1,
            detail.capabilities.maxImageCount);
    if (imgCount == 0)
    {
        throw std::runtime_error(ErrorMessages::FAILED_DRIVER_NOT_SUPPORT_IMGBUFFER);
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imgCount;
    createInfo.imageFormat = swapchainFormat.format;
    createInfo.imageColorSpace = swapchainFormat.colorSpace;

    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilies fam(physDevice, surface);
    if (not fam.suitable())
    {
        throw std::runtime_error(ErrorMessages::FAILED_CREATE_QUEUE_FAMILY);
    }
    uint32_t idx[] = {
            fam.graphicsFamily.value(),
            fam.presentationFamily.value()
    };

    if (fam.graphicsFamily != fam.presentationFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = idx;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = detail.capabilities.currentTransform;

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = detail.chooseSwapPresentMode();
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    return vkCreateSwapchainKHR(*logicalDev, &createInfo, nullptr, &swapChain);
}

VkResult SwapchainComponents::createRenderPasses()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;

    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dep;

    return vkCreateRenderPass(*logicalDev, &renderPassCreateInfo, nullptr, &renderPass);
}

SwapchainComponents::SwapchainComponents(
        VkDevice* logicalDev, VkPhysicalDevice const& physDevice,
        VkSurfaceKHR const& surface, std::pair<size_t, size_t> const& windowHeight) : detail(physDevice, surface), logicalDev(logicalDev)
{
    CHECK_VK_SUCCESS(
            initSwapChain(physDevice, windowHeight, surface),
            ErrorMessages::FAILED_CREATE_SWAP_CHAIN);

    // get swap chain image
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(*logicalDev, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(*logicalDev, swapChain, &imageCount, swapChainImages.data());

    CHECK_VK_SUCCESS(
            createRenderPasses(),
            ErrorMessages::FAILED_CREATE_RENDER_PASS);

    std::transform(swapChainImages.begin(), swapChainImages.end(),
                   std::back_inserter(swapchainSupport),
                   [this, &fmt=swapchainFormat.format](VkImage& swapChainImg)
                   {
                       return SwapchainImageSupport(
                               this->logicalDev, this->renderPass, this->swapchainExtent,
                               swapChainImg, fmt);
                   });
}

SwapchainComponents::SwapchainComponents(SwapchainComponents&& swpchainComp) noexcept:
        detail(std::move(swpchainComp.detail)), logicalDev(swpchainComp.logicalDev)
{
    swapChain = std::move(swpchainComp.swapChain);
    swapChainImages = std::move(swpchainComp.swapChainImages);
    swapchainFormat = std::move(swpchainComp.swapchainFormat);
    swapchainExtent = std::move(swpchainComp.swapchainExtent);
    swapchainSupport = std::move(swpchainComp.swapchainSupport);
    renderPass = std::move(swpchainComp.renderPass);

    swpchainComp.logicalDev = nullptr;
}

SwapchainComponents& SwapchainComponents::operator=(SwapchainComponents&& swpchainComp) noexcept
{
    detail = std::move(swpchainComp.detail);
    logicalDev = std::move(swpchainComp.logicalDev);

    swapChain = std::move(swpchainComp.swapChain);
    swapChainImages = std::move(swpchainComp.swapChainImages);
    swapchainFormat = std::move(swpchainComp.swapchainFormat);
    swapchainExtent = std::move(swpchainComp.swapchainExtent);
    swapchainSupport = std::move(swpchainComp.swapchainSupport);
    renderPass = std::move(swpchainComp.renderPass);

    swpchainComp.logicalDev = nullptr;

    return *this;
}

SwapchainComponents::~SwapchainComponents()
{
    if (logicalDev)
    {
        vkDestroyRenderPass(*logicalDev, renderPass, nullptr);
        swapchainSupport.clear();
        vkDestroySwapchainKHR(*logicalDev, swapChain, nullptr);
    }
}
