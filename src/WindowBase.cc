#include "WindowBase.h"

#if defined(ENABLE_VALIDATION_LAYERS)
#include "validationTargets.h"
#include "dbgCallBacks.h"
#endif

std::vector<char const*> const requiredDevExtension = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
        requiredDevExtension.begin(), requiredDevExtension.end(), std::back_inserter(extensions)
    );

    return extensions;
}

bool checkDeviceExtensionSupport(
    VkPhysicalDevice const& dev, std::vector<char const*> const& requiredExts = requiredDevExtension
)
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

    bool deviceSupported = true;

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

    return checkDeviceExtensionSupport(dev) && features.tessellationShader
           && features.geometryShader && features.samplerAnisotropy;
}

WindowBase::WindowBase(size_t const& width, size_t const& height, std::string windowTitle)
    : width(width), height(height), title(std::move(windowTitle))
{
    glfwInit();

    // window creation
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(
        static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr
    );

    // instance creation
    CHECK_VK_SUCCESS(initInstance(), ErrorMessages::FAILED_CANNOT_CREATE_INSTANCE);
    // setup messenger
#if ENABLE_VALIDATION_LAYERS == 1
    CHECK_VK_SUCCESS(setupDebugMessenger(), "Cannot set up debug messenger!");
#endif
    CHECK_VK_SUCCESS(createSurface(), ErrorMessages::FAILED_CANNOT_CREATE_SURFACE);
    dev = selectPhysicalDev();
    CHECK_VK_SUCCESS(createLogicalDevice(), ErrorMessages::FAILED_CANNOT_CREATE_LOGICAL_DEV);
    // end selection of logical device
    // create allocator
    CHECK_VK_SUCCESS(createAllocator(), ErrorMessages::FAILED_CANNOT_CREATE_ALLOCATOR)
    //
}

#if defined(ENABLE_VALIDATION_LAYERS)
VkDebugUtilsMessengerCreateInfoEXT WindowBase::createDebugInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {};
    dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgInfo.pfnUserCallback = debugCallback;
    dbgInfo.pUserData = reinterpret_cast<void*>(this);

    return dbgInfo;
}

VkResult WindowBase::setupDebugMessenger()
{
    auto createInfo = createDebugInfo();
    auto createDbgFn = getVkExtension<
        PFN_vkCreateDebugUtilsMessengerEXT, VkInstance, VkDebugUtilsMessengerCreateInfoEXT const*,
        VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*>("vkCreateDebugUtilsMessengerEXT");
    return createDbgFn(instance, &createInfo, nullptr, &this->dbgMessenger);
}
#endif

WindowBase::~WindowBase()
{
    vmaDestroyAllocator(allocator);
#if ENABLE_VALIDATION_LAYERS == 1
    auto destroyFn = getVkExtensionVoid<
        PFN_vkDestroyDebugUtilsMessengerEXT, VkInstance, VkDebugUtilsMessengerEXT,
        VkAllocationCallbacks const*>("vkDestroyDebugUtilsMessengerEXT");
    destroyFn(instance, dbgMessenger, nullptr);
#endif
    vkDestroyDevice(logicalDev, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkResult WindowBase::initInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan!";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "What?";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION;

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
    createInfo.enabledLayerCount = CAST_UINT32(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    createInfo.enabledExtensionCount = CAST_UINT32(extNeeded.size());
    createInfo.ppEnabledExtensionNames = extNeeded.data();

    return vkCreateInstance(&createInfo, nullptr, &instance);
}

// assume little endian here - x86+ARM are both little endian
#define NVIDIA_VENDOR_ID 0x000010DE
#define AMD_VENDOR_ID 0x00001002

bool GPUHigherPriority(VkPhysicalDevice const& pd1, VkPhysicalDevice const& pd2)
{
    if (pd2 == VK_NULL_HANDLE)
    {
        return true;
    }

    if (pd1 == pd2)
    {
        return false;
    }

    VkPhysicalDeviceProperties dv1 = {}, dv2 = {};
    vkGetPhysicalDeviceProperties(pd1, &dv1);
    vkGetPhysicalDeviceProperties(pd2, &dv2);

    // mostly heuristic here
    // try to avoid software GPU
    bool dv1IsNVIDIAorAMD = dv1.vendorID == NVIDIA_VENDOR_ID || dv1.vendorID == AMD_VENDOR_ID;
    bool dv21IsNVIDIAorAMD = dv2.vendorID == NVIDIA_VENDOR_ID || dv2.vendorID == AMD_VENDOR_ID;
    if (dv1IsNVIDIAorAMD && !dv21IsNVIDIAorAMD)
    {
        return true;
    }

    return false;
}

VkPhysicalDevice WindowBase::selectPhysicalDev()
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
        if (deviceSuitable(devList) && fam.suitable() && GPUHigherPriority(devList, physDev))
        {
            physDev = devList;
        }
    }

    if (physDev == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Could not find a Vulkan-capable GPU!");
    }

    return physDev;
}

VkResult WindowBase::createLogicalDevice()
{
    queueFamilyIndex = QueueFamilies(dev, surface);
    if (not queueFamilyIndex.suitable())
    {
        throw std::runtime_error("Cannot create device queue on GPU.");
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateList;

    float priority = 1.f;
    VkDeviceQueueCreateInfo createQueueInfo = {};
    createQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    createQueueInfo.queueFamilyIndex = queueFamilyIndex.graphicsFamily.value();
    createQueueInfo.queueCount = 1;
    createQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo presentationQueueInfo = {};
    presentationQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentationQueueInfo.queueFamilyIndex = queueFamilyIndex.presentationFamily.value();
    presentationQueueInfo.queueCount = 1;
    presentationQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo transferQueueInfo = {};
    transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transferQueueInfo.queueFamilyIndex = queueFamilyIndex.transferQueueFamily();
    transferQueueInfo.queueCount = 1;
    transferQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo queues[] = {createQueueInfo, presentationQueueInfo, transferQueueInfo};

    VkPhysicalDeviceFeatures feat = {};
    feat.samplerAnisotropy = VK_TRUE;

    auto deviceExts = getRequiredDeviceExts();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 3;
    createInfo.pQueueCreateInfos = queues;
    createInfo.pEnabledFeatures = &feat;
    createInfo.enabledExtensionCount = CAST_UINT32(deviceExts.size());
    createInfo.ppEnabledExtensionNames = deviceExts.data();

    VkResult result = vkCreateDevice(dev, &createInfo, nullptr, &logicalDev);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.presentationFamily.value(), 0, &presentQueue);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.transferQueueFamily(), 0, &transferQueue);

    return result;
}

VkResult WindowBase::createSurface()
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

VkResult WindowBase::createAllocator()
{
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.vulkanApiVersion = VK_API_VERSION;
    createInfo.physicalDevice = dev;
    createInfo.device = logicalDev;
    createInfo.instance = instance;

    return vmaCreateAllocator(&createInfo, &allocator);
}

std::pair<uint32_t, uint32_t> WindowBase::size() const
{
    return {CAST_UINT32(width), CAST_UINT32(height)};
}
