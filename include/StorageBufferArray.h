//
// Created by Supakorn on 11/9/2021.
//

#pragma once
#include "common.h"
#include "Buffers.h"

template <typename T> class StorageBufferArray : public Buffers::Buffer
{
public:
    StorageBufferArray(
        VkDevice* dev, VmaAllocator* allocator, VkPhysicalDevice const& physicalDev,
        uint32_t maxAllocatedSize, Buffers::optUint32Set const& usedQueues = nullopt,
        VkFlags const& additionalFlags = 0, VkMemoryPropertyFlags const& memoryFlags = 0
    )
        : Buffers::Buffer(
              dev, allocator, physicalDev, maxAllocatedSize * sizeof(T) + sizeof(glm::vec4),
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | additionalFlags, VMA_MEMORY_USAGE_CPU_TO_GPU,
              memoryFlags, usedQueues
          ),
          maxAllocatedSize(maxAllocatedSize), currentSize(0)
    {
    }

    DISALLOW_COPY(StorageBufferArray)

    StorageBufferArray(StorageBufferArray&& sba) noexcept
        : Buffers::Buffer(std::move(sba)), maxAllocatedSize(sba.maxAllocatedSize),
          currentSize(sba.currentSize)
    {
    }
    StorageBufferArray& operator=(StorageBufferArray&& sba) noexcept
    {
        Buffers::Buffer::operator=(std::move(sba));
        maxAllocatedSize = sba.maxAllocatedSize;
        currentSize = sba.currentSize;
        return *this;
    }

    void setCurrentSize(uint32_t const& newSize)
    {
        if (newSize > maxAllocatedSize)
        {
            throw std::runtime_error("Cannot be larger than the maximum allocated size!");
        }
        currentSize = newSize;
    }

    VkResult loadDataAndSetSize(std::vector<T> const& objList)
    {
        setCurrentSize(CAST_UINT32(objList.size()));
        return Buffer::loadData(
            {{&currentSize, 0, sizeof(uint32_t)},
             {objList.data(), sizeof(glm::vec4), sizeof(T) * objList.size()}}
        );
    }
    VkResult loadDataIdx(T const& data, uint32_t idx)
    {
        return Buffers::Buffer::loadData(&data, sizeof(uint32_t) + idx * sizeof(T), sizeof(T));
    }

    static VkDescriptorSetLayoutBinding DescriptorSetLayout(uint32_t binding)
    {
        VkDescriptorSetLayoutBinding uboLayout = {};
        uboLayout.binding = binding;
        uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        uboLayout.descriptorCount = 1;
        uboLayout.stageFlags = T::stageFlags();
        uboLayout.pImmutableSamplers = nullptr;

        return uboLayout;
    }

private:
    uint32_t maxAllocatedSize;
    uint32_t currentSize;
};
