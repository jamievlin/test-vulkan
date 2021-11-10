//
// Created by Supakorn on 9/17/2021.
//

#pragma once
#include "common.h"
#include "Buffers.h"



struct UniformObjects
{
    float time;
    VEC4_ALIGN glm::vec4 cameraPos;
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 modelInvDual;

    static VkDescriptorSetLayoutBinding descriptorSetLayout(uint32_t binding=0);
};

template<typename TUniformBuffer=UniformObjects>
class UniformObjBuffer : public Buffers::Buffer
{
public:
    UniformObjBuffer() = default;
    ~UniformObjBuffer() override = default;

    UniformObjBuffer(
            VkDevice* dev,
            VmaAllocator* allocator,
            VkPhysicalDevice const& physicalDev,
            Buffers::optUint32Set const& usedQueues = nullopt,
            VkFlags const& additionalFlags = 0,
            VkMemoryPropertyFlags const& memoryFlags = 0

    ) : Buffer(dev, allocator, physicalDev, sizeof(TUniformBuffer),
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | additionalFlags,
               VMA_MEMORY_USAGE_CPU_TO_GPU,
               memoryFlags, usedQueues)
    {
    }

    UniformObjBuffer(UniformObjBuffer const&) = delete;
    UniformObjBuffer& operator=(UniformObjBuffer const&) = delete;

    UniformObjBuffer(UniformObjBuffer&& ubo) noexcept : Buffers::Buffer(std::move(ubo)) {}

    UniformObjBuffer& operator= (UniformObjBuffer&& ubo) noexcept
    {
        Buffers::Buffer::operator=(std::move(ubo));

        return *this;
    }

    VkResult loadData(TUniformBuffer const& data)
    {
        return Buffers::Buffer::loadData(&data);
    }
};