//
// Created by Supakorn on 9/18/2021.
//

#include "SwapchainImgBuffers.h"

void SwapchainImageBuffers::configureBuffers(uint32_t const& binding, Image::Image& img)
{
    for (uint32_t i = 0; i < imgSize; ++i)
    {
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

        std::vector<VkWriteDescriptorSet> descriptorWriteInfo = {
                UniformObjects::descriptorWrite(0, unifBuffers[i].bufferInfo(), descriptorSets[i]),
                descriptorWriteImg
        };

        vkUpdateDescriptorSets(
                getLogicalDev(), CAST_UINT32(descriptorWriteInfo.size()),
                descriptorWriteInfo.data(), 0, nullptr);
    }
}

void SwapchainImageBuffers::configureMeshBuffers(uint32_t const& binding, DynUniformObjBuffer<MeshUniform> const& unif)
{
    for (uint32_t i = 0; i < imgSize; ++i)
    {
        VkDescriptorBufferInfo sboBufferInfo = {};
        sboBufferInfo.buffer = unif.vertexBuffer;
        sboBufferInfo.offset = 0;
        sboBufferInfo.range = sizeof(MeshUniform);

        VkWriteDescriptorSet writeDescSet = MeshUniform::descriptorWrite(0, sboBufferInfo, meshDescriptorSets[i]);
        vkUpdateDescriptorSets(
                getLogicalDev(),
                1, &writeDescSet,
                0, nullptr);
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
            imgBindingData
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = CAST_UINT32(bindings.size());
    layoutInfo.pBindings = bindings.data();

    return vkCreateDescriptorSetLayout(getLogicalDev(), &layoutInfo, nullptr, &descriptorSetLayout);
}

VkResult SwapchainImageBuffers::createDescriptorSets(SwapchainComponents const& swapchainComponent)
{
    std::vector<VkDescriptorSetLayout> layouts(imgSize, descriptorSetLayout);

    VkDescriptorSetAllocateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    createInfo.descriptorPool = swapchainComponent.descriptorPool;
    createInfo.descriptorSetCount = CAST_UINT32(layouts.size());
    createInfo.pSetLayouts = layouts.data();
    descriptorSets.resize(imgSize);
    return vkAllocateDescriptorSets(getLogicalDev(), &createInfo, descriptorSets.data());
}

VkResult SwapchainImageBuffers::createMeshDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
            MeshUniform::descriptorSetLayout(0)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = CAST_UINT32(bindings.size());
    layoutInfo.pBindings = bindings.data();

    return vkCreateDescriptorSetLayout(getLogicalDev(), &layoutInfo, nullptr, &meshDescriptorSetLayout);
}

VkResult SwapchainImageBuffers::createMeshDescriptorSets(VkDescriptorPool const& descPool)
{
    std::vector<VkDescriptorSetLayout> layouts(imgSize, meshDescriptorSetLayout);

    VkDescriptorSetAllocateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    createInfo.descriptorPool = descPool;
    createInfo.descriptorSetCount = CAST_UINT32(layouts.size());
    createInfo.pSetLayouts = layouts.data();
    meshDescriptorSets.resize(imgSize);
    return vkAllocateDescriptorSets(getLogicalDev(), &createInfo, meshDescriptorSets.data());
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
    meshDescriptorSetLayout = std::move(sib.meshDescriptorSetLayout);
    unifBuffers = std::move(sib.unifBuffers);
    descriptorSets = std::move(sib.descriptorSets);
    meshDescriptorSets = std::move(sib.meshDescriptorSets);
    allocator = sib.allocator;
    imgSize = sib.imgSize;

    sib.unifBuffers.clear();
    sib.descriptorSets.clear();

    return *this;
}

SwapchainImageBuffers::SwapchainImageBuffers(SwapchainImageBuffers&& sib) noexcept:
        AVkGraphicsBase(std::move(sib)),
        descriptorSetLayout(std::move(sib.descriptorSetLayout)),
        meshDescriptorSetLayout(std::move(sib.meshDescriptorSetLayout)),
        unifBuffers(std::move(sib.unifBuffers)),
        descriptorSets(std::move(sib.descriptorSets)),
        meshDescriptorSets(std::move(sib.meshDescriptorSets)),
        allocator(sib.allocator),
        imgSize(sib.imgSize)
{
    sib.unifBuffers.clear();
    sib.descriptorSets.clear();
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


    CHECK_VK_SUCCESS(createMeshDescriptorSetLayout(), "Cannot create descriptor set layout!");
    CHECK_VK_SUCCESS(createMeshDescriptorSets(swapchainComponent.descriptorPool),
                     "Cannot create descriptor sets!");

    configureBuffers(binding, img);
}

SwapchainImageBuffers::~SwapchainImageBuffers()
{
    if (initialized())
    {
        vkDestroyDescriptorSetLayout(getLogicalDev(), meshDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(getLogicalDev(), descriptorSetLayout, nullptr);
    }
}
