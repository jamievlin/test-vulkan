//
// Created by Supakorn on 9/8/2021.
//

#pragma once
#include "common.h"
#include "QueueFamilies.h"
#include "SwapchainComponent.h"
#include "Shaders.h"

class GraphicsPipeline
{
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmdBuffers;

    GraphicsPipeline() = default;
    GraphicsPipeline(
            VkDevice* device,
            VkPhysicalDevice const& physDev,
            VkCommandPool* cmdPool,
            VkSurfaceKHR const& surface,
            std::string const& vertShader,
            std::string const& fragShader,
            SwapchainComponents const& swapChain);

    GraphicsPipeline(GraphicsPipeline const&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline const&) = delete;

    GraphicsPipeline(GraphicsPipeline&& graphicspipeline) noexcept;
    GraphicsPipeline& operator= (GraphicsPipeline&& graphicspipeline) noexcept;

    ~GraphicsPipeline();

protected:
    VkResult createGraphicsPipeline(
            std::string const& vertShaderName,
            std::string const& fragShaderName,
            SwapchainComponents const& swapChain);

    VkResult createCmdBuffers(SwapchainComponents const& swapChain);

private:
    VkDevice* logicalDev=nullptr;
    VkCommandPool* cmdPool;
};