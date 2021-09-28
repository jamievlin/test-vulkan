//
// Created by Supakorn on 9/25/2021.
//

#pragma once
#include <utility>

#include "common.h"
#include "Buffers.h"

namespace Image
{
    constexpr size_t depthFormatCount = 3;
    constexpr VkFormat depthFormats[depthFormatCount] = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT };

    VkFormat findSuitableFormat(
            VkPhysicalDevice const& dev,
            std::vector<VkFormat> const& formats,
            VkImageTiling const& tilingMode,
            VkFormatFeatureFlags const& flags);

    VkFormat findDepthFormat(VkPhysicalDevice const& dev);
    bool hasStencilComponent(VkFormat const& fmt);

    typedef std::optional<std::set<uint32_t>> optUint32Set;

    class Image : public AVkGraphicsBase
    {
    public:
        VkImage img = VK_NULL_HANDLE;
        VkImageView imgView = VK_NULL_HANDLE;
        VkSampler baseSampler = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        Image() = default;
        Image(
                VkDevice* logicalDev, VmaAllocator* allocator, std::pair<uint32_t, uint32_t> const& size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                std::optional<VkSamplerCreateInfo> const& samplerCreateInfo,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                VkImageAspectFlags const& aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
        );
        Image(
                VkDevice* logicalDev, VmaAllocator* allocator, std::tuple<uint32_t, uint32_t, uint32_t> size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                std::optional<VkSamplerCreateInfo> const& samplerCreateInfo,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                VkImageAspectFlags const& aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
        );

        VkResult createSampler(VkSamplerCreateInfo const& samplerCreateInfo);

        VkResult createImage(
                VkFormat const& imgFormat,
                VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                std::optional<std::set<uint32_t>> const& queues,
                VkImageType const& imgType = VK_IMAGE_TYPE_2D,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                VmaMemoryUsage const& memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY);

        Image(Image const&) = delete;
        Image& operator=(Image const&) = delete;

        Image(Image&& im) noexcept;
        Image& operator=(Image&& im) noexcept;

        void cmdTransitionBeginCopy(VkCommandBuffer& cmdBuffer) const;
        void cmdTransitionEndCopy(VkCommandBuffer& cmdBuffer) const;

        VkResult createBaseImageView(
                VkFormat const& format,
                VkImageViewType const& viewType,
                VkImageAspectFlags const& aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

        void cmdTransitionLayout(
                VkImageLayout const& oldLayout,
                VkImageLayout const& newLayout,
                VkPipelineStageFlags const& srcStage,
                VkPipelineStageFlags const& dstStage,
                VkAccessFlags const& srcAccessMask,
                VkAccessFlags const& dstAccessMask,
                VkCommandBuffer& cmdBuffer,
                VkImageAspectFlags const& aspectFlags=VK_IMAGE_ASPECT_COLOR_BIT) const;

        void cmdCopyFromBuffer(
                Buffers::Buffer const& srcBuffer,
                VkImageLayout const& layout,
                VkCommandBuffer& cmdBuffer);

        static VkDescriptorSetLayoutBinding layoutBinding(uint32_t binding)
        {
            VkDescriptorSetLayoutBinding bindingData = {};

            bindingData.binding = binding;
            bindingData.descriptorCount = 1;
            bindingData.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindingData.pImmutableSamplers = nullptr;
            bindingData.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            return bindingData;
        }

        void dispose()
        {
            if (initialized())
            {
                vkDestroySampler(getLogicalDev(), baseSampler, nullptr);
                vkDestroyImageView(getLogicalDev(), imgView, nullptr);
                vmaDestroyImage(*allocator, img, allocation);
            }
        }

        ~Image() override;

    private:
        VmaAllocator* allocator = VK_NULL_HANDLE;
        std::tuple<uint32_t, uint32_t, uint32_t> size = {0,0,0};
    };

}