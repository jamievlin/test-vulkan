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

    class Buffer
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
        explicit operator bool() const;

        [[nodiscard]]
        bool isInitialized() const;

        [[nodiscard]]
        uint32_t getSize() const;

        VkResult loadData(void const* data, VkDeviceSize const& offset = 0);
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

        [[nodiscard]]
        VkDevice* getLogicalDev();

    private:
        VkDevice* logicalDev = nullptr;
        VmaAllocator* allocator = nullptr;
        size_t size = -1;
    };
} // namespace buffers