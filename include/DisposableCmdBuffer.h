//
// Created by Supakorn on 9/16/2021.
//

#pragma once
#include "common.h"

class DisposableCmdBuffer : public AVkGraphicsBase
{
public:
    DisposableCmdBuffer();
    ~DisposableCmdBuffer();

    explicit DisposableCmdBuffer(VkDevice* logicalDev, VkCommandPool* pool);

    DisposableCmdBuffer(DisposableCmdBuffer const&) = delete;
    DisposableCmdBuffer& operator=(DisposableCmdBuffer const&) = delete;

    DisposableCmdBuffer(DisposableCmdBuffer&& dcb) noexcept;
    DisposableCmdBuffer& operator=(DisposableCmdBuffer&& dcb) noexcept;

    VkCommandBuffer& commandBuffer();
    VkResult submit(VkQueue& queue);
    void finish();

private:
    VkCommandPool* cmdPool = nullptr;
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

    bool disposed = false;
};
