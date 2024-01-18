//
// Created by Supakorn on 9/11/2021.
//

#include "Buffers.h"

namespace Buffers
{
uint32_t getMemoryType(
    VkPhysicalDevice const& physicalDev, VkMemoryPropertyFlags properties, uint32_t filter
)
{
    VkPhysicalDeviceMemoryProperties physMemProperty;
    vkGetPhysicalDeviceMemoryProperties(physicalDev, &physMemProperty);

    uint32_t mask = 1;
    for (uint32_t i = 0; i < physMemProperty.memoryTypeCount; ++i)
    {
        if ((filter & mask)
            && ((physMemProperty.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
        mask = mask << 1;
    }

    throw std::runtime_error("Cannot find suitable memory type!");
}

Buffer::Buffer(
    VkDevice* dev, VmaAllocator* allocator, VkPhysicalDevice const& physicalDev,
    size_t const& bufferSize, VkBufferUsageFlags const& bufferUsageFlags,
    VmaMemoryUsage const& memoryUsage, VkMemoryPropertyFlags const& memoryFlags,
    optUint32Set const& usedQueues
)
    : AVkGraphicsBase(dev), allocator(allocator), size(bufferSize)
{
    if (usedQueues.has_value() and usedQueues.value().size() > 1)
    {
        CHECK_VK_SUCCESS(
            createVertexBufferConcurrent(
                bufferUsageFlags, memoryUsage, memoryFlags, usedQueues.value()
            ),
            "Cannot create buffer!"
        );
    }
    else
    {
        CHECK_VK_SUCCESS(
            createVertexBuffer(bufferUsageFlags, memoryUsage, memoryFlags), "Cannot create buffer!"
        );
    }
}

Buffer::Buffer(Buffer&& buf) noexcept
    : AVkGraphicsBase(std::move(buf)), allocator(std::move(buf.allocator)),
      size(std::move(buf.size)), vertexBuffer(std::move(buf.vertexBuffer)),
      allocation(std::move(buf.allocation))
{
}

Buffer& Buffer::operator=(Buffer&& buf) noexcept
{
    AVkGraphicsBase::operator=(std::move(buf));

    allocator = std::move(buf.allocator);
    size = std::move(buf.size);
    vertexBuffer = std::move(buf.vertexBuffer);
    allocation = std::move(buf.allocation);

    return *this;
}

Buffer::~Buffer()
{
    if (initialized())
    {
        if (mappedMemory)
        {
            vmaUnmapMemory(*allocator, allocation);
        }
        vmaDestroyBuffer(*allocator, vertexBuffer, allocation);
    }
}

uint32_t Buffer::getSize() const
{
    return CAST_UINT32(size);
}

VkResult Buffer::createVertexBuffer(
    VkBufferUsageFlags const& bufferUsageFlags, VmaMemoryUsage const& memoryUsage,
    VkMemoryPropertyFlags const& memoryFlags
)
{
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = bufferUsageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.requiredFlags = memoryFlags;

    return vmaCreateBuffer(
        *allocator, &createInfo, &allocInfo, &vertexBuffer, &allocation, nullptr
    );
}

VkResult Buffer::createVertexBufferConcurrent(
    VkBufferUsageFlags const& bufferUsageFlags, VmaMemoryUsage const& memoryUsage,
    VkMemoryPropertyFlags const& memoryFlags, std::set<uint32_t> const& queues
)
{
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = bufferUsageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = CAST_UINT32(queues.size());

    std::vector<uint32_t> queueVec(queues.size());
    queueVec.assign(queues.begin(), queues.end());
    createInfo.pQueueFamilyIndices = queueVec.data();

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.requiredFlags = memoryFlags;

    return vmaCreateBuffer(
        *allocator, &createInfo, &allocInfo, &vertexBuffer, &allocation, nullptr
    );
}

VkResult Buffer::loadData(void const* data)
{
    return loadData(data, 0, size);
}

VkResult Buffer::loadData(void const* data, uint32_t const& offset, uint32_t const& dataSize)
{
    if (mappedMemory == nullptr)
    {
        CHECK_VK_SUCCESS(
            vmaMapMemory(*allocator, allocation, &mappedMemory), "Cannot map buffer memory!"
        );
    }
    memcpy(reinterpret_cast<uint8_t*>(mappedMemory) + offset, data, dataSize);

    return VK_SUCCESS;
}

void Buffer::cmdCopyDataFrom(VkBuffer const& src, VkCommandBuffer& transferBuffer) const
{
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(transferBuffer, src, vertexBuffer, 1, &copyRegion);
}

void Buffer::copyDataFrom(
    VkBuffer const& src, VkQueue& transferQueue, VkCommandPool& transferCmdPool
) const
{
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = transferCmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        // 1 command buffer per frame buffer
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    CHECK_VK_SUCCESS(
        vkAllocateCommandBuffers(getLogicalDev(), &allocateInfo, &cmdBuffer),
        ErrorMessages::FAILED_CANNOT_CREATE_CMD_BUFFER
    );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VK_SUCCESS(
        vkBeginCommandBuffer(cmdBuffer, &beginInfo), ErrorMessages::FAILED_CANNOT_BEGIN_CMD_BUFFER
    );
    cmdCopyDataFrom(src, cmdBuffer);
    CHECK_VK_SUCCESS(vkEndCommandBuffer(cmdBuffer), ErrorMessages::FAILED_CANNOT_END_CMD_BUFFER);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

        // not exactly the best idea. One cmd per copy data is suboptimal.
    CHECK_VK_SUCCESS(
        vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE),
        ErrorMessages::FAILED_CANNOT_SUBMIT_QUEUE
    );

    CHECK_VK_SUCCESS(vkQueueWaitIdle(transferQueue), ErrorMessages::FAILED_WAIT_IDLE);
    vkFreeCommandBuffers(getLogicalDev(), transferCmdPool, 1, &cmdBuffer);
}

void Buffer::copyDataFrom(Buffer const& src, VkQueue& transferQueue, VkCommandPool& transferCmdPool)
    const
{
    copyDataFrom(src.vertexBuffer, transferQueue, transferCmdPool);
}

void Buffer::cmdCopyDataFrom(Buffer const& src, VkCommandBuffer& transferBuffer) const
{
    cmdCopyDataFrom(src.vertexBuffer, transferBuffer);
}

VkResult Buffer::loadData(std::vector<std::tuple<void const*, size_t, size_t>> const& data)
{
    if (mappedMemory == nullptr)
    {
        CHECK_VK_SUCCESS(
            vmaMapMemory(*allocator, allocation, &mappedMemory), "Cannot map buffer memory!"
        );
    }
    for (auto const& [src, offset, sz] : data)
    {
        memcpy(reinterpret_cast<uint8_t*>(mappedMemory) + offset, src, sz);
    }
    return VK_SUCCESS;
}

StagingBuffer::StagingBuffer(
    VkDevice* dev, VmaAllocator* allocator, VkPhysicalDevice const& physicalDev,
    size_t const& bufferSize, VkMemoryPropertyFlags const& memoryFlags,
    optUint32Set const& usedQueues
)
    : Buffer(
          dev, allocator, physicalDev, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VMA_MEMORY_USAGE_CPU_TO_GPU, memoryFlags, usedQueues
      )
{
}

StagingBuffer& StagingBuffer::operator=(StagingBuffer&& buf) noexcept
{
    Buffer::operator=(std::move(buf));
    return *this;
}

StagingBuffer::StagingBuffer(StagingBuffer&& buf) noexcept : Buffer(std::move(buf))
{
}
} // namespace Buffers
