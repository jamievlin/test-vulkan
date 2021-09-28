//
// Created by Supakorn on 9/27/2021.
//

#pragma once
#include "common.h"
#include "WindowBase.h"

class Window : public WindowBase
{
public:
    Window(
            size_t const& width,
            size_t const& height,
            std::string windowTitle,
            float const& fov=60.f, float const& clipnear=0.1f, float const& clipfar=100.f);

    ~Window() override;
    int mainLoop();

protected:
    // Callback functions
    static void onWindowSizeChange(GLFWwindow* ptr, int width, int height);

    virtual void updateFrame(float const& deltaTime);
    VkResult createCommandPool();
    VkResult createTransferCmdPool();

    void recordCommands();
    void drawFrame();
    void resetSwapChain();

    void initBuffers();
    void setUniforms(UniformObjBuffer<UniformObjects>& bufObject);

    void initCallbacks();

private:
    std::unique_ptr<SwapchainComponents> swapchainComponent;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPool cmdTransferPool = VK_NULL_HANDLE;

    std::unique_ptr<SwapchainImageBuffers> uniformData;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    // buffers
    Buffers::VertexBuffer<Vertex> vertexBuffer;
    Buffers::IndexBuffer idxBuffer;

    std::vector<FrameSemaphores> frameSemaphores;
    size_t currentFrame = 0;
    Image::Image img;
    float totalTime = 0;

    float fovDegrees;
    float clipNear, clipFar;
    glm::mat4 projectMat;

};