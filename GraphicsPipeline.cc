//
// Created by Supakorn on 9/8/2021.
//

#include "GraphicsPipeline.h"
constexpr char const* CREATE_GRAPHICS_PIPELINE_FAILED = "Cannot create graphics pipeline!";
constexpr char const* CREATE_COMMAND_POOL_FAILED = "Cannot create command pool!";
constexpr char const* CREATE_COMMAND_BUFFERS_FAILED = "Cannot create command buffers!";

VkResult GraphicsPipeline::createGraphicsPipeline(std::string const& vertShaderName, std::string const& fragShaderName,
                                                  SwapchainComponents const& swapChain)
{
    auto [vertShader, ret] = Shaders::createShaderModule(*logicalDev, vertShaderName);
    auto [fragShader, ret2] = Shaders::createShaderModule(*logicalDev, fragShaderName);
    if (ret != VK_SUCCESS or ret2 != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create shader");
    }

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

    VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
    vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputInfo.vertexBindingDescriptionCount = 0;
    vertInputInfo.pVertexBindingDescriptions = nullptr;
    vertInputInfo.vertexAttributeDescriptionCount = 0;
    vertInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAsmStateInfo = {};
    inputAsmStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAsmStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChain.swapchainExtent.width;
    viewport.height=(float)swapChain.swapchainExtent.height;
    viewport.minDepth=0.0f;
    viewport.maxDepth=1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0,0};
    scissor.extent = swapChain.swapchainExtent;
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
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(
            *logicalDev, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)
        != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot create pipeline layout!");
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
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = swapChain.renderPass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto retFinal = vkCreateGraphicsPipelines(
            *logicalDev, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipeline);
    vkDestroyShaderModule(*logicalDev, vertShader, nullptr);
    vkDestroyShaderModule(*logicalDev, fragShader, nullptr);

    return retFinal;
}

GraphicsPipeline::GraphicsPipeline(
        VkDevice* device,
        VkPhysicalDevice const& physDev,
        VkCommandPool* cmdPool,
        VkSurfaceKHR const& surface,
        std::string const& vertShader,
        std::string const& fragShader,
        SwapchainComponents const& swapChain) :
        logicalDev(device), cmdPool(cmdPool)
{
    CHECK_VK_SUCCESS(
            createGraphicsPipeline(vertShader, fragShader, swapChain),
            CREATE_GRAPHICS_PIPELINE_FAILED);

    CHECK_VK_SUCCESS(
            createCmdBuffers(swapChain),
            CREATE_COMMAND_BUFFERS_FAILED);
}

VkResult GraphicsPipeline::createCmdBuffers(SwapchainComponents const& swapChain)
{
    cmdBuffers.resize(swapChain.swapChainImages.size());
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = *cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // 1 command buffer per frame buffer
    allocateInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());

    return vkAllocateCommandBuffers(*logicalDev, &allocateInfo, cmdBuffers.data());
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& graphicspipeline) noexcept:
        logicalDev(std::move(graphicspipeline.logicalDev))
{
    pipeline = std::move(graphicspipeline.pipeline);
    pipelineLayout = std::move(graphicspipeline.pipelineLayout);
    cmdBuffers = std::move(graphicspipeline.cmdBuffers);

    graphicspipeline.logicalDev = nullptr;
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& graphicspipeline) noexcept
{
    logicalDev = std::move(graphicspipeline.logicalDev);
    pipeline = std::move(graphicspipeline.pipeline);
    pipelineLayout = std::move(graphicspipeline.pipelineLayout);
    cmdBuffers = std::move(graphicspipeline.cmdBuffers);

    graphicspipeline.logicalDev = nullptr;

    return *this;
}



GraphicsPipeline::~GraphicsPipeline()
{
    if (logicalDev)
    {
        vkFreeCommandBuffers(*logicalDev, *cmdPool,
                             static_cast<uint32_t>(cmdBuffers.size()),
                             cmdBuffers.data());
        vkDestroyPipeline(*logicalDev, pipeline, nullptr);
        vkDestroyPipelineLayout(*logicalDev, pipelineLayout, nullptr);

        cmdBuffers.clear();
        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
    }
}

