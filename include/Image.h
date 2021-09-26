//
// Created by Supakorn on 9/25/2021.
//

#pragma once
#include <utility>

#include "common.h"
#include "Buffers.h"

namespace Image
{
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
                VkDevice* logicalDev, VmaAllocator* allocator,
                std::pair<uint32_t, uint32_t> const& size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                VkSamplerCreateInfo const& samplerCreateInfo,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        ) :
                AVkGraphicsBase(logicalDev), allocator(allocator),
                size(size.first, size.second, 1)
        {
            auto const&[width, height] = size;
            CHECK_VK_SUCCESS(
                    createImage(
                            imgFormat, usage, memoryFlags,
                            queueTransfers, VK_IMAGE_TYPE_2D,
                            tiling, mipLevels, initialLayout),
                            ErrorMessages::FAILED_CANNOT_CREATE_IMAGE);

            CHECK_VK_SUCCESS(
                    createBaseImageView(imgFormat, VK_IMAGE_VIEW_TYPE_2D),
                    ErrorMessages::FAILED_CANNOT_CREATE_IMAGE_VIEW)

            CHECK_VK_SUCCESS(
                    createSampler(samplerCreateInfo),
                    ErrorMessages::FAILED_CANNOT_CREATE_SAMPLER
            );
        }
        Image(
                VkDevice* logicalDev, VmaAllocator* allocator,
                std::tuple<uint32_t, uint32_t, uint32_t> size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                VkSamplerCreateInfo const& samplerCreateInfo,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        ) : AVkGraphicsBase(logicalDev), allocator(allocator), size(std::move(size))
        {
            CHECK_VK_SUCCESS(
                    createImage(
                            imgFormat, usage, memoryFlags,
                            queueTransfers, VK_IMAGE_TYPE_3D,
                            tiling, mipLevels, initialLayout),
                            ErrorMessages::FAILED_CANNOT_CREATE_IMAGE);

            CHECK_VK_SUCCESS(
                    createBaseImageView(imgFormat, VK_IMAGE_VIEW_TYPE_3D),
                    ErrorMessages::FAILED_CANNOT_CREATE_IMAGE_VIEW);

            CHECK_VK_SUCCESS(
                    createSampler(samplerCreateInfo),
                    ErrorMessages::FAILED_CANNOT_CREATE_SAMPLER
                    );
        }

        VkResult createSampler(VkSamplerCreateInfo const& samplerCreateInfo)
        {
            return vkCreateSampler(getLogicalDev(), &samplerCreateInfo, nullptr, &baseSampler);
        }

        VkResult createImage(
                VkFormat const& imgFormat,
                VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                std::optional<std::set<uint32_t>> const& queues,
                VkImageType const& imgType = VK_IMAGE_TYPE_2D,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                VmaMemoryUsage const& memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY)
        {
            auto const&[width, height, depth] = size;
            VkImageCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            createInfo.imageType = imgType;
            createInfo.extent.width = width;
            createInfo.extent.height = height;
            createInfo.extent.depth = depth;

            createInfo.mipLevels = mipLevels;
            createInfo.arrayLayers = 1;

            createInfo.format = imgFormat;
            createInfo.tiling = tiling;
            createInfo.initialLayout = initialLayout;

            createInfo.usage = usage;

            if (queues.has_value())
            {
                createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
                auto& queueValue = queues.value();
                std::vector<uint32_t> queueVec(queueValue.size());
                queueVec.assign(queueValue.begin(), queueValue.end());
                createInfo.pQueueFamilyIndices = queueVec.data();
            }
            else
            {
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.pQueueFamilyIndices = nullptr;
            }

            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            createInfo.flags = 0;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryUsage;
            allocCreateInfo.requiredFlags = memoryFlags;

            return vmaCreateImage(*allocator, &createInfo, &allocCreateInfo, &img, &allocation, nullptr);
        }

        Image(Image const&) = delete;
        Image& operator=(Image const&) = delete;

        Image(Image&& im) noexcept :
            AVkGraphicsBase(std::move(im)),
            img(std::move(im.img)),
            imgView(std::move(im.imgView)),
            baseSampler(std::move(im.baseSampler)),
            allocation(std::move(im.allocation)),
            allocator(std::move(im.allocator)),
            size(std::move(im.size))
        {

        }

        Image& operator=(Image&& im) noexcept
        {
            AVkGraphicsBase::operator=(std::move(im));

            img = std::move(im.img);
            imgView = std::move(im.imgView);
            baseSampler = std::move(im.baseSampler);
            allocation = std::move(im.allocation);
            allocator = std::move(im.allocator);
            size = std::move(im.size);

            return *this;
        }

        void cmdTransitionBeginCopy(VkCommandBuffer& cmdBuffer) const
        {
            cmdTransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, VK_ACCESS_TRANSFER_WRITE_BIT, cmdBuffer);
        }

        void cmdTransitionEndCopy(VkCommandBuffer& cmdBuffer) const
        {
            cmdTransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT, 0, cmdBuffer);
        }

        VkResult createBaseImageView(
                VkFormat const& format,
                VkImageViewType const& viewType)
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = img;

            createInfo.viewType = viewType;
            createInfo.format = format;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            return vkCreateImageView(getLogicalDev(), &createInfo, nullptr, &imgView);

        }

        void cmdTransitionLayout(
                VkImageLayout const& oldLayout,
                VkImageLayout const& newLayout,
                VkPipelineStageFlags const& srcStage,
                VkPipelineStageFlags const& dstStage,
                VkAccessFlags const& srcAccessMask,
                VkAccessFlags const& dstAccessMask,
                VkCommandBuffer& cmdBuffer) const
        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = img;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;

            vkCmdPipelineBarrier(
                    cmdBuffer, srcStage, dstStage,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
        }

        void cmdCopyFromBuffer(
                Buffers::Buffer const& srcBuffer,
                VkImageLayout const& layout,
                VkCommandBuffer& cmdBuffer)
        {
            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;

            copyRegion.imageOffset = {0, 0, 0};

            auto const& [width, height, depth] = size;
            copyRegion.imageExtent = {width, height, depth};

            vkCmdCopyBufferToImage(
                    cmdBuffer,
                    srcBuffer.vertexBuffer,
                    img,
                    layout,
                    1, &copyRegion);
        }

        VkDescriptorSetLayoutBinding layoutBinding(uint32_t binding)
        {
            VkDescriptorSetLayoutBinding bindingData = {};

            bindingData.binding = binding;
            bindingData.descriptorCount = 1;
            bindingData.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindingData.pImmutableSamplers = nullptr;
            bindingData.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            return bindingData;
        }

        ~Image()
        {
            if (initialized())
            {
                vkDestroySampler(getLogicalDev(), baseSampler, nullptr);
                vkDestroyImageView(getLogicalDev(), imgView, nullptr);
                vmaDestroyImage(*allocator, img, allocation);
            }
        }

    private:
        VmaAllocator* allocator = VK_NULL_HANDLE;
        std::tuple<uint32_t, uint32_t, uint32_t> size = {0,0,0};
    };

}