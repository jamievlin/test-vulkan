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
    ~SwapchainImageBuffers()
    {
        if (logicalDev)
        {
            vkDestroyDescriptorSetLayout(*logicalDev, descriptorSetLayout, nullptr);
        }
    }

    SwapchainImageBuffers(SwapchainImageBuffers const&) = delete;
    SwapchainImageBuffers& operator=(SwapchainImageBuffers const&) = delete;

    SwapchainImageBuffers(SwapchainImageBuffers&& sib) noexcept :
        descriptorSetLayout(std::move(sib.descriptorSetLayout)),
        unifBuffers(std::move(sib.unifBuffers)),
        descriptorSets(std::move(sib.descriptorSets)),
        logicalDev(sib.logicalDev),
        allocator(sib.allocator),
        imgSize(sib.imgSize)
    {

        sib.unifBuffers.clear();
        sib.descriptorSets.clear();
        sib.logicalDev = nullptr;
    }

    SwapchainImageBuffers& operator= (SwapchainImageBuffers&& sib) noexcept
    {
        descriptorSetLayout = std::move(sib.descriptorSetLayout);
        unifBuffers = std::move(sib.unifBuffers);
        descriptorSets = std::move(sib.descriptorSets);
        logicalDev = sib.logicalDev;
        allocator = sib.allocator;
        imgSize = sib.imgSize;
        sib.logicalDev = nullptr;

        sib.unifBuffers.clear();
        sib.descriptorSets.clear();

        return *this;
    }


    SwapchainImageBuffers(
            VkDevice* logicalDev,
            VmaAllocator* allocator,
            VkPhysicalDevice const& physDev,
            SwapchainComponents const& swapchainComponent,
            uint32_t const& binding) :
            logicalDev(logicalDev), allocator(allocator), imgSize(swapchainComponent.imageCount())
    {
        CHECK_VK_SUCCESS(createDescriptorSetLayout(), "Cannot create descriptor set layout!");
        createUniformBuffers(physDev, swapchainComponent);
        CHECK_VK_SUCCESS(createDescriptorSets(swapchainComponent), "Cannot create descriptor sets!");

        configureBuffers(binding);
    }

    void createUniformBuffers(VkPhysicalDevice const& physDev, SwapchainComponents const& swapchainComponent)
    {
        for (uint32_t i=0; i < imgSize; ++i)
        {
            unifBuffers.emplace_back(
                    logicalDev, allocator, physDev,
                    nullopt, 0,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    void configureBuffers(uint32_t const& binding)
    {
        for (uint32_t i = 0; i < imgSize; ++i)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = unifBuffers[i].vertexBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = unifBuffers[i].getSize();

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;

            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr;
            descriptorWrite.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(*logicalDev, 1, &descriptorWrite, 0, nullptr);
        }
    }

    VkResult createDescriptorSetLayout()
    {
        auto layout = UniformObjects::descriptorSetLayout(0);

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &layout;

        return vkCreateDescriptorSetLayout(*logicalDev, &layoutInfo, nullptr, &descriptorSetLayout);
    }

    VkResult createDescriptorSets(SwapchainComponents const& swapchainComponent)
    {
        std::vector<VkDescriptorSetLayout> layouts(imgSize, descriptorSetLayout);

        VkDescriptorSetAllocateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        createInfo.descriptorPool = swapchainComponent.descriptorPool;
        createInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        createInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(imgSize);
        return vkAllocateDescriptorSets(*logicalDev, &createInfo, descriptorSets.data());
    }

    std::pair<UniformObjBuffer<UniformObjects>&, VkDescriptorSet& >
            operator[](uint32_t const& i)
    {
        if (i < imgSize)
        {
            return { unifBuffers[i], descriptorSets[i] };
        }
        throw std::runtime_error("Attempting to access index outside image count!");
    }

private:
    VkDevice* logicalDev=nullptr;
    VmaAllocator* allocator=nullptr;
    uint32_t imgSize=0;
};