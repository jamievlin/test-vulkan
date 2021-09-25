//
// Created by Supakorn on 9/16/2021.
//

#include "DisposableCmdBuffer.h"

DisposableCmdBuffer::DisposableCmdBuffer(VkDevice* logicalDev, VkCommandPool* pool) :
        AVkGraphicsBase(logicalDev), cmdPool(pool), disposed(false)
{
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = *pool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // 1 command buffer per frame buffer
    allocateInfo.commandBufferCount = 1;

    CHECK_VK_SUCCESS(vkAllocateCommandBuffers(*logicalDev, &allocateInfo, &cmdBuffer),
                     ErrorMessages::FAILED_CANNOT_CREATE_CMD_BUFFER);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VK_SUCCESS(vkBeginCommandBuffer(cmdBuffer, &beginInfo),
                     ErrorMessages::FAILED_CANNOT_BEGIN_CMD_BUFFER);
}

DisposableCmdBuffer::DisposableCmdBuffer(DisposableCmdBuffer&& dcb) noexcept:
        AVkGraphicsBase(std::move(dcb)), cmdPool(std::move(dcb.cmdPool)),
        cmdBuffer(std::move(dcb.cmdBuffer)), disposed(std::move(dcb.disposed))
{
}

DisposableCmdBuffer& DisposableCmdBuffer::operator=(DisposableCmdBuffer&& dcb) noexcept
{
    AVkGraphicsBase::operator=(std::move(dcb));
    cmdPool = std::move(dcb.cmdPool);
    cmdBuffer = std::move(dcb.cmdBuffer);
    disposed = std::move(dcb.disposed);

    return *this;
}

VkResult DisposableCmdBuffer::submit(VkQueue& queue)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    return vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void DisposableCmdBuffer::finish()
{
    if (initialized() && !disposed)
    {
        CHECK_VK_SUCCESS(vkEndCommandBuffer(cmdBuffer),
                         ErrorMessages::FAILED_CANNOT_END_CMD_BUFFER);
        disposed = true;
    }
}

DisposableCmdBuffer::~DisposableCmdBuffer()
{
    finish();
    if (initialized())
    {
        vkFreeCommandBuffers(getLogicalDev(), *cmdPool, 1, &cmdBuffer);
    }
}

DisposableCmdBuffer::DisposableCmdBuffer() : AVkGraphicsBase(), disposed(true) {}

VkCommandBuffer& DisposableCmdBuffer::commandBuffer()
{
    return cmdBuffer;
}
