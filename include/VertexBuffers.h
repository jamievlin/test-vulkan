//
// Created by Supakorn on 9/15/2021.
//

#pragma once
#include "common.h"
#include "Vertex.h"
#include "Buffers.h"

namespace Buffers
{
    template<typename TVertex=Vertex>
    class VertexBuffer : public Buffer
    {
    public:
        VertexBuffer() = default;
        ~VertexBuffer() override = default;

        VertexBuffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev,
                std::vector<TVertex> const& vertices,
                VkFlags const& additionalFlags = 0,
                VkMemoryPropertyFlags const& memoryFlags = 0,
                optUint32Set const& usedQueues = nullopt
        ) : Buffer(dev, allocator, physicalDev,
                   vertices.size() * sizeof(TVertex),
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalFlags,
                   VMA_MEMORY_USAGE_GPU_ONLY,
                   memoryFlags,
                   usedQueues)
        {
            // Memory requirements
            CHECK_VK_SUCCESS(loadData(vertices.data()), "Cannot copy data!");
        }

        VertexBuffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev,
                size_t vertexBufferLength,
                VkFlags const& additionalFlags = 0,
                VkMemoryPropertyFlags const& memoryFlags = 0,
                optUint32Set const& usedQueues = nullopt

        ) : Buffer(dev, allocator, physicalDev,
                   vertexBufferLength * sizeof(TVertex),
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalFlags,
                   VMA_MEMORY_USAGE_GPU_ONLY,
                   memoryFlags,
                   usedQueues)
        {
        }

        VertexBuffer(VertexBuffer const&) = delete;
        VertexBuffer& operator=(VertexBuffer const&) = delete;
    };

    class IndexBuffer : public Buffer
    {
    public:
        IndexBuffer() = default;
        ~IndexBuffer() override = default;

        IndexBuffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev,
                std::vector<uint32_t> const& indices,
                VkFlags const& additionalFlags = 0,
                VkMemoryPropertyFlags const& memoryFlags = 0,
                optUint32Set const& usedQueues = nullopt
        ) : Buffer(dev, allocator, physicalDev,
                   indices.size() * sizeof(uint32_t),
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalFlags,
                   VMA_MEMORY_USAGE_GPU_ONLY,
                   memoryFlags,
                   usedQueues)
        {
            // Memory requirements
            CHECK_VK_SUCCESS(loadData(indices.data()), "Cannot copy data!");
        }

        IndexBuffer(
                VkDevice* dev,
                VmaAllocator* allocator,
                VkPhysicalDevice const& physicalDev,
                size_t vertexBufferLength,
                VkFlags const& additionalFlags = 0,
                VkMemoryPropertyFlags const& memoryFlags = 0,
                optUint32Set const& usedQueues = nullopt

        ) : Buffer(dev, allocator, physicalDev,
                   vertexBufferLength * sizeof(uint32_t),
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalFlags,
                   VMA_MEMORY_USAGE_GPU_ONLY,
                   memoryFlags,
                   usedQueues)
        {
        }

        IndexBuffer(IndexBuffer const&) = delete;
        IndexBuffer& operator=(IndexBuffer const&) = delete;
    };
}