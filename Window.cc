#include "Window.h"
#include "validationTargets.h"
#include "dbgCallBacks.h"
#include "QueueFamilies.h"




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
}

void Window::mainLoop()
{
    while (not glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
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
