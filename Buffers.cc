//
// Created by Supakorn on 9/11/2021.
//

#include "Buffers.h"

namespace Buffers
{
    uint32_t getMemoryType(VkPhysicalDevice const& physicalDev, VkMemoryPropertyFlags properties, uint32_t filter)
    {
        VkPhysicalDeviceMemoryProperties physMemProperty;
        vkGetPhysicalDeviceMemoryProperties(physicalDev, &physMemProperty);

        uint32_t mask = 1;
        for (uint32_t i = 0; i < physMemProperty.memoryTypeCount; ++i)
        {
            if (
                    (filter & mask) &&
                    ((physMemProperty.memoryTypes[i].propertyFlags & properties) == properties))
            {
                return i;
            }
            mask = mask << 1;
        }

        throw std::runtime_error("Cannot find suitable memory type!");
    }

    Buffer::Buffer(VkDevice* dev, size_t const& bufferSize, VkPhysicalDevice const& physicalDev,
                   VkBufferUsageFlags const& bufferUsageFlags, optUint32Set const& usedQueues
                   ) : logicalDev(dev), size(bufferSize)
    {
        if (usedQueues.has_value() and usedQueues.value().size() > 1)
        {
            CHECK_VK_SUCCESS(createVertexBufferConcurrent(
                    bufferUsageFlags,
                    usedQueues.value()), "Cannot create buffer!");
        }
        else
        {
            CHECK_VK_SUCCESS(createVertexBuffer(bufferUsageFlags), "Cannot create buffer!");
        }
        CHECK_VK_SUCCESS(allocateMemory(physicalDev), "Cannot allocate device memory!");

        CHECK_VK_SUCCESS(vkBindBufferMemory(*getLogicalDev(), vertexBuffer, deviceMemory, 0),
                         "Cannot bind buffer!");
    }

    Buffer::Buffer(Buffer&& buf) noexcept:
            logicalDev(std::move(buf.logicalDev)), size(std::move(buf.size)),
            vertexBuffer(std::move(buf.vertexBuffer)), deviceMemory(std::move(buf.deviceMemory))
    {
        buf.logicalDev = nullptr;
    }

    Buffer& Buffer::operator=(Buffer&& buf) noexcept
    {
        logicalDev = std::move(buf.logicalDev);
        size = std::move(buf.size);
        vertexBuffer = std::move(buf.vertexBuffer);
        deviceMemory = std::move(buf.deviceMemory);

        buf.logicalDev = nullptr;
        return *this;
    }

    Buffer::~Buffer()
    {
        if (isInitialized())
        {
            vkFreeMemory(*logicalDev, deviceMemory, nullptr);
            vkDestroyBuffer(*logicalDev, vertexBuffer, nullptr);
        }
    }

    Buffer::operator bool() const
    {
        return isInitialized();
    }

    bool Buffer::isInitialized() const
    {
        return logicalDev != nullptr;
    }

    uint32_t Buffer::getSize() const
    {
        return static_cast<uint32_t>(size);
    }

    VkResult Buffer::createVertexBuffer(VkBufferUsageFlags const& bufferUsageFlags)
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = bufferUsageFlags;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        return vkCreateBuffer(*logicalDev, &createInfo, nullptr, &vertexBuffer);
    }

    VkResult Buffer::createVertexBufferConcurrent(
            VkBufferUsageFlags const& bufferUsageFlags,
            std::set<uint32_t> const& queues)
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = bufferUsageFlags;
        createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

        std::vector<uint32_t> queueVec(queues.size());
        queueVec.assign(queues.begin(), queues.end());
        createInfo.pQueueFamilyIndices = queueVec.data();

        return vkCreateBuffer(*logicalDev, &createInfo, nullptr, &vertexBuffer);
    }

    VkResult Buffer::allocateMemory(VkPhysicalDevice const& physicalDev)
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

    VkDevice* Buffer::getLogicalDev()
    {
        return logicalDev;
    }
} // namespace Buffers