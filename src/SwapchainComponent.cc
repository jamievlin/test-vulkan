//
// Created by Supakorn on 9/8/2021.
//

#include "SwapchainComponent.h"
#include "Image.h"

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
    swapchainExtent=detail.chooseSwapExtent(
            CAST_UINT32(windowHeight.first),
            CAST_UINT32(windowHeight.second));

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

    return vkCreateSwapchainKHR(getLogicalDev(), &createInfo, nullptr, &swapChain);
}

VkResult SwapchainComponents::createRenderPasses(VkPhysicalDevice const& physDevice)
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

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = Image::findDepthFormat(physDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkAttachmentDescription> attachments = {
            colorAttachment,
            depthAttachment
            };

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = CAST_UINT32(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dep;

    return vkCreateRenderPass(getLogicalDev(), &renderPassCreateInfo, nullptr, &renderPass);
}

SwapchainComponents::SwapchainComponents(
        VkDevice* logicalDev, VkPhysicalDevice const& physDevice,
        VkSurfaceKHR const& surface, std::pair<size_t, size_t> const& windowSize,
        std::optional<VkImageView> const& depthBufferImgView) :
            AVkGraphicsBase(logicalDev), detail(physDevice, surface)
{
    CHECK_VK_SUCCESS(
            initSwapChain(physDevice, windowSize, surface),
            ErrorMessages::FAILED_CREATE_SWAP_CHAIN);

    // get swap chain image
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(*logicalDev, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(*logicalDev, swapChain, &imageCount, swapChainImages.data());

    CHECK_VK_SUCCESS(createRenderPasses(physDevice), ErrorMessages::FAILED_CREATE_RENDER_PASS);

    std::transform(swapChainImages.begin(), swapChainImages.end(),
                   std::back_inserter(swapchainSupport),
                   [this, &fmt=swapchainFormat.format, &depthBufferImgView](VkImage& swapChainImg)
                   {
                       return SwapchainImageSupport(
                               this->getLogicalDevPtr(), this->renderPass, this->swapchainExtent,
                               swapChainImg, fmt, depthBufferImgView);
                   });

    CHECK_VK_SUCCESS(createDescriptorPool(), ErrorMessages::FAILED_CANNOT_CREATE_DESC_POOL);
}

SwapchainComponents::SwapchainComponents(SwapchainComponents&& swpchainComp) noexcept:
        AVkGraphicsBase(std::move(swpchainComp)),
        detail(std::move(swpchainComp.detail)),
        swapChain(std::move(swpchainComp.swapChain)),
        swapChainImages(std::move(swpchainComp.swapChainImages)),
        swapchainFormat(std::move(swpchainComp.swapchainFormat)),
        swapchainExtent(std::move(swpchainComp.swapchainExtent)),
        swapchainSupport(std::move(swpchainComp.swapchainSupport)),
        renderPass(std::move(swpchainComp.renderPass)),
        descriptorPool(std::move(swpchainComp.descriptorPool))
{
}

SwapchainComponents& SwapchainComponents::operator=(SwapchainComponents&& swpchainComp) noexcept
{
    if (initialized())
    {
        vkDestroyDescriptorPool(getLogicalDev(), descriptorPool, nullptr);
        vkDestroyRenderPass(getLogicalDev(), renderPass, nullptr);
        vkDestroySwapchainKHR(getLogicalDev(), swapChain, nullptr);
    }

    detail = std::move(swpchainComp.detail);
    swapChain = std::move(swpchainComp.swapChain);
    swapChainImages = std::move(swpchainComp.swapChainImages);
    swapchainFormat = std::move(swpchainComp.swapchainFormat);
    swapchainExtent = std::move(swpchainComp.swapchainExtent);
    swapchainSupport = std::move(swpchainComp.swapchainSupport);
    renderPass = std::move(swpchainComp.renderPass);
    descriptorPool = std::move(swpchainComp.descriptorPool);

    AVkGraphicsBase::operator=(std::move(swpchainComp));
    return *this;
}

SwapchainComponents::~SwapchainComponents()
{
    if (initialized())
    {
        vkDestroyDescriptorPool(getLogicalDev(), descriptorPool, nullptr);
        vkDestroyRenderPass(getLogicalDev(), renderPass, nullptr);
        vkDestroySwapchainKHR(getLogicalDev(), swapChain, nullptr);
    }
}

uint32_t SwapchainComponents::imageCount() const
{
    return CAST_UINT32(swapChainImages.size());
}

VkResult SwapchainComponents::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize[4] = {{}, {}, {}, {}};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = imageCount();

    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = imageCount();

    poolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[2].descriptorCount = imageCount();

    poolSize[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize[3].descriptorCount = imageCount();


    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 4;
    createInfo.pPoolSizes = poolSize;
    createInfo.maxSets = 3 * imageCount();

    return vkCreateDescriptorPool(getLogicalDev(), &createInfo, nullptr, &descriptorPool);

}
