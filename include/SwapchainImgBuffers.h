//
// Created by Supakorn on 9/18/2021.
//

#pragma once
#include "common.h"
#include "SwapchainComponent.h"
#include "UniformObjects.h"

class SwapchainImageBuffers
{
public:
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<UniformObjBuffer<UniformObjects>> unifBuffers;
    std::vector<VkDescriptorSet> descriptorSets;

    SwapchainImageBuffers() = default;
    ~SwapchainImageBuffers();

    SwapchainImageBuffers(SwapchainImageBuffers const&) = delete;
    SwapchainImageBuffers& operator=(SwapchainImageBuffers const&) = delete;

    SwapchainImageBuffers(SwapchainImageBuffers&& sib) noexcept;
    SwapchainImageBuffers& operator= (SwapchainImageBuffers&& sib) noexcept;


    SwapchainImageBuffers(
            VkDevice* logicalDev,
            VmaAllocator* allocator,
            VkPhysicalDevice const& physDev,
            SwapchainComponents const& swapchainComponent,
            uint32_t const& binding);

    void createUniformBuffers(VkPhysicalDevice const& physDev, SwapchainComponents const& swapchainComponent);
    void configureBuffers(uint32_t const& binding);
    VkResult createDescriptorSetLayout();
    VkResult createDescriptorSets(SwapchainComponents const& swapchainComponent);
    std::pair<UniformObjBuffer<UniformObjects>&, VkDescriptorSet& >
            operator[](uint32_t const& i);

private:
    VkDevice* logicalDev=nullptr;
    VmaAllocator* allocator=nullptr;
    uint32_t imgSize=0;
};