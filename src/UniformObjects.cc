//
// Created by Supakorn on 9/17/2021.
//

#include "UniformObjects.h"

VkDescriptorSetLayoutBinding UniformObjects::descriptorSetLayout(uint32_t binding)
{
    VkDescriptorSetLayoutBinding uboLayout = {};
    uboLayout.binding = binding;
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayout.descriptorCount = 1;
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayout.pImmutableSamplers = nullptr;

    return uboLayout;
}

VkWriteDescriptorSet UniformObjects::descriptorWrite(
        uint32_t const& binding, VkDescriptorBufferInfo const& bufferInfo, VkDescriptorSet& dest)
{
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = dest;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    return descriptorWrite;
}

VkDescriptorSetLayoutBinding MeshUniform::descriptorSetLayout(uint32_t binding)
{
    VkDescriptorSetLayoutBinding uboLayout = {};
    uboLayout.binding = binding;
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboLayout.descriptorCount = 1;
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayout.pImmutableSamplers = nullptr;

    return uboLayout;
}

void MeshUniform::computeInvDual()
{
    modelInvDual = glm::inverseTranspose(model);
}

void MeshUniform::setModelMatrix(glm::mat4 const& newModel)
{
    model = newModel;
    computeInvDual();
}
