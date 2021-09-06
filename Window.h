#pragma once

#include "common.h"
#include "SwapChains.h"

#define HEIGHT 768
#define WIDTH 1024
#define TITLE "Vulkan"

std::vector<char const*> getRequiredExts();
bool deviceSuitable(VkPhysicalDevice const& dev);

class Window
{
public:
    Window();
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
    VkResult createFrameBuffers();
    VkResult createCommandPool();
    VkResult createCmdBuffers();
    bool createImageViews();

    void recordCommands();

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
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice dev;
    VkDevice logicalDev;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;

    std::vector<VkImage> swapChainImages;
    VkSurfaceFormatKHR swapchainFormat;
    VkExtent2D swapchainExtent;

    std::vector<VkImageView> swapchainImgViews;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;

    VkPipeline pipeline;
    std::vector<VkFramebuffer> frameBuffers;

    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> cmdBuffers;
};