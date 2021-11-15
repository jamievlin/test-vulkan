//
// Created by Supakorn on 9/8/2021.
//

#pragma once
#include "common.h"
#include "QueueFamilies.h"
#include "SwapchainComponent.h"
#include "Shaders.h"
#include "UniformObjects.h"
#include "Image.h"

class ShadowmapPipeline : public AVkGraphicsBase
{
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkFramebuffer shadowmapFramebuffer;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::unique_ptr<Image::Image> depthTarget;
    // std::unique_ptr<Image::Image> dummyColorTarget;
    VkDescriptorSetLayout descSetLayout;

    // descriptor sets
    VkDescriptorPool lightDirDescPool;
    std::vector<VkDescriptorSet> lightDirSets;
    std::vector<UniformObjBuffer<glm::mat4>> lightDirUnifBuffer;
    std::vector<VkCommandBuffer> smapCmdBuffer;


    ShadowmapPipeline() = default;
    ShadowmapPipeline(
            VkDevice* device,
            VmaAllocator* allocator,
            VkPhysicalDevice const& physDev,
            std::string const& vertShader,
            uint32_t const& shadowResolution,
            size_t const& swpchainImgCount,
            std::vector<VkDescriptorSetLayout>& descSetLayouts,
            VkCommandPool& cmdPool);

    DISALLOW_COPY(ShadowmapPipeline)

    ShadowmapPipeline(ShadowmapPipeline&& graphicspipeline) noexcept;
    ShadowmapPipeline& operator= (ShadowmapPipeline&& graphicspipeline) noexcept;

    ~ShadowmapPipeline() override;

    // void configureBuffers(
    //         std::vector<VkDescriptorSet>& meshUnifDescSets,
    //        DynUniformObjBuffer<MeshUniform> const& unif);

protected:
    void createGraphicsPipeline(
            std::string const& vertShaderName,
            uint32_t const& shadowResolution,
            std::vector<VkDescriptorSetLayout>& descSetLayouts);

    VkRenderPass createRenderPass(VkPhysicalDevice const& physDev);
    static VkSamplerCreateInfo createSamplerInfo();
    VkFramebuffer createFrameBuffer(
            uint32_t const& shadowResolution,
            // VkImageView& colImgView,
            VkImageView& imgView);

    // void createDescSet(uint32_t imageSize);
    // VkDescriptorSetLayout createDescSetLayout();

private:
    VkCommandPool& cmdPool;
};

