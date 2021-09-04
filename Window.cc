#include "Window.h"
#include "validationTargets.h"
#include "dbgCallBacks.h"
#include "QueueFamilies.h"

#define HEIGHT 768
#define WIDTH 1024
#define TITLE "Vulkan"

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

bool deviceSuitable(VkPhysicalDevice const& dev)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(dev, &properties);
    vkGetPhysicalDeviceFeatures(dev, &features);

    return features.tessellationShader && features.geometryShader;
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
    return VkDebugUtilsMessengerCreateInfoEXT
            {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                    .pfnUserCallback = debugCallback,
                    .pUserData = static_cast<void*>(this)
            };
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
    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Vulkan!",
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "What?",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_2
    };
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

    VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if ENABLE_VALIDATION_LAYERS == 1
        .pNext = &dbgCreateInfo,
#else
        .pNext = nullptr,
#endif
        .pApplicationInfo = &appInfo,
#if ENABLE_VALIDATION_LAYERS == 1
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
#else
        ..enabledLayerCount = 0,
#endif
        .enabledExtensionCount = static_cast<uint32_t>(extNeeded.size()),
        .ppEnabledExtensionNames = extNeeded.data(),
    };
    return vkCreateInstance(&createInfo, nullptr, &instance);
}

VkPhysicalDevice Window::selectPhysicalDev()
{
    VkPhysicalDevice dev = VK_NULL_HANDLE;
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0)
    {
        throw std::runtime_error("There is no GPU capable of Vulkan!");
    }

    std::vector<VkPhysicalDevice> devs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, devs.data());

    for (auto const& devlist : devs)
    {
        QueueFamilies fam(devlist, surface);
        if (deviceSuitable(devlist) && fam.suitable())
        {
            dev = devlist;
            break;
        }
    }

    if (dev == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Could not find a Vulkan-capable GPU!");
    }
    return dev;
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
    VkDeviceQueueCreateInfo createQueueInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFam.graphicsFamily.value(),
        .queueCount = 1,
        .pQueuePriorities = &priority
    };

    VkPhysicalDeviceFeatures feat {
    };

    VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &createQueueInfo,
        .pEnabledFeatures = &feat
    };

    VkResult result = vkCreateDevice(dev, &createInfo, nullptr, &logicalDev);
    vkGetDeviceQueue(logicalDev, queueFam.graphicsFamily.value(), 0, &graphicsQueue);
    return result;
}

VkResult Window::createSurface()
{
#if defined(__linux__)
    return glfwCreateWindowSurface(instance, window, nullptr, &surface);
#else
    throw std::runtime_error("To be done for Windows!");
#endif
}

