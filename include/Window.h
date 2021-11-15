//
// Created by Supakorn on 9/27/2021.
//

#pragma once
#include "common.h"
#include "WindowBase.h"
#include "Mesh.h"
#include "Drawable.h"
#include "ShadowmapPipeline.h"
#include "DisposableCmdBuffer.h"

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

    void recordCmd(uint32_t imageIdx, VkFence& submissionFence);
    void recordShadowmapCmd(VkCommandBuffer& smapBuf, uint32_t imageIdx, VkFence& submissionFence);
    void drawFrame();
    void resetSwapChain();

    void initBuffers();
    void setUniforms(UniformObjBuffer<UniformObjects>& bufObject);
    void setLights(StorageBufferArray<Light>& storageObj);

    void initCallbacks();
private:
    bool running = true;
    std::unique_ptr<SwapchainComponents> swapchainComponent;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPool cmdTransferPool = VK_NULL_HANDLE;

    std::unique_ptr<SwapchainImageBuffers> uniformData;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    std::unique_ptr<ShadowmapPipeline> shadowmapGraphicsPipeline;

    std::unique_ptr<DynUniformObjBuffer<MeshUniform>> meshUniformGroup;

    // buffers
    std::vector<FrameSemaphores> frameSemaphores;
    size_t currentFrame = 0;
    Image::Image img;
    Image::Image depthBuffer;
    float totalTime = 0;

    float fovDegrees;
    float clipNear, clipFar;
    glm::mat4 projectMat;
    glm::vec3 cameraPos;

    std::map<std::string, std::unique_ptr<Mesh>> meshStorage;
    std::vector<Drawable> drawables;
};