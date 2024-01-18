//
// Created by Supakorn on 9/18/2021.
//

#pragma once
#include "common.h"
#include "SwapchainComponent.h"
#include "Image.h"
#include "UniformObjects.h"
#include "Lights.h"
#include "StorageBufferArray.h"

class SwapchainImageBuffers : public AVkGraphicsBase
{
public:
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout meshDescriptorSetLayout = VK_NULL_HANDLE;

    std::vector<UniformObjBuffer<UniformObjects>> unifBuffers;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> meshDescriptorSets;

    SwapchainImageBuffers() = default;
    ~SwapchainImageBuffers();

    SwapchainImageBuffers(SwapchainImageBuffers const&) = delete;
    SwapchainImageBuffers& operator=(SwapchainImageBuffers const&) = delete;

    SwapchainImageBuffers(SwapchainImageBuffers&& sib) noexcept;
    SwapchainImageBuffers& operator=(SwapchainImageBuffers&& sib) noexcept;

    SwapchainImageBuffers(
        VkDevice* logicalDev, VmaAllocator* allocator, VkPhysicalDevice const& physDev,
        SwapchainComponents const& swapchainComponent, Image::Image& img, uint32_t const& binding
    );

    void createUniformBuffers(
        VkPhysicalDevice const& physDev, SwapchainComponents const& swapchainComponent
    );
    void configureBuffers(uint32_t const& binding, Image::Image& img);
    void configureMeshBuffers(
        uint32_t const& binding, DynUniformObjBuffer<MeshUniform> const& unif
    );
    VkResult createDescriptorSetLayout();
    VkResult createDescriptorSets(SwapchainComponents const& swapchainComponent);

    VkResult createMeshDescriptorSetLayout();
    VkResult createMeshDescriptorSets(VkDescriptorPool const& descPool);
    std::pair<UniformObjBuffer<UniformObjects>&, VkDescriptorSet&> operator[](uint32_t const& i);

private:
    VmaAllocator* allocator = nullptr;
    uint32_t imgSize = 0;
};
