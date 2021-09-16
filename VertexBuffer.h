//
// Created by Supakorn on 9/11/2021.
//

#pragma once
#include "common.h"
#include "Vertex.h"

uint32_t getMemoryType(
        VkPhysicalDevice const& physicalDev, VkMemoryPropertyFlags properties,
        uint32_t filter=0xFFFFFFFF);

template<typename TVertex=Vertex>
class VertexBuffer
{
public:
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;

    VertexBuffer() = default;
    VertexBuffer(
            VkDevice* dev,
            VkPhysicalDevice const& physicalDev,
            std::vector<TVertex> const& vertices,
            std::optional<std::set<uint32_t>> const& usedQueues=nullopt
            ) : logicalDev(dev), size(vertices.size() * sizeof(TVertex))
    {
        if (usedQueues.has_value() and usedQueues.value().size() > 1)
        {
            CHECK_VK_SUCCESS(createVertexBufferConcurrent(usedQueues.value()), "Cannot create Vertex buffer!");
        }
        else
        {
            CHECK_VK_SUCCESS(createVertexBuffer(), "Cannot create Vertex buffer!");
        }

        CHECK_VK_SUCCESS(allocateMemory(physicalDev), "Cannot allocate device memory!");

        CHECK_VK_SUCCESS(vkBindBufferMemory(*logicalDev, vertexBuffer, deviceMemory, 0), "Cannot bind buffer!");
        // Memory requirements
        CHECK_VK_SUCCESS(copyData(vertices), "Cannot copy data!");
    }

    VertexBuffer(VertexBuffer const&) = delete;
    VertexBuffer& operator=(VertexBuffer const&) = delete;

    VertexBuffer(VertexBuffer&& buffer) noexcept :
        logicalDev(std::move(buffer.logicalDev)), size(std::move(buffer.size)),
        vertexBuffer(std::move(buffer.vertexBuffer)), deviceMemory(std::move(buffer.deviceMemory))
    {
        buffer.logicalDev = nullptr;
    }

    VertexBuffer& operator=(VertexBuffer&& buffer) noexcept
    {
        logicalDev = std::move(buffer.logicalDev);
        size = std::move(buffer.size);
        vertexBuffer = std::move(buffer.vertexBuffer);
        deviceMemory = std::move(buffer.deviceMemory);

        buffer.logicalDev = nullptr;
    }

    [[nodiscard]]
    uint32_t getSize() const
    {
        return static_cast<uint32_t>(size);
    }


    VkResult createVertexBuffer()
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        return vkCreateBuffer(*logicalDev, &createInfo, nullptr, &vertexBuffer);
    }

    VkResult createVertexBufferConcurrent(std::set<uint32_t> const& queues)
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

        std::vector<uint32_t> queueVec(queues.size());
        queueVec.assign(queues.begin(), queues.end());
        createInfo.pQueueFamilyIndices = queueVec.data();

        return vkCreateBuffer(*logicalDev, &createInfo, nullptr, &vertexBuffer);
    }

    VkResult allocateMemory(VkPhysicalDevice const& physicalDev)
    {
        VkMemoryRequirements memReq = {};
        vkGetBufferMemoryRequirements(*logicalDev, vertexBuffer, &memReq);

        VkMemoryAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memReq.size;
        allocateInfo.memoryTypeIndex =
                getMemoryType(physicalDev,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              memReq.memoryTypeBits);

        return vkAllocateMemory(*logicalDev, &allocateInfo, nullptr, &deviceMemory);
    }

    VkResult copyData(std::vector<TVertex> const& data)
    {
        void* dat = nullptr;
        auto ret = vkMapMemory(*logicalDev, deviceMemory, 0, sizeof(TVertex) * data.size(), 0, &dat);
        memcpy(dat, data.data(), size);

        vkUnmapMemory(*logicalDev, deviceMemory);

        return ret;
    }

    ~VertexBuffer()
    {
        if (logicalDev)
        {
            vkFreeMemory(*logicalDev, deviceMemory, nullptr);
            vkDestroyBuffer(*logicalDev, vertexBuffer, nullptr);
        }
    }
private:
    size_t size = 0;
    VkDevice* logicalDev = nullptr;
};