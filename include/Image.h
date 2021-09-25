//
// Created by Supakorn on 9/25/2021.
//

#pragma once
#include "common.h"

namespace Image
{
    typedef std::optional<std::set<uint32_t>> optUint32Set;
    class Image : public AVkGraphicsBase
    {
    public:
        VkImage img = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        Image() = default;
        Image(
                VkDevice* logicalDev, VmaAllocator* allocator,
                std::pair<uint32_t, uint32_t> const& size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        ) :
                AVkGraphicsBase(logicalDev), allocator(allocator)
        {
            auto const&[width, height] = size;
            CHECK_VK_SUCCESS(createImage(
                    {width, height, 1}, imgFormat, usage, memoryFlags,
                    queueTransfers, VK_IMAGE_TYPE_2D,
                    tiling, mipLevels, initialLayout),
                             ErrorMessages::FAILED_CANNOT_CREATE_IMAGE);
        }
        Image(
                VkDevice* logicalDev, VmaAllocator* allocator,
                std::tuple<uint32_t, uint32_t, uint32_t> const& size,
                VkFormat const& imgFormat, VkImageUsageFlags const& usage,
                VkMemoryPropertyFlags const& memoryFlags,
                optUint32Set const& queueTransfers = nullopt,
                VkImageTiling const& tiling = VK_IMAGE_TILING_OPTIMAL,
                uint32_t const& mipLevels = 1,
                VkImageLayout const& initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        ) : AVkGraphicsBase(logicalDev), allocator(allocator)
        {
            CHECK_VK_SUCCESS(createImage(
                    size, imgFormat, usage, memoryFlags,
                    queueTransfers, VK_IMAGE_TYPE_3D,
                    tiling, mipLevels, initialLayout),
                             ErrorMessages::FAILED_CANNOT_CREATE_IMAGE);
        }

        VkResult createImage(
                std::tuple<uint32_t, uint32_t, uint32_t> const& size,
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
            allocation(std::move(im.allocation)),
            allocator(std::move(im.allocator))
        {

        }

        Image& operator=(Image&& im) noexcept
        {
            AVkGraphicsBase::operator=(std::move(im));

            img = std::move(im.img);
            allocation = std::move(im.allocation);
            allocator = std::move(im.allocator);

            return *this;
        }

        ~Image()
        {
            if (initialized())
            {

            }
        }

    private:
        VmaAllocator* allocator = VK_NULL_HANDLE;
    };

}