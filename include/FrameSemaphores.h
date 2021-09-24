//
// Created by Supakorn on 9/6/2021.
//

#pragma once
#include "common.h"

class FrameSemaphores
{
public:
    VkSemaphore imgAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;

    FrameSemaphores();
    explicit FrameSemaphores(
            VkDevice* logicalDev,
            VkFenceCreateFlags const& fenceFlags=VK_FENCE_CREATE_SIGNALED_BIT);

    FrameSemaphores(FrameSemaphores const&) = delete;
    FrameSemaphores& operator=(FrameSemaphores const&) = delete;

    FrameSemaphores(FrameSemaphores&& frameSem) noexcept;
    FrameSemaphores& operator= (FrameSemaphores&& frameSem) noexcept;

    ~FrameSemaphores();

private:
    VkDevice* logicalDev;
};