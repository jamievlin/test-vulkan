//
// Created by Supakorn on 9/8/2021.
//

#include "GraphicsPipeline.h"

VkResult GraphicsPipeline::createGraphicsPipeline(
        std::string const& vertShaderName,
        std::string const& fragShaderName,
        VkExtent2D const& extent,
        VkRenderPass const& renderPass,
        std::vector<VkDescriptorSetLayout> const& descriptorSetLayout,
        bool enableDepthTest)
{
    auto vertShader = Shaders::createShaderModuleChecked(getLogicalDev(), vertShaderName);
    auto fragShader = Shaders::createShaderModuleChecked(getLogicalDev(), fragShaderName);

    VkPipelineShaderStageCreateInfo vertShaderInfo = {};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderInfo.module = vertShader;
    vertShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderInfo = {};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderInfo.module = fragShader;
    fragShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertShaderInfo, fragShaderInfo};

    auto bindingDesc = Vertex::bindingDescription();
    auto attribDesc = Vertex::attributeDescription();

    VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
    vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputInfo.vertexBindingDescriptionCount = 1;
    vertInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
    vertInputInfo.pVertexAttributeDescriptions = attribDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAsmStateInfo = {};
    inputAsmStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAsmStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height=(float)extent.height;
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
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    CHECK_VK_SUCCESS(vkCreatePipelineLayout(getLogicalDev(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
                     "Cannot create pipeline layout!");

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    if (enableDepthTest)
    {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;

        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
    }


    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = stages;
    pipelineCreateInfo.pVertexInputState = &vertInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAsmStateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = enableDepthTest ? &depthStencil : nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto retFinal = vkCreateGraphicsPipelines(
            getLogicalDev(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipeline);
    vkDestroyShaderModule(getLogicalDev(), vertShader, nullptr);
    vkDestroyShaderModule(getLogicalDev(), fragShader, nullptr);

    return retFinal;
}

GraphicsPipeline::GraphicsPipeline(
        VkDevice* device,
        VkPhysicalDevice const& physDev,
        VkCommandPool* cmdPool,
        std::string const& vertShader,
        std::string const& fragShader,
        VkExtent2D const& extent,
        size_t const& swpchainImgCount,
        VkRenderPass const& renderPass,
        std::vector<VkDescriptorSetLayout> const& descriptorSetLayout,
        bool enableDepthTest) :
        AVkGraphicsBase(device), cmdPool(cmdPool)
{
    CHECK_VK_SUCCESS(
            createGraphicsPipeline(
                    vertShader, fragShader, extent, renderPass, descriptorSetLayout,
                    enableDepthTest),
            ErrorMessages::CREATE_GRAPHICS_PIPELINE_FAILED);

    CHECK_VK_SUCCESS(
            createCmdBuffers(swpchainImgCount),
            ErrorMessages::CREATE_COMMAND_BUFFERS_FAILED);
}

VkResult GraphicsPipeline::createCmdBuffers(size_t const& swpchainImgCount)
{
    cmdBuffers.resize(swpchainImgCount);
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = *cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // 1 command buffer per frame buffer
    allocateInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());

    return vkAllocateCommandBuffers(getLogicalDev(), &allocateInfo, cmdBuffers.data());
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& graphicspipeline) noexcept:
        AVkGraphicsBase(std::move(graphicspipeline)),
        pipeline(std::move(graphicspipeline.pipeline)),
        pipelineLayout(std::move(graphicspipeline.pipelineLayout)),
        cmdBuffers(std::move(graphicspipeline.cmdBuffers))
{
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& graphicspipeline) noexcept
{
    if (initialized())
    {
        vkFreeCommandBuffers(getLogicalDev(), *cmdPool,
                             static_cast<uint32_t>(cmdBuffers.size()),
                             cmdBuffers.data());
        vkDestroyPipeline(getLogicalDev(), pipeline, nullptr);
        vkDestroyPipelineLayout(getLogicalDev(), pipelineLayout, nullptr);
    }

    pipeline = std::move(graphicspipeline.pipeline);
    pipelineLayout = std::move(graphicspipeline.pipelineLayout);
    cmdBuffers = std::move(graphicspipeline.cmdBuffers);
    AVkGraphicsBase::operator=(std::move(graphicspipeline));
    return *this;
}



GraphicsPipeline::~GraphicsPipeline()
{
    if (initialized())
    {
        vkFreeCommandBuffers(getLogicalDev(), *cmdPool,
                             static_cast<uint32_t>(cmdBuffers.size()),
                             cmdBuffers.data());
        vkDestroyPipeline(getLogicalDev(), pipeline, nullptr);
        vkDestroyPipelineLayout(getLogicalDev(), pipelineLayout, nullptr);
        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
    }
}