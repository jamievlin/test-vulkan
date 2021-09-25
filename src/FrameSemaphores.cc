//
// Created by Supakorn on 9/6/2021.
//
#include "FrameSemaphores.h"

constexpr char const* CREATE_SEMAPHORE_FAILED = "Cannot create Semaphore!";
constexpr char const* CREATE_FENCE_FAILED = "Cannot create Fence!";

FrameSemaphores::FrameSemaphores(VkDevice* logicalDev, VkFenceCreateFlags const& fenceFlags) :
        AVkGraphicsBase(logicalDev)
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
        AVkGraphicsBase(std::move(frameSem))
{
    imgAvailable = frameSem.imgAvailable;
    renderFinished = frameSem.renderFinished;
    inFlight = frameSem.inFlight;

    frameSem.imgAvailable = VK_NULL_HANDLE;
    frameSem.renderFinished = VK_NULL_HANDLE;
    frameSem.inFlight = VK_NULL_HANDLE;
}

FrameSemaphores& FrameSemaphores::operator=(FrameSemaphores&& frameSem) noexcept
{
    AVkGraphicsBase::operator=(std::move(frameSem));
    imgAvailable = frameSem.imgAvailable;
    renderFinished = frameSem.renderFinished;

    frameSem.imgAvailable = VK_NULL_HANDLE;
    frameSem.renderFinished = VK_NULL_HANDLE;
    frameSem.inFlight = VK_NULL_HANDLE;

    return *this;
}

FrameSemaphores::~FrameSemaphores()
{
    if (initialized())
    {
        vkDestroySemaphore(getLogicalDev(), imgAvailable, nullptr);
        vkDestroySemaphore(getLogicalDev(), renderFinished, nullptr);
        vkDestroyFence(getLogicalDev(), inFlight, nullptr);
    }
}

FrameSemaphores::FrameSemaphores() : AVkGraphicsBase() {}
