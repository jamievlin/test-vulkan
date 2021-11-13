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
    static VkDescriptorSetLayoutBinding descriptorSetLayout(uint32_t binding=0);
    static VkWriteDescriptorSet descriptorWrite(
            uint32_t const& binding,
            VkDescriptorBufferInfo const& bufferInfo,
            VkDescriptorSet& dest);
};

struct MeshUniform
{
    glm::vec4 baseColor;
    glm::mat4 model;
    glm::mat4 modelInvDual;


    static VkDescriptorSetLayoutBinding descriptorSetLayout(uint32_t binding=0);

    MeshUniform() = default;
    explicit MeshUniform(glm::mat4 const& model) : model(model), modelInvDual(glm::inverseTranspose(model)) {}

    void computeInvDual();
    void setModelMatrix(glm::mat4 const& newModel);

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

    [[nodiscard]]
    VkDescriptorBufferInfo bufferInfo() const
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = vertexBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(TUniformBuffer);
        return bufferInfo;
    }
};

/*
 * Desgined as a circular queue.
 */
template<typename TUniformBuffer>
class DynUniformObjBuffer : public Buffers::Buffer
{
public:
    DynUniformObjBuffer() = default;
    ~DynUniformObjBuffer() override
    {
    };

    DynUniformObjBuffer(
            VkDevice* dev,
            VmaAllocator* allocator,
            VkPhysicalDevice const& physicalDev,
            uint32_t sizeCount = 256,
            Buffers::optUint32Set const& usedQueues = nullopt,
            VkFlags const& additionalFlags = 0,
            VkMemoryPropertyFlags const& memoryFlags = 0

    ) : Buffer(dev, allocator, physicalDev, sizeCount * stride(physicalDev),
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | additionalFlags,
               VMA_MEMORY_USAGE_CPU_TO_GPU,
               memoryFlags, usedQueues),
        sizeCount(sizeCount), currentFreeIdx(0), strideSize(DynUniformObjBuffer::stride(physicalDev))
    {
    }

    DynUniformObjBuffer(DynUniformObjBuffer const&) = delete;
    DynUniformObjBuffer& operator=(DynUniformObjBuffer const&) = delete;

    DynUniformObjBuffer(DynUniformObjBuffer&& dubo) noexcept :
        Buffers::Buffer(std::move(dubo)), sizeCount(dubo.sizeCount) {}

    DynUniformObjBuffer& operator= (DynUniformObjBuffer&& dubo) noexcept
    {
        Buffers::Buffer::operator=(std::move(dubo));
        sizeCount = dubo.sizeCount;
        return *this;
    }

    static uint32_t stride(VkPhysicalDevice const& physDev)
    {
        VkPhysicalDeviceProperties prop;
        vkGetPhysicalDeviceProperties(physDev, &prop);

        uint32_t base_multiple = sizeof(TUniformBuffer) / prop.limits.minUniformBufferOffsetAlignment;
        uint32_t additional = (sizeof(TUniformBuffer) % prop.limits.minUniformBufferOffsetAlignment == 0) ? 0 : 1;
        uint32_t strideResult = (base_multiple + additional) * (uint32_t)prop.limits.minUniformBufferOffsetAlignment;
        return strideResult;
    }

    void beginFenceGroup(uint32_t const& frameIdx, VkFence fence)
    {
        waitingFrame[currentFreeIdx] = frameIdx;
        latestFrame[frameIdx] = currentFreeIdx;
        frameFences[frameIdx] = fence;
    }

    uint32_t placeNextData(TUniformBuffer const& data)
    {
        // if we encounter a frame with marked fence,
        auto it = waitingFrame.find(currentFreeIdx);
        if (!ignoreFence && it != waitingFrame.end())
        {
            VkFence fen = frameFences[it->second];
            if (fen != VK_NULL_HANDLE && latestFrame[it->second] == currentFreeIdx)
            {
                CHECK_VK_SUCCESS(
                        vkWaitForFences(getLogicalDev(), 1, &frameFences[it->second], VK_TRUE, UINT64_MAX),
                        "Cannot wait for fence!");
                waitingFrame.erase(it);
            }
        }
        CHECK_VK_SUCCESS(loadDataIdx(data, currentFreeIdx), "Cannot load data!");

        auto idx = currentFreeIdx * strideSize;
        currentFreeIdx = (currentFreeIdx + 1) % sizeCount;
        ignoreFence = false;
        return idx;
    }

    VkResult loadDataIdx(TUniformBuffer const& data, uint32_t const& idx)
    {
        return Buffers::Buffer::loadData(&data, idx * strideSize, sizeof(TUniformBuffer));
    }

    [[nodiscard]]
    VkDescriptorBufferInfo bufferInfo() const
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = vertexBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(TUniformBuffer);
        return bufferInfo;
    }

    static VkWriteDescriptorSet descriptorWrite(
            uint32_t const& binding,
            VkDescriptorBufferInfo const& bufferInfo,
            VkDescriptorSet& dest)
    {
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = dest;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        return descriptorWrite;
    }

private:
    uint32_t sizeCount;
    uint32_t currentFreeIdx = 0;
    uint32_t strideSize;
    std::map<uint32_t, uint32_t> waitingFrame;
    VkFence frameFences[MAX_FRAMES_IN_FLIGHT] = {nullptr};
    int32_t latestFrame[MAX_FRAMES_IN_FLIGHT] = {-1};
    bool ignoreFence = false;
};