//
// Created by Supakorn on 11/12/2021.
//
#include "common.h"
#include "ShadowmapPipeline.h"
#include "Image.h"
#include "Lights.h"

void
ShadowmapPipeline::createGraphicsPipeline(std::string const& vertShaderName,
                                          uint32_t const& shadowResolution,
                                          std::vector<VkDescriptorSetLayout>& descSetLayouts)
{
    VkExtent2D extent;
    extent.height = shadowResolution;
    extent.width = shadowResolution;

    auto vertShader = Shaders::createShaderModuleChecked(getLogicalDev(), vertShaderName);

    VkPipelineShaderStageCreateInfo vertShaderInfo = {};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderInfo.module = vertShader;
    vertShaderInfo.pName = "main";

    /*
    auto fragShader = Shaders::createShaderModuleChecked(getLogicalDev(), "shadowmap_dir.frag.spv");

    VkPipelineShaderStageCreateInfo fragShaderInfo = {};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderInfo.module = fragShader;
    fragShaderInfo.pName = "main";

     */
    VkPipelineShaderStageCreateInfo stages[] = {vertShaderInfo}; // , fragShaderInfo};

    auto bindingDesc = Vertex::bindingDescription();
    auto attribDesc = Vertex::attributeDescription();

    VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
    vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputInfo.vertexBindingDescriptionCount = 1;
    vertInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertInputInfo.vertexAttributeDescriptionCount = CAST_UINT32(attribDesc.size());
    vertInputInfo.pVertexAttributeDescriptions = attribDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAsmStateInfo = {};
    inputAsmStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAsmStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)shadowResolution;
    viewport.height=(float)shadowResolution;
    viewport.minDepth=0.0f;
    viewport.maxDepth=1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0,0};
    scissor.extent = extent;
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

    for (float& blendConstant : colorBlendCreateInfo.blendConstants)
    {
        blendConstant = 0.0f;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = CAST_UINT32(descSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    CHECK_VK_SUCCESS(vkCreatePipelineLayout(getLogicalDev(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
                     "Cannot create pipeline layout!");

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};


    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 1;
    pipelineCreateInfo.pStages = stages;
    pipelineCreateInfo.pVertexInputState = &vertInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAsmStateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    CHECK_VK_SUCCESS(vkCreateGraphicsPipelines(
            getLogicalDev(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipeline),
                     "Cannot create shadow map render pass!");
    vkDestroyShaderModule(getLogicalDev(), vertShader, nullptr);
}

VkRenderPass ShadowmapPipeline::createRenderPass(VkPhysicalDevice const& physDev)
{
    VkRenderPass newRenderPass;

    /*
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_SNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
     */

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.preserveAttachmentCount = 0;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = nullptr; //&colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[] = { /*colorAttachment,*/ depthAttachment};

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    CHECK_VK_SUCCESS(
            vkCreateRenderPass(getLogicalDev(), &renderPassCreateInfo, nullptr, &newRenderPass),
            "Cannot create shadow map render pass!");
    return newRenderPass;
}

VkSamplerCreateInfo ShadowmapPipeline::createSamplerInfo()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    return samplerInfo;
}

ShadowmapPipeline::ShadowmapPipeline(
        VkDevice* device,
        VmaAllocator* allocator,
        VkPhysicalDevice const& physDev,
        std::string const& vertShader,
        uint32_t const& shadowResolution,
        size_t const& swpchainImgCount,
        std::vector<VkDescriptorSetLayout>& descSetLayouts,
        VkCommandPool& cmdPool) :
        AVkGraphicsBase(device), cmdPool(cmdPool), shadowMapRes(shadowResolution)
{
    // descSetLayout = createDescSetLayout();
    renderPass = createRenderPass(physDev);

    depthTarget = std::make_unique<Image::Image>(
            device, allocator, std::make_pair(shadowResolution, shadowResolution),
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            createSamplerInfo(),
            nullopt,
            VK_IMAGE_TILING_OPTIMAL,
            1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_ASPECT_DEPTH_BIT);
    /*
    dummyColorTarget = std::make_unique<Image::Image>(
            device, allocator, std::make_pair(shadowResolution, shadowResolution),
            VK_FORMAT_R8G8B8A8_SNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            createSamplerInfo(),
            nullopt,
            VK_IMAGE_TILING_OPTIMAL,
            1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_ASPECT_COLOR_BIT);
            */
    shadowmapFramebuffer = createFrameBuffer(shadowResolution,
                                             // dummyColorTarget->imgView,
                                             depthTarget->imgView);
    createGraphicsPipeline(vertShader, shadowResolution, descSetLayouts);
    // createDescSet(swpchainImgCount);

    for (int i = 0; i < swpchainImgCount; ++i)
    {
        lightDirUnifBuffer.emplace_back(
                device, allocator, physDev,
                nullopt, 0,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    smapCmdBuffer.resize(swpchainImgCount);

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // 1 command buffer per frame buffer
    allocateInfo.commandBufferCount = smapCmdBuffer.size();

    CHECK_VK_SUCCESS(vkAllocateCommandBuffers(*device, &allocateInfo, smapCmdBuffer.data()),
                     ErrorMessages::FAILED_CANNOT_CREATE_CMD_BUFFER);
}

VkFramebuffer ShadowmapPipeline::createFrameBuffer(
        uint32_t const& shadowResolution,
        // VkImageView& colImgView,
        VkImageView& imgView)
{
    VkFramebuffer fb;
    // creating frame buffer
    std::vector<VkImageView> attachments = {
            // colImgView,
            imgView
    };

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = CAST_UINT32(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.width = shadowResolution;
    createInfo.height = shadowResolution;
    createInfo.layers = 1;

    CHECK_VK_SUCCESS(
            vkCreateFramebuffer(getLogicalDev(), &createInfo, nullptr, &fb),
            "Cannot create shadow map depth buffer!");

    return fb;
}

ShadowmapPipeline::~ShadowmapPipeline()
{
    if (initialized())
    {
        vkFreeCommandBuffers(getLogicalDev(), cmdPool, smapCmdBuffer.size(), smapCmdBuffer.data());
        // vkDestroyDescriptorPool(getLogicalDev(), lightDirDescPool, nullptr);
        // vkDestroyDescriptorSetLayout(getLogicalDev(), descSetLayout, nullptr);

        vkDestroyFramebuffer(getLogicalDev(), shadowmapFramebuffer, nullptr);

        vkDestroyRenderPass(getLogicalDev(), renderPass, nullptr);
        vkDestroyPipeline(getLogicalDev(), pipeline, nullptr);
        vkDestroyPipelineLayout(getLogicalDev(), pipelineLayout, nullptr);
    }
}

VkRenderPassBeginInfo ShadowmapPipeline::renderPassBeginInfo(VkClearValue const& clearVal) const
{
    VkRect2D renderArea = {};
    renderArea.offset = {0, 0};
    renderArea.extent.width = shadowMapRes;
    renderArea.extent.height = shadowMapRes;

    VkRenderPassBeginInfo smapRenderPassBeginInfo = {};
    smapRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    smapRenderPassBeginInfo.renderPass = renderPass;
    smapRenderPassBeginInfo.framebuffer = shadowmapFramebuffer;
    smapRenderPassBeginInfo.renderArea = renderArea;

    smapRenderPassBeginInfo.clearValueCount = 1;
    smapRenderPassBeginInfo.pClearValues = &clearVal;

    return smapRenderPassBeginInfo;
}

/*
VkDescriptorSetLayout ShadowmapPipeline::createDescSetLayout()
{
    VkDescriptorSetLayout descLayout;
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
            MeshUniform::descriptorSetLayout(0),
            Light::directionalLightTransfLayoutBinding(1)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = CAST_UINT32(bindings.size());
    layoutInfo.pBindings = bindings.data();

    CHECK_VK_SUCCESS(
            vkCreateDescriptorSetLayout(getLogicalDev(), &layoutInfo, nullptr, &descLayout),
            "Cannot create Shadow map descriptor set layout!");

    return descLayout;
}

void ShadowmapPipeline::createDescSet(uint32_t imageSize)
{
    VkDescriptorPoolSize poolSize[1] = {{}};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = imageSize;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = poolSize;
    createInfo.maxSets = imageSize;
    CHECK_VK_SUCCESS(
            vkCreateDescriptorPool(getLogicalDev(), &createInfo, nullptr, &lightDirDescPool),
            "Cannot create descriptor pools for shadow maps!"
    );

    std::vector<VkDescriptorSetLayout> layouts(imageSize, descSetLayout);

    VkDescriptorSetAllocateInfo VDSAcreateInfo = {};

    VDSAcreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    VDSAcreateInfo.descriptorPool = lightDirDescPool;
    VDSAcreateInfo.descriptorSetCount = CAST_UINT32(layouts.size());
    VDSAcreateInfo.pSetLayouts = layouts.data();

    lightDirSets.resize(imageSize);

    CHECK_VK_SUCCESS(
            vkAllocateDescriptorSets(getLogicalDev(), &VDSAcreateInfo, lightDirSets.data()),
            "Cannot create descriptor sets for shadow maps!"
    );
}

void ShadowmapPipeline::configureBuffers(
        std::vector<VkDescriptorSet>& meshUnifDescSets,
        DynUniformObjBuffer<MeshUniform> const& unif)
{
    VkDescriptorBufferInfo sboBufferInfo = {};
    sboBufferInfo.buffer = unif.vertexBuffer;
    sboBufferInfo.offset = 0;
    sboBufferInfo.range = sizeof(MeshUniform);

    for (int i = 0; i < lightDirSets.size(); ++i)
    {
        VkWriteDescriptorSet writeDescSet = MeshUniform::descriptorWrite(0, sboBufferInfo, meshUnifDescSets[i]);

        VkDescriptorBufferInfo lightUnifBufferInfo = {};
        lightUnifBufferInfo.buffer = lightDirUnifBuffer[i].vertexBuffer;
        lightUnifBufferInfo.offset = 0;
        lightUnifBufferInfo.range = sizeof(glm::mat4);

        VkWriteDescriptorSet descriptorWriteLight = {};
        descriptorWriteLight.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteLight.dstSet = lightDirSets[i];
        descriptorWriteLight.dstBinding = 1;
        descriptorWriteLight.dstArrayElement = 0;
        descriptorWriteLight.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteLight.descriptorCount = 1;
        descriptorWriteLight.pBufferInfo = &lightUnifBufferInfo;

        VkWriteDescriptorSet descSets[] = {writeDescSet, descriptorWriteLight};

        vkUpdateDescriptorSets(
                getLogicalDev(),
                2, descSets,
                0, nullptr);
    }
}
*/