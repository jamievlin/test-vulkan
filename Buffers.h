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
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;

        Buffer() = default;
        Buffer(
                VkDevice* dev, size_t const& bufferSize, VkPhysicalDevice const& physicalDev,
                VkBufferUsageFlags const& bufferUsageFlags, optUint32Set const& usedQueues = nullopt);

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

    protected:
        VkResult createVertexBuffer(VkBufferUsageFlags const& bufferUsageFlags);
        VkResult createVertexBufferConcurrent(
                VkBufferUsageFlags const& bufferUsageFlags,
                std::set<uint32_t> const& queues);
        VkResult allocateMemory(VkPhysicalDevice const& physicalDev);

        [[nodiscard]]
        VkDevice* getLogicalDev();

    private:
        VkDevice* logicalDev = nullptr;
        size_t size = -1;
    };
} // namespace buffers