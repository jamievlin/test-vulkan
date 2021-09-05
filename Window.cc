#include "Window.h"
#include "validationTargets.h"
#include "dbgCallBacks.h"
#include "QueueFamilies.h"
#include "Shaders.h"



const std::vector<char const*> requiredDevExtension = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<char const*> getRequiredExts()
{
    uint32_t glfwExtCount = 0;
    char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtCount);

#if ENABLE_VALIDATION_LAYERS == 1
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

std::vector<char const*> getRequiredDeviceExts()
{
    std::vector<char const*> extensions;
    std::copy(
            requiredDevExtension.begin(), requiredDevExtension.end(),
            std::back_inserter(extensions));

    return extensions;
}

bool checkDeviceExtensionSupport(
        VkPhysicalDevice const& dev,
        std::vector<char const*> requiredExts=requiredDevExtension)
{
    uint32_t extCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, extensions.data());

    std::unordered_set<std::string> extNames;
    for (auto const& ext : extensions)
    {
        extNames.emplace(ext.extensionName);
    }

    bool deviceSupported=true;

    for (auto const& extName : requiredExts)
    {
        if (extNames.find(std::string(extName)) == extNames.end())
        {
            deviceSupported = false;
            break;
        }
    }

    return deviceSupported;
}

bool deviceSuitable(VkPhysicalDevice const& dev)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(dev, &properties);
    vkGetPhysicalDeviceFeatures(dev, &features);

    return checkDeviceExtensionSupport(dev) && features.tessellationShader && features.geometryShader;
}



Window::Window() : instance()
{
    glfwInit();

    // window creation
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);

    // instance creation
    if (initInstance() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create Vulkan instance.");
    }

    // setup messenger
#if ENABLE_VALIDATION_LAYERS == 1
    if (setupDebugMessenger() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot set up debug messenger!");
    }
#endif

    if (createSurface() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create surface!");
    }

    dev = selectPhysicalDev();
    if (createLogicalDevice() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create Vulkan logical device.");
    }

    if (initSwapChain() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create Vulkan Swap chain!");
    }
    getSwapChainImage();

    if (not createImageViews())
    {
        throw std::runtime_error("Cannot create image views!");
    }

    if (createRenderPasses() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create render passes!");
    }

    if (createGraphicsPipeline() != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create graphics pipeline!");
    }
}

int Window::mainLoop()
{
    while (not glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return 0;
}

VkDebugUtilsMessengerCreateInfoEXT Window::createDebugInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {};
    dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgInfo.pfnUserCallback = debugCallback;
    dbgInfo.pUserData = reinterpret_cast<void*>(this);

    return dbgInfo;
}

VkResult Window::setupDebugMessenger()
{
    auto createInfo = createDebugInfo();
    auto createDbgFn = getVkExtension<
            PFN_vkCreateDebugUtilsMessengerEXT,
            VkInstance, VkDebugUtilsMessengerCreateInfoEXT const*,
            VkAllocationCallbacks*,
            VkDebugUtilsMessengerEXT*
            >("vkCreateDebugUtilsMessengerEXT");
    return createDbgFn(instance, &createInfo, nullptr, &this->dbgMessenger);
}

Window::~Window()
{
    vkDestroyPipeline(logicalDev, pipeline, nullptr);
    vkDestroyRenderPass(logicalDev, renderPass, nullptr);
    vkDestroyPipelineLayout(logicalDev, pipelineLayout, nullptr);
    for (auto& view : swapchainImgViews)
    {
        vkDestroyImageView(logicalDev, view, nullptr);
    }
    vkDestroySwapchainKHR(logicalDev, swapChain, nullptr);

#if ENABLE_VALIDATION_LAYERS == 1
    auto destroyFn = getVkExtensionVoid<
            PFN_vkDestroyDebugUtilsMessengerEXT,
            VkInstance, VkDebugUtilsMessengerEXT,
            VkAllocationCallbacks const*
            >("vkDestroyDebugUtilsMessengerEXT");
    destroyFn(instance, dbgMessenger, nullptr);
#endif
    vkDestroyDevice(logicalDev, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkResult Window::initInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan!";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "What?";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    auto extNeeded = getRequiredExts();

    uint32_t extCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extProperties(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProperties.data());

    std::unordered_set<std::string> extNames;
    for (auto const& ext : extProperties)
    {
        extNames.emplace(ext.extensionName);
    }

    for (auto const& extName : extNeeded)
    {
        if (extNames.find(std::string(extName)) == extNames.end())
        {
            std::string msg = "Extension " + std::string(extName) + " not found";
            throw std::runtime_error(msg);
        }
    }

#ifdef DEBUG
    std::cerr << "All GLFW Extensions supported" << std::endl;
#endif

#if ENABLE_VALIDATION_LAYERS == 1
    if (not checkValidationSupport())
    {
        throw std::runtime_error("Validation layers not supported.");
    }

    auto dbgCreateInfo = createDebugInfo();
#endif

    VkInstanceCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if ENABLE_VALIDATION_LAYERS == 1
    createInfo.pNext = &dbgCreateInfo;
#else
    createInfo.pNext = nullptr;
#endif
    createInfo.pApplicationInfo = &appInfo;
#if ENABLE_VALIDATION_LAYERS == 1
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extNeeded.size());
    createInfo.ppEnabledExtensionNames = extNeeded.data();

    return vkCreateInstance(&createInfo, nullptr, &instance);
}

VkPhysicalDevice Window::selectPhysicalDev()
{
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0)
    {
        throw std::runtime_error("There is no GPU capable of Vulkan!");
    }

    std::vector<VkPhysicalDevice> devs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, devs.data());

    for (auto const& devList : devs)
    {
        QueueFamilies fam(devList, surface);
        if (deviceSuitable(devList) && fam.suitable())
        {
            physDev = devList;
            break;
        }
    }

    if (physDev == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Could not find a Vulkan-capable GPU!");
    }
    return physDev;
}

VkResult Window::createLogicalDevice()
{
    QueueFamilies queueFam(dev, surface);
    if (not queueFam.suitable())
    {
        throw std::runtime_error("Cannot create device queue on GPU.");
    }


    std::vector<VkDeviceQueueCreateInfo> queueCreateList;

    float priority = 1.f;
    VkDeviceQueueCreateInfo createQueueInfo = {};

    createQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    createQueueInfo.queueFamilyIndex = queueFam.graphicsFamily.value();
    createQueueInfo.queueCount = 1;
    createQueueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures feat = {};

    auto deviceExts = getRequiredDeviceExts();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &createQueueInfo;
    createInfo.pEnabledFeatures = &feat;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    createInfo.ppEnabledExtensionNames = deviceExts.data();

    VkResult result = vkCreateDevice(dev, &createInfo, nullptr, &logicalDev);
    vkGetDeviceQueue(logicalDev, queueFam.graphicsFamily.value(), 0, &graphicsQueue);
    return result;
}

VkResult Window::createSurface()
{
#if defined(__linux__)
    return glfwCreateWindowSurface(instance, window, nullptr, &surface);
#else
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(window);
    createInfo.hinstance = GetModuleHandle(nullptr);

    return vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#endif
}

VkResult Window::initSwapChain()
{
    SwapChainsDetail detail(dev, surface);
    if (!detail.adequate())
    {
        throw std::runtime_error("Cannot create Swap Chain!");
    }

    swapchainFormat=detail.selectFmt();
    swapchainExtent=detail.chooseSwapExtent(WIDTH, HEIGHT);

    uint32_t imgCount=std::min(
            detail.capabilities.minImageCount + 1,
            detail.capabilities.maxImageCount);
    if (imgCount == 0)
    {
        throw std::runtime_error("Driver does not support Image buffer!");
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

    QueueFamilies fam(dev, surface);
    if (not fam.suitable())
    {
        throw std::runtime_error("Cannot create queue family!");
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

    return vkCreateSwapchainKHR(logicalDev, &createInfo, nullptr, &swapChain);
}

void Window::getSwapChainImage()
{
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(logicalDev, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(logicalDev, swapChain, &imageCount, swapChainImages.data());
}

bool Window::createImageViews()
{
    swapchainImgViews.resize(swapChainImages.size());
    bool success=true;

    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainFormat.format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        auto ret = vkCreateImageView(
                logicalDev, &createInfo, nullptr, swapchainImgViews.data() + i);
        if (ret != VK_SUCCESS)
        {
            success = false;
            break;
        }
    }

    return success;
}

VkResult Window::createGraphicsPipeline()
{
    auto [vertShader, ret] = Shaders::createShaderModule(logicalDev, "main.vert.spv");
    auto [fragShader, ret2] = Shaders::createShaderModule(logicalDev, "main.frag.spv");
    if (ret != VK_SUCCESS or ret2 != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create shader");
    }

    VkPipelineShaderStageCreateInfo vertShaderInfo = {};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderInfo.module = vertShader;
    vertShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderInfo = {};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderInfo.module = fragShader;
    fragShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertShaderInfo, fragShaderInfo};

    VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
    vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputInfo.vertexBindingDescriptionCount = 0;
    vertInputInfo.pVertexBindingDescriptions = nullptr;
    vertInputInfo.vertexAttributeDescriptionCount = 0;
    vertInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAsmStateInfo = {};
    inputAsmStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAsmStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height=(float)swapchainExtent.height;
    viewport.minDepth=0.0f;
    viewport.maxDepth=1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0,0};
    scissor.extent = swapchainExtent;
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

    for (float& blendConstant : colorBlendCreateInfo.blendConstants)
    {
        blendConstant = 0.0f;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(
            logicalDev, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)
            != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = stages;
    pipelineCreateInfo.pVertexInputState = &vertInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAsmStateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto retFinal = vkCreateGraphicsPipelines(
            logicalDev,
            VK_NULL_HANDLE,
            1,
            &pipelineCreateInfo,
            nullptr,
            &pipeline);
    vkDestroyShaderModule(logicalDev, vertShader, nullptr);
    vkDestroyShaderModule(logicalDev, fragShader, nullptr);

    return retFinal;
}

VkResult Window::createRenderPasses()
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

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    return vkCreateRenderPass(logicalDev, &renderPassCreateInfo, nullptr, &renderPass);
}
