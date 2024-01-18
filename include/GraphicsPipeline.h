//
// Created by Supakorn on 9/8/2021.
//

#pragma once
#include "common.h"
#include "QueueFamilies.h"
#include "SwapchainComponent.h"
#include "Shaders.h"
#include "UniformObjects.h"

class GraphicsPipeline : public AVkGraphicsBase
{
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmdBuffers;

    GraphicsPipeline() = default;
    GraphicsPipeline(
        VkDevice* device, VkPhysicalDevice const& physDev, VkCommandPool* cmdPool,
        std::string const& vertShader, std::string const& fragShader, VkExtent2D const& extent,
        size_t const& swpchainImgCount, VkRenderPass const& renderPass,
        std::vector<VkDescriptorSetLayout> const& descriptorSetLayout = {},
        bool enableDepthTest = true
    );

    GraphicsPipeline(GraphicsPipeline const&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline const&) = delete;

    GraphicsPipeline(GraphicsPipeline&& graphicspipeline) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& graphicspipeline) noexcept;

    ~GraphicsPipeline();

protected:
    VkResult createGraphicsPipeline(
        std::string const& vertShaderName, std::string const& fragShaderName,
        VkExtent2D const& extent, VkRenderPass const& renderPass,
        std::vector<VkDescriptorSetLayout> const& descriptorSetLayout = {},
        bool enableDepthTest = true
    );

    VkResult createCmdBuffers(size_t const& swpchainImgCoun);

private:
    VkCommandPool* cmdPool = nullptr;
};
