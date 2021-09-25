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

std::vector<char const*> getRequiredExts();
bool deviceSuitable(VkPhysicalDevice const& dev);
constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

class Window
{
public:
    Window(
            size_t const& width,
            size_t const& height,
            std::string windowTitle,
            float const& fov=60.f, float const& clipnear=0.1f, float const& clipfar=100.f);
    virtual ~Window();

    Window(Window const& win) = delete;
    Window& operator=(Window const& win) = delete;

    int mainLoop();

protected:
    // Callback functions
    static void onWindowSizeChange(GLFWwindow* ptr, int width, int height);

    void initCallbacks();
    VkResult initInstance();
#if defined(ENABLE_VALIDATION_LAYERS)
    VkDebugUtilsMessengerEXT dbgMessenger;
    VkResult setupDebugMessenger();
#endif
    VkResult createLogicalDevice();
    VkResult createSurface();
    VkResult createCommandPool();
    VkResult createTransferCmdPool();

    void recordCommands();
    void drawFrame();
    void resetSwapChain();

    VkResult createAllocator();


    void initBuffers();
    void setUniforms(UniformObjBuffer<UniformObjects>& bufObject);
    virtual void updateFrame(float const& deltaTime);

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


private:
    size_t width, height;
    std::string title;

    float fovDegrees;
    float clipNear, clipFar;
    glm::mat4 projectMat;

    GLFWwindow* window = nullptr;
    VkInstance instance = {};
    VkPhysicalDevice dev = {};
    VkDevice logicalDev = {};

    VmaAllocator allocator = VK_NULL_HANDLE;

    QueueFamilies queueFamilyIndex;
    VkQueue graphicsQueue = {};
    VkQueue presentQueue = {};
    VkQueue transferQueue = {};

    VkSurfaceKHR surface = {};

    std::unique_ptr<SwapchainComponents> swapchainComponent;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPool cmdTransferPool = VK_NULL_HANDLE;

    std::unique_ptr<SwapchainImageBuffers> uniformData;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    // buffers
    std::unique_ptr<Buffers::VertexBuffer<Vertex>> vertexBuffer;
    std::unique_ptr<Buffers::IndexBuffer> idxBuffer;

    std::vector<FrameSemaphores> frameSemaphores;
    size_t currentFrame = 0;

    float totalTime = 0;
};