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

    void mainLoop();

protected:
    VkResult initInstance();
    VkDebugUtilsMessengerEXT dbgMessenger;
    VkResult setupDebugMessenger();
    VkResult createLogicalDevice();
    VkResult createSurface();
    VkResult initSwapChain();

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

    VkDebugUtilsMessengerCreateInfoEXT createDebugInfo();
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

};