//
// Created by Supakorn on 9/18/2021.
//

#include "SwapchainImgBuffers.h"

void SwapchainImageBuffers::configureBuffers(uint32_t const& binding, Image::Image& img)
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

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = img.imgView;
        imageInfo.sampler = img.baseSampler;

        VkWriteDescriptorSet descriptorWriteImg = {};
        descriptorWriteImg.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteImg.dstSet = descriptorSets[i];
        descriptorWriteImg.dstBinding = 1;
        descriptorWriteImg.dstArrayElement = 0;
        descriptorWriteImg.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteImg.descriptorCount = 1;
        descriptorWriteImg.pImageInfo = &imageInfo;

        VkDescriptorBufferInfo sboBufferInfo = {};
        sboBufferInfo.buffer = lightSBOs[i].vertexBuffer;
        sboBufferInfo.offset = 0;
        sboBufferInfo.range = lightSBOs[i].getSize();

        VkWriteDescriptorSet descriptorWriteSBO = {};
        descriptorWriteSBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSBO.dstSet = descriptorSets[i];
        descriptorWriteSBO.dstBinding = 2;
        descriptorWriteSBO.dstArrayElement = 0;
        descriptorWriteSBO.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWriteSBO.descriptorCount = 1;
        descriptorWriteSBO.pBufferInfo = &sboBufferInfo;


        std::vector<VkWriteDescriptorSet> descriptorWriteInfo = {
                descriptorWrite, descriptorWriteImg, descriptorWriteSBO};

        vkUpdateDescriptorSets(
                getLogicalDev(), static_cast<uint32_t>(descriptorWriteInfo.size()),
                descriptorWriteInfo.data(), 0, nullptr);
    }
}

VkResult SwapchainImageBuffers::createDescriptorSetLayout()
{

    VkDescriptorSetLayoutBinding imgBindingData = {};
    imgBindingData.binding = 1;
    imgBindingData.descriptorCount = 1;
    imgBindingData.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    imgBindingData.pImmutableSamplers = nullptr;
    imgBindingData.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
            UniformObjects::descriptorSetLayout(0),
            imgBindingData,
            StorageBufferArray<Light>::DescriptorSetLayout(2)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    return vkCreateDescriptorSetLayout(getLogicalDev(), &layoutInfo, nullptr, &descriptorSetLayout);
}

VkResult SwapchainImageBuffers::createDescriptorSets(SwapchainComponents const& swapchainComponent)
{
    std::vector<VkDescriptorSetLayout> layouts(imgSize, descriptorSetLayout);

    VkDescriptorSetAllocateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    createInfo.descriptorPool = swapchainComponent.descriptorPool;
    createInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    createInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(imgSize);
    return vkAllocateDescriptorSets(getLogicalDev(), &createInfo, descriptorSets.data());
}

std::pair<UniformObjBuffer<UniformObjects>&, VkDescriptorSet&> SwapchainImageBuffers::operator[](uint32_t const& i)
{
    if (i < imgSize)
    {
        return { unifBuffers[i], descriptorSets[i] };
    }
    throw std::runtime_error("Attempting to access index outside image count!");
}

SwapchainImageBuffers& SwapchainImageBuffers::operator=(SwapchainImageBuffers&& sib) noexcept
{
    AVkGraphicsBase::operator=(std::move(sib));
    descriptorSetLayout = std::move(sib.descriptorSetLayout);
    unifBuffers = std::move(sib.unifBuffers);
    descriptorSets = std::move(sib.descriptorSets);
    lightSBOs = std::move(sib.lightSBOs);
    allocator = sib.allocator;
    imgSize = sib.imgSize;

    sib.unifBuffers.clear();
    sib.descriptorSets.clear();
    sib.lightSBOs.clear();

    return *this;
}

SwapchainImageBuffers::SwapchainImageBuffers(SwapchainImageBuffers&& sib) noexcept:
        AVkGraphicsBase(std::move(sib)),
        descriptorSetLayout(std::move(sib.descriptorSetLayout)),
        unifBuffers(std::move(sib.unifBuffers)),
        lightSBOs(std::move(sib.lightSBOs)),
        descriptorSets(std::move(sib.descriptorSets)),
        allocator(sib.allocator),
        imgSize(sib.imgSize)
{
    sib.unifBuffers.clear();
    sib.descriptorSets.clear();
    sib.lightSBOs.clear();
}

void SwapchainImageBuffers::createUniformBuffers(VkPhysicalDevice const& physDev,
                                                 SwapchainComponents const& swapchainComponent)
{
    for (uint32_t i=0; i < imgSize; ++i)
    {
        unifBuffers.emplace_back(
                getLogicalDevPtr(), allocator, physDev,
                nullopt, 0,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        lightSBOs.emplace_back(
                getLogicalDevPtr(), allocator, physDev, 64,
                nullopt, 0,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

SwapchainImageBuffers::SwapchainImageBuffers(VkDevice* logicalDev, VmaAllocator* allocator,
                                             VkPhysicalDevice const& physDev,
                                             SwapchainComponents const& swapchainComponent,
                                             Image::Image& img,
                                             uint32_t const& binding) :
        AVkGraphicsBase(logicalDev), allocator(allocator), imgSize(swapchainComponent.imageCount())
{
    CHECK_VK_SUCCESS(createDescriptorSetLayout(), "Cannot create descriptor set layout!");
    createUniformBuffers(physDev, swapchainComponent);
    CHECK_VK_SUCCESS(createDescriptorSets(swapchainComponent), "Cannot create descriptor sets!");

    configureBuffers(binding, img);
}

SwapchainImageBuffers::~SwapchainImageBuffers()
{
    if (initialized())
    {
        vkDestroyDescriptorSetLayout(getLogicalDev(), descriptorSetLayout, nullptr);
    }
}
