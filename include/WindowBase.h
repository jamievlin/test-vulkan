#pragma once

#include "common.h"
#include "SwapChains.h"
#include "SwapchainComponent.h"
#include "GraphicsPipeline.h"
#include "FrameSemaphores.h"
#include "Vertex.h"
#include "Buffers.h"
#include "VertexBuffers.h"
#include "SwapchainImgBuffers.h"
#include "Image.h"

std::vector<char const*> getRequiredExts();
bool deviceSuitable(VkPhysicalDevice const& dev);

class WindowBase
{
public:
    WindowBase(size_t const& width, size_t const& height, std::string windowTitle);
    virtual ~WindowBase();

    WindowBase(WindowBase const& win) = delete;
    WindowBase& operator=(WindowBase const& win) = delete;

    [[nodiscard]]
    std::pair<uint32_t, uint32_t> size() const;

protected:
    VkResult initInstance();
#if defined(ENABLE_VALIDATION_LAYERS)
    VkDebugUtilsMessengerEXT dbgMessenger;
    VkResult setupDebugMessenger();
#endif
    VkResult createLogicalDevice();
    VkResult createAllocator();
    VkResult createSurface();

    // vk extension functions
    template <typename TPtrExt, typename... T_args>
    std::function<VkResult(T_args...)> getVkExtension(std::string const& extName)
    {
        return getVkExtensionRet<TPtrExt, VkResult, T_args...>(
            extName, VK_ERROR_EXTENSION_NOT_PRESENT
        );
    }

    template <typename TPtrExt, typename TRet, typename... T_args>
    std::function<TRet(T_args...)> getVkExtensionRet(
        std::string const& extName, TRet const& defaultReturn
    )
    {
        return [this, extName, defaultReturn](T_args... args) -> TRet
        {
            auto func = (TPtrExt)vkGetInstanceProcAddr(this->instance, extName.c_str());
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

    template <typename TPtrExt, typename... T_args>
    std::function<void(T_args...)> getVkExtensionVoid(std::string const& extName)
    {
        return [this, extName](T_args... args)
        {
            auto func = (TPtrExt)vkGetInstanceProcAddr(this->instance, extName.c_str());
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

protected:
    QueueFamilies queueFamilyIndex;
    VkQueue graphicsQueue = {};
    VkQueue presentQueue = {};
    VkQueue transferQueue = {};

    size_t width, height;
    std::string title;

    GLFWwindow* window = nullptr;
    VkInstance instance = {};
    VkPhysicalDevice dev = {};
    VkDevice logicalDev = {};
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};
