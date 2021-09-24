//
// Created by Supakorn on 9/6/2021.
//
#include "FrameSemaphores.h"

constexpr char const* CREATE_SEMAPHORE_FAILED = "Cannot create Semaphore!";
constexpr char const* CREATE_FENCE_FAILED = "Cannot create Fence!";

FrameSemaphores::FrameSemaphores(VkDevice* logicalDev, VkFenceCreateFlags const& fenceFlags) :
        logicalDev(logicalDev)
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    CHECK_VK_SUCCESS(
            vkCreateSemaphore(*logicalDev, &createInfo, nullptr, &imgAvailable),
            CREATE_SEMAPHORE_FAILED);
    CHECK_VK_SUCCESS(
            vkCreateSemaphore(*logicalDev, &createInfo, nullptr, &renderFinished),
            CREATE_SEMAPHORE_FAILED);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = fenceFlags;

    CHECK_VK_SUCCESS(
            vkCreateFence(*logicalDev, &fenceCreateInfo, nullptr, &inFlight),
            CREATE_FENCE_FAILED);
}

FrameSemaphores::FrameSemaphores(FrameSemaphores&& frameSem) noexcept:
        logicalDev(frameSem.logicalDev)
{
    imgAvailable = frameSem.imgAvailable;
    renderFinished = frameSem.renderFinished;
    inFlight = frameSem.inFlight;

    frameSem.imgAvailable = VK_NULL_HANDLE;
    frameSem.renderFinished = VK_NULL_HANDLE;
    frameSem.logicalDev = VK_NULL_HANDLE;
    frameSem.inFlight = VK_NULL_HANDLE;
}

FrameSemaphores& FrameSemaphores::operator=(FrameSemaphores&& frameSem) noexcept
{
    logicalDev = frameSem.logicalDev;
    imgAvailable = frameSem.imgAvailable;
    renderFinished = frameSem.renderFinished;

    frameSem.imgAvailable = VK_NULL_HANDLE;
    frameSem.renderFinished = VK_NULL_HANDLE;
    frameSem.logicalDev = VK_NULL_HANDLE;
    frameSem.inFlight = VK_NULL_HANDLE;

    return *this;
}

FrameSemaphores::~FrameSemaphores()
{
    if (logicalDev != nullptr)
    {
        vkDestroySemaphore(*logicalDev, imgAvailable, nullptr);
        vkDestroySemaphore(*logicalDev, renderFinished, nullptr);
        vkDestroyFence(*logicalDev, inFlight, nullptr);
    }
}

FrameSemaphores::FrameSemaphores() : logicalDev(nullptr) {}
