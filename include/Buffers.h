//
// Created by Supakorn on 9/11/2021.
//

#pragma once
#include "common.h"
#include "Vertex.h"

namespace Buffers
{
    typedef std::optional<std::set<uint32_t>> optUint32Set;
    uint32_t getMemoryType(
            VkPhysicalDevice const& physicalDev, VkMemoryPropertyFlags properties,
            uint32_t filter = 0xFFFFFFFF);

    class Buffer : public AVkGraphicsBase
    {
    public:
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        Buffer() = default;
        Buffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev, size_t const& bufferSize,
                VkBufferUsageFlags const& bufferUsageFlags,
                VmaMemoryUsage const& memoryUsage,
                VkMemoryPropertyFlags const& memoryFlags,
                optUint32Set const& usedQueues = nullopt);

        Buffer(Buffer const&) = delete;
        Buffer& operator= (Buffer const&) = delete;

        Buffer(Buffer&& buf) noexcept;
        Buffer& operator=(Buffer&& buf) noexcept;

        virtual ~Buffer();

        [[nodiscard]]
        uint32_t getSize() const;

        VkResult loadData(void const* data);
        VkResult loadData(void const* data, uint32_t const& offset, uint32_t const& dataSize);

        /**
         * @param data vectors of <src, offset, size>
         * @return vkMapMemory success or not
         */
        VkResult loadData(std::vector<std::tuple<void const*, size_t, size_t>> const& data);

        void copyDataFrom(VkBuffer const& src, VkQueue& transferQueue, VkCommandPool& transferCmdPool) const;
        void cmdCopyDataFrom(VkBuffer const& src, VkCommandBuffer& transferBuffer) const;

        void copyDataFrom(Buffer const& src, VkQueue& transferQueue, VkCommandPool& transferCmdPool) const;
        void cmdCopyDataFrom(Buffer const& src, VkCommandBuffer& transferBuffer) const;

    protected:
        VkResult createVertexBuffer(
                VkBufferUsageFlags const& bufferUsageFlags,
                VmaMemoryUsage const& memoryUsage,
                VkMemoryPropertyFlags const& memoryFlags);
        VkResult createVertexBufferConcurrent(
                VkBufferUsageFlags const& bufferUsageFlags,
                VmaMemoryUsage const& memoryUsage,
                VkMemoryPropertyFlags const& memoryFlags,
                std::set<uint32_t> const& queues);

    private:
        VmaAllocator* allocator = nullptr;
        size_t size = -1;
        void* mappedMemory = nullptr;
    };

    class StagingBuffer : public Buffer
    {
    public:
        StagingBuffer() = default;
        StagingBuffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev, size_t const& bufferSize,
                VkMemoryPropertyFlags const& memoryFlags,
                optUint32Set const& usedQueues);

        StagingBuffer(StagingBuffer const&) = delete;
        StagingBuffer& operator= (StagingBuffer const&) = delete;

        StagingBuffer(StagingBuffer&& buf) noexcept;
        StagingBuffer& operator=(StagingBuffer&& buf) noexcept;
    };
} // namespace buffers