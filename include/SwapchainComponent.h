//
// Created by Supakorn on 9/8/2021.
//

#pragma once
#include "common.h"
#include "QueueFamilies.h"
#include "SwapChains.h"
#include "SwapchainSupport.h"

class SwapchainComponents
{
public:
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkSurfaceFormatKHR swapchainFormat = {};
    VkExtent2D swapchainExtent = {};

    std::vector<SwapchainImageSupport> swapchainSupport;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    SwapChainsDetail detail;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    SwapchainComponents() = default;

    SwapchainComponents(
            VkDevice* logicalDev,
            VkPhysicalDevice const& physDevice,
            VkSurfaceKHR const& surface,
            std::pair<size_t, size_t> const& windowHeight
            );

    SwapchainComponents(SwapchainComponents const&) = delete;
    SwapchainComponents& operator=(SwapchainComponents const&) = delete;

    SwapchainComponents(SwapchainComponents&& swpchainComp) noexcept;
    SwapchainComponents& operator= (SwapchainComponents&& swpchainComp) noexcept;

    ~SwapchainComponents();

    [[nodiscard]]
    uint32_t imageCount() const;

protected:
    VkResult initSwapChain(
            VkPhysicalDevice const& physDevice,
            std::pair<size_t, size_t> const& windowHeight,
            VkSurfaceKHR const& surface);

    VkResult createRenderPasses();
    VkResult createDescriptorPool();

private:
    VkDevice* logicalDev=nullptr;

};