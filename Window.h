#pragma once

#include "common.h"
#include "SwapChains.h"
#include "FrameSemaphores.h"
#include "SwapchainSupport.h"

std::vector<char const*> getRequiredExts();
bool deviceSuitable(VkPhysicalDevice const& dev);
constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

class Window
{
public:
    Window(size_t const& width, size_t const& height, std::string title);
    ~Window();

    Window(Window const& win) = delete;
    Window& operator=(Window const& win) = delete;

    int mainLoop();

protected:
    VkResult initInstance();
#if defined(ENABLE_VALIDATION_LAYERS)
    VkDebugUtilsMessengerEXT dbgMessenger;
    VkResult setupDebugMessenger();
#endif
    VkResult createLogicalDevice();
    VkResult createSurface();
    VkResult initSwapChain();
    VkResult createRenderPasses();
    VkResult createGraphicsPipeline();
    VkResult createCommandPool();
    VkResult createCmdBuffers();
    void recordCommands();
    void drawFrame();

    // vk extension functions
    template<typename TPtrExt, typename ...T_args>
    std::function<VkResult (T_args...)> getVkExtension(std::string const& extName)
    {
        return getVkExtensionRet<TPtrExt, VkResult, T_args...>(extName, VK_ERROR_EXTENSION_NOT_PRESENT);
    }

    template<typename TPtrExt, typename TRet, typename ...T_args>
    std::function<TRet(T_args...)> getVkExtensionRet(std::string const& extName, TRet const& defaultReturn)
    {
        return [this, extName, defaultReturn](T_args...args) -> TRet
        {
            auto func = (TPtrExt) vkGetInstanceProcAddr(this->instance, extName.c_str());
            if (func != nullptr)
            {
                return func(args...);
            }
            else
            {
                return defaultReturn;
            }
        };
    }

    template<typename TPtrExt, typename ...T_args>
    std::function<void(T_args...)> getVkExtensionVoid(std::string const& extName)
    {
        return [this, extName](T_args...args)
        {
            auto func = (TPtrExt) vkGetInstanceProcAddr(this->instance, extName.c_str());
            if (func != nullptr)
            {
                func(args...);
            }
            return;
        };
    }
#if defined(ENABLE_VALIDATION_LAYERS)
    VkDebugUtilsMessengerCreateInfoEXT createDebugInfo();
#endif
    VkPhysicalDevice selectPhysicalDev();

    void getSwapChainImage();

private:
    size_t width, height;
    std::string title;

    GLFWwindow* window = nullptr;
    VkInstance instance = {};
    VkPhysicalDevice dev = {};
    VkDevice logicalDev = {};

    VkQueue graphicsQueue = {};
    VkQueue presentQueue = {};

    VkSurfaceKHR surface = {};
    VkSwapchainKHR swapChain = {};

    std::vector<VkImage> swapChainImages;
    VkSurfaceFormatKHR swapchainFormat = {};
    VkExtent2D swapchainExtent = {};

    std::vector<SwapchainImageSupport> swapchainSupport;
    VkPipelineLayout pipelineLayout = {};
    VkRenderPass renderPass = {};

    VkPipeline pipeline = {};

    VkCommandPool cmdPool = {};
    std::vector<VkCommandBuffer> cmdBuffers;

    std::vector<FrameSemaphores> frameSemaphores;
    size_t currentFrame = 0;
};