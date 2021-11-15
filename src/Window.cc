//
// Created by Supakorn on 9/27/2021.
//
#include "Window.h"
#include "WindowBase.h"
#include "DisposableCmdBuffer.h"
#include "helpers.h"
#include "Mesh.h"

#include <memory>
#include <utility>
#include <chrono>

Window::Window(size_t const& width,
               size_t const& height,
               std::string windowTitle,
               float const& fov, float const& clipnear, float const& clipfar) :
        WindowBase(width, height, std::move(windowTitle)),
        fovDegrees(fov), clipNear(clipnear), clipFar(clipfar),
        projectMat(glm::perspective(
                glm::radians(fovDegrees),
                static_cast<float>(width) / static_cast<float>(height),
                clipNear, clipFar)),
        cameraPos(1.f, -1.f, 1.f)
{
    initCallbacks();


    depthBuffer = Image::Image(
            &logicalDev, &allocator, size(),
            Image::findDepthFormat(dev),
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            nullopt,
            nullopt,
            VK_IMAGE_TILING_OPTIMAL, 1, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_ASPECT_DEPTH_BIT);

    swapchainComponent = std::make_unique<SwapchainComponents>(
            &logicalDev, dev,
            surface, size(), depthBuffer.imgView);

    CHECK_VK_SUCCESS(createCommandPool(), ErrorMessages::CREATE_COMMAND_POOL_FAILED);
    CHECK_VK_SUCCESS(createTransferCmdPool(), "Cannot create transfer command pool!");

    meshStorage.emplace("teapot", std::make_unique<Mesh>(
            &logicalDev, &allocator, &dev, helpers::searchPath("assets/teapot.obj")
            ));

    meshStorage.emplace("plane", std::make_unique<Mesh>(
            &logicalDev, &allocator, &dev, helpers::searchPath("assets/plane.obj")
            ));

    // drawables

    glm::mat4 baseMat = glm::scale(glm::transpose(glm::mat4(
            0, 0, 1, 0,
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
    )), glm::vec3(0.15f));

    drawables.emplace_back(meshStorage["teapot"].get(), baseMat);
    drawables.emplace_back(meshStorage["plane"].get(), baseMat);

    drawables[0].uniform.baseColor = glm::vec4(1,1,1,1);
    drawables[1].uniform.baseColor = glm::vec4(0.6,0.2,0.45,1);
    drawables[0].uniform.params = glm::vec4(0.15,0,0.04,0);
    drawables[1].uniform.params = glm::vec4(0.35,0,0.04,0);

    // init light+shadowmap desc sets
    lightShadowmapDesc.lightStorage =
            std::make_unique<StorageBufferArray<Light>>(
                            &logicalDev, &allocator, dev, 64,
                            nullopt, 0,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    createShadowmapsDescPool();
    createShadowmapsDescSets();

    // for vertex buffer
    initBuffers();

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, img, 0
    );

    std::vector<VkDescriptorSetLayout> shadowDescSetLayout = {
            uniformData->descriptorSetLayout,
            uniformData->meshDescriptorSetLayout};

    shadowmapGraphicsPipeline = std::make_unique<ShadowmapPipeline>(
            &logicalDev, &allocator, dev,
            helpers::searchPath("shadowmap_dir.vert.spv"),
            1024,
            swapchainComponent->imageCount(),
            shadowDescSetLayout,
            cmdPool);

    std::vector<VkDescriptorSetLayout> mainPipelineDescSetsLayout = {
            uniformData->descriptorSetLayout,
            uniformData->meshDescriptorSetLayout,
            lightShadowmapDesc.lightsAndShadowmapsLayout
    };

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
            &logicalDev, dev, &cmdPool,
            helpers::searchPath("main.vert.spv"), helpers::searchPath("main.frag.spv"),
            swapchainComponent->swapchainExtent, swapchainComponent->imageCount(),
            swapchainComponent->renderPass,
            mainPipelineDescSetsLayout,
            true);

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frameSemaphores.emplace_back(&logicalDev);
    }
    // shadowmapGraphicsPipeline->configureBuffers(uniformData->meshDescriptorSets, *meshUniformGroup);
    // uniformData->configureBuffers(0, shadowmapGraphicsPipeline->depthTargets[0]);
    uniformData->configureMeshBuffers(0, *meshUniformGroup);

    updateShadowmapBuffer();
}

int Window::mainLoop()
{
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (not glfwWindowShouldClose(window))
    {
        float timepassed = 0;
        auto newTime = std::chrono::high_resolution_clock::now();
        if (running)
        {
            timepassed = std::chrono::duration<float, std::milli>(newTime - lastTime).count();
        }
        running = true;
        updateFrame(timepassed);
        glfwPollEvents();
        drawFrame();

        lastTime = newTime;
    }

    vkDeviceWaitIdle(logicalDev);
    return 0;
}

Window::~Window()
{
    lightShadowmapDesc.lightStorage.reset();
    vkDestroyDescriptorSetLayout(logicalDev, lightShadowmapDesc.lightsAndShadowmapsLayout, nullptr);
    vkDestroyDescriptorPool(logicalDev, lightShadowmapDesc.lightsSmapDescPool, nullptr);

    shadowmapGraphicsPipeline.reset();
    meshUniformGroup.reset();
    graphicsPipeline.reset();
    swapchainComponent.reset();

    vkDestroyCommandPool(logicalDev, cmdTransferPool, nullptr);
    vkDestroyCommandPool(logicalDev, cmdPool, nullptr);
}

void Window::initCallbacks()
{
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, Window::onWindowSizeChange);
}

void Window::recordShadowmapCmd(VkCommandBuffer& smapBuf, uint32_t imageIdx, VkFence& submissionFence)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    CHECK_VK_SUCCESS(
            vkBeginCommandBuffer(smapBuf, &beginInfo),
            "Failed to begin buffer recording!");

    VkRenderPassBeginInfo smapRenderPassBeginInfo = {};
    smapRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    smapRenderPassBeginInfo.renderPass = shadowmapGraphicsPipeline->renderPass;
    smapRenderPassBeginInfo.framebuffer = shadowmapGraphicsPipeline->shadowmapFramebuffer;
    smapRenderPassBeginInfo.renderArea.offset = {0,0};
    smapRenderPassBeginInfo.renderArea.extent.width = 1024;
    smapRenderPassBeginInfo.renderArea.extent.height = 1024;

    VkClearValue smapClearValue[1];
    // smapClearValue[0].color = {0.f, 0.f, 0.f, 1.f};
    smapClearValue[0].depthStencil = {1.0f, 0};

    smapRenderPassBeginInfo.clearValueCount = 1;
    smapRenderPassBeginInfo.pClearValues = smapClearValue;

    vkCmdBeginRenderPass(smapBuf, &smapRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(smapBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmapGraphicsPipeline->pipeline);

    std::vector<uint32_t> offsets;

    for (auto& drawable : drawables)
    {
        uint32_t offset_val = meshUniformGroup->placeNextData(drawable.uniform);
        offsets.push_back(offset_val);
    }

    VkDescriptorSet descSets[1] = {uniformData->descriptorSets[imageIdx]};
    vkCmdBindDescriptorSets(
            smapBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadowmapGraphicsPipeline->pipelineLayout,
            0,
            1, descSets,
            0, nullptr);

    for (int i = 0; i < drawables.size(); ++i)
    {
        Mesh& mesh = drawables[i].getMesh();

        vkCmdBindDescriptorSets(
                smapBuf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                shadowmapGraphicsPipeline->pipelineLayout,
                1,
                1, &uniformData->meshDescriptorSets[imageIdx],
                1, offsets.data() + i);

        VkBuffer vertBuffers[] = { mesh.buf.vertexBuffer };

        VkDeviceSize vertoffsets[1] = {0};
        vkCmdBindVertexBuffers(smapBuf, 0, 1, vertBuffers, vertoffsets);
        vkCmdBindIndexBuffer(smapBuf, mesh.buf.vertexBuffer, mesh.idxOffset(), VK_INDEX_TYPE_UINT32);
        // actual drawing command :)
        // vkCmdDraw(cmdBuf, vertexBuffer->getSize(), 1, 0, 0);
        vkCmdDrawIndexed(smapBuf, CAST_UINT32(mesh.idxCount()), 1, 0, 0, 0);
    }

    /*
    shadowmapGraphicsPipeline->depthTargets[imageIdx].cmdTransitionLayout(
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            smapBuf,
            VK_IMAGE_ASPECT_DEPTH_BIT);
            */

    vkCmdEndRenderPass(smapBuf);
    vkEndCommandBuffer(smapBuf);
    // vkEndCommandBuffer(smapBuf.commandBuffer());
}

void Window::recordCmd(uint32_t imageIdx, VkFence& submissionFence)
{
    auto& cmdBuf = graphicsPipeline->cmdBuffers[imageIdx];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    CHECK_VK_SUCCESS(
            vkBeginCommandBuffer(cmdBuf, &beginInfo),
            "Failed to begin buffer recording!");

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = swapchainComponent->renderPass;
    renderPassBeginInfo.framebuffer = swapchainComponent->swapchainSupport[imageIdx].frameBuffer;
    renderPassBeginInfo.renderArea.offset = {0,0};
    renderPassBeginInfo.renderArea.extent = swapchainComponent->swapchainExtent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassBeginInfo.clearValueCount = CAST_UINT32(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipeline);

    std::vector<uint32_t> offsets;
    for (auto& drawable : drawables)
    {
        uint32_t offset_val = meshUniformGroup->placeNextData(drawable.uniform);
        offsets.push_back(offset_val);
    }


    vkCmdBindDescriptorSets(
            cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->pipelineLayout,
            0,
            1, &uniformData->descriptorSets[imageIdx],
            0, nullptr);

    vkCmdBindDescriptorSets(
            cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->pipelineLayout,
            2,
            1, &lightShadowmapDesc.lightAndShadowmapsDescSets,
            0, nullptr);


    for (int i = 0; i < drawables.size(); ++i)
    {
        Mesh& mesh = drawables[i].getMesh();

        vkCmdBindDescriptorSets(
                cmdBuf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->pipelineLayout,
                1,
                1, &uniformData->meshDescriptorSets[imageIdx],
                1, offsets.data() + i);

        VkBuffer vertBuffers[] = { mesh.buf.vertexBuffer };
        VkDeviceSize vertoffsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, vertoffsets);
        vkCmdBindIndexBuffer(cmdBuf, mesh.buf.vertexBuffer, mesh.idxOffset(), VK_INDEX_TYPE_UINT32);
        // actual drawing command :)
        // vkCmdDraw(cmdBuf, vertexBuffer->getSize(), 1, 0, 0);
        vkCmdDrawIndexed(cmdBuf, CAST_UINT32(mesh.idxCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmdBuf);


    depthBuffer.cmdTransitionLayout(
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            cmdBuf,
            Image::hasStencilComponent(Image::findDepthFormat(dev)) ?
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
            VK_IMAGE_ASPECT_DEPTH_BIT);

    CHECK_VK_SUCCESS(
            vkEndCommandBuffer(cmdBuf),
            "Cannot end command buffer!");

}

void Window::drawFrame()
{
    // code goes here
    uint32_t imgIndex;
    VkSemaphore& imgAvailable = frameSemaphores[currentFrame].imgAvailable;
    VkSemaphore& renderFinished = frameSemaphores[currentFrame].imgAvailable;
    VkSemaphore& smapFinished = frameSemaphores[currentFrame].renderFinished;
    VkFence& inFlightFence = frameSemaphores[currentFrame].inFlight;

    vkWaitForFences(logicalDev, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult nextImgResult = vkAcquireNextImageKHR(
            logicalDev, swapchainComponent->swapChain, UINT64_MAX,
            imgAvailable, VK_NULL_HANDLE, &imgIndex);

    if (nextImgResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resetSwapChain();
        return;
    }
    else if (nextImgResult != VK_SUCCESS && nextImgResult != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Cannot acquire swap chain image!");
    }

    // if this specific image has been rendered in the previous frame,
    // wait for it.

    // If there is multiple in-flight frames per a single image,
    // this will also check for /that/ frame as well, and
    // switch to the current fence.
    VkFence& imgIdxFence = swapchainComponent->swapchainSupport[imgIndex].imagesInFlight;
    if (imgIdxFence != VK_NULL_HANDLE)
    {
        vkWaitForFences(logicalDev, 1, &imgIdxFence, VK_TRUE, UINT64_MAX);
    }
    imgIdxFence = inFlightFence;
    // at this point, image is fully ours.
    meshUniformGroup->beginFenceGroup(imgIndex, inFlightFence);

    // primary drawing
    vkResetCommandBuffer(graphicsPipeline->cmdBuffers[imgIndex], 0);
    vkResetCommandBuffer(shadowmapGraphicsPipeline->smapCmdBuffer[imgIndex], 0);

    recordShadowmapCmd(shadowmapGraphicsPipeline->smapCmdBuffer[imgIndex], imgIndex, inFlightFence);
    recordCmd(imgIndex, inFlightFence);

    setUniforms((*uniformData)[imgIndex].first);
    setLights(*lightShadowmapDesc.lightStorage);

    VkSubmitInfo smapSubmitInfo = {};
    smapSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore smapWaitSems[] = { imgAvailable };
    VkPipelineStageFlags smapWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    smapSubmitInfo.waitSemaphoreCount = 1;
    smapSubmitInfo.pWaitSemaphores = smapWaitSems;
    smapSubmitInfo.pWaitDstStageMask = smapWaitStages;

    smapSubmitInfo.commandBufferCount = 1;
    smapSubmitInfo.pCommandBuffers = &shadowmapGraphicsPipeline->smapCmdBuffer[imgIndex];

    VkSemaphore smapSignals[] = { smapFinished };
    smapSubmitInfo.signalSemaphoreCount = 1;
    smapSubmitInfo.pSignalSemaphores = smapSignals;

    vkResetFences(logicalDev, 1, &inFlightFence);
    vkQueueSubmit(graphicsQueue, 1, &smapSubmitInfo, nullptr);

    // wait then for img to become available
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSems[] = { smapFinished };
    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSems;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = graphicsPipeline->cmdBuffers.data() + imgIndex;

    VkSemaphore signals[] = { renderFinished };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signals;

    // reset here, as inFlightFence could be the same as imgIdxFence, in which case
    // we would have resetted /before/ waiting for fence again, which causes
    // an infinite wait (as there's nothing to render and /signal/ the fence).


    CHECK_VK_SUCCESS(
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence),
            "Cannot submit draw queue!");

    // presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signals;

    VkSwapchainKHR chains[] = { swapchainComponent->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = chains;
    presentInfo.pImageIndices = &imgIndex;
    presentInfo.pResults = nullptr;

    auto presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        resetSwapChain();
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("Cannot present image!");
    }

    // vkDestroySemaphore(logicalDev, shadowMapFinished, nullptr);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Window::resetSwapChain()
{
    running = false;
    while (this->width == 0 && this->height == 0)
    {
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(logicalDev);

    depthBuffer = Image::Image(
            &logicalDev, &allocator, size(),
            Image::findDepthFormat(dev),
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            nullopt,
            nullopt,
            VK_IMAGE_TILING_OPTIMAL, 1, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_ASPECT_DEPTH_BIT);

    graphicsPipeline.reset();
    uniformData.reset();
    swapchainComponent.reset();

    swapchainComponent = std::make_unique<SwapchainComponents>(
            &logicalDev, dev,
            surface, std::make_pair(this->width, this->height),
            depthBuffer.imgView);

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, img, 0
    );

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
            &logicalDev, dev, &cmdPool,
            helpers::searchPath("main.vert.spv"), helpers::searchPath("main.frag.spv"),
            swapchainComponent->swapchainExtent, swapchainComponent->imageCount(),
            swapchainComponent->renderPass,
            std::vector<VkDescriptorSetLayout> {uniformData->descriptorSetLayout, uniformData->meshDescriptorSetLayout},
            true);

    uniformData->configureMeshBuffers(0, *meshUniformGroup);
}

// static
void Window::onWindowSizeChange(GLFWwindow* ptr, int width, int height)
{
    auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(ptr));
    self->width = width;
    self->height = height;

    if (height != 0)
    {
        self->projectMat = glm::perspective(
                glm::radians(self->fovDegrees),
                static_cast<float>(width) / static_cast<float>(height),
                self->clipNear, self->clipFar);
    }
}


VkResult Window::createCommandPool()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueFamilyIndex.graphicsFamily.value();
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    return vkCreateCommandPool(logicalDev, &createInfo, nullptr, &cmdPool);
}

VkResult Window::createTransferCmdPool()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueFamilyIndex.transferQueueFamily();
    createInfo.flags = 0;

    return vkCreateCommandPool(logicalDev, &createInfo, nullptr, &cmdTransferPool);
}

void Window::initBuffers()
{
    meshUniformGroup = std::make_unique<DynUniformObjBuffer<MeshUniform>>(
            &logicalDev, &allocator, dev,
            256,
            nullopt,
            0,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    helpers::img_r8g8b8a8 image = helpers::fromPng(helpers::searchPath("assets/smile.png"));
    Buffers::StagingBuffer imageStgBuffer(
            &logicalDev, &allocator, dev, image.totalSize(),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            queueFamilyIndex.queuesForTransfer());
    imageStgBuffer.loadData(image.imgData.data());

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


    img = Image::Image(
            &logicalDev, &allocator, image.size(),
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            samplerInfo);

    // submit in one batch
    DisposableCmdBuffer dcb(&logicalDev, &cmdTransferPool);

    std::vector<std::shared_ptr<Buffers::StagingBuffer>> meshStgBuffers;
    for (auto& [name, meshPtr] : meshStorage)
    {
        auto stagingPtr = meshPtr->stagingBuffer(queueFamilyIndex.queuesForTransfer());
        meshStgBuffers.push_back(stagingPtr);
        meshPtr->buf.cmdCopyDataFrom(stagingPtr->vertexBuffer, dcb.commandBuffer());
    }
    // vertexBuffer.cmdCopyDataFrom(stagingBuffer, dcb.commandBuffer());
    // idxBuffer.cmdCopyDataFrom(stagingBufferIdx, dcb.commandBuffer());
    img.cmdTransitionBeginCopy(dcb.commandBuffer());
    img.cmdCopyFromBuffer(imageStgBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dcb.commandBuffer());
    img.cmdTransitionEndCopy(dcb.commandBuffer());

    dcb.finish();
    CHECK_VK_SUCCESS(dcb.submit(transferQueue), ErrorMessages::FAILED_CANNOT_SUBMIT_QUEUE);
    CHECK_VK_SUCCESS(vkQueueWaitIdle(transferQueue), ErrorMessages::FAILED_WAIT_IDLE);
}

void Window::setUniforms(UniformObjBuffer<UniformObjects>& bufObject)
{
    glm::vec3 Zup(0,0,1);
    glm::vec3 Yup(0,1,0);
    glm::vec3 Xup(1,0,0);
    glm::vec3 O(0,0,0);

    UniformObjects ubo = {};
    ubo.time = totalTime / 1000.f;
    ubo.view = glm::lookAt(
            cameraPos,
            O,
            glm::vec3(1.f, -1.f, -1.f));
    ubo.proj = projectMat;
    ubo.cameraPos = glm::vec4(cameraPos, 1);

    glm::vec3 dir(1,0,-1);

    ubo.lightDirMatrix = glm::lookAt(
            -0.5f * dir,
            O,
            -Zup);
    CHECK_VK_SUCCESS(bufObject.loadData(ubo), "Cannot set uniforms!");
}

void Window::setLights(StorageBufferArray<Light>& storageObj)
{
    float t = sin(totalTime / 500);

    std::vector<Light> li(3);
    li[0].lightType = LightType::POINT_LIGHT;
    li[0].position = glm::vec4(t,2,2,1);
    li[0].color = glm::vec4(1,1,1,1) * (t + 1);
    li[0].intensity = 5.f;

    li[1].lightType = LightType::POINT_LIGHT;
    li[1].position = glm::vec4(2,t,3,1);
    li[1].color = glm::vec4(0,1,0,1);
    li[1].intensity = 2.f;

    li[2].lightType = LightType::DIRECTIONAL_LIGHT;
    li[2].position = glm::vec4(1,0,-1,0);
    li[2].color = glm::vec4(1,1,0,1);
    li[2].intensity = 1.f;

    storageObj.loadDataAndSetSize(li);
}

void Window::updateFrame(float const& deltaTime)
{
    totalTime += deltaTime;
    // setting new uniform

    glm::mat4 baseMat = glm::scale(glm::transpose(glm::mat4(
            0, 0, 1, 0,
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
    )), glm::vec3(0.15f));

    drawables[0].uniform.setModelMatrix(glm::rotate(baseMat, -totalTime / 1000.0f, glm::vec3(0,1,0)));
}

void Window::createShadowmapsDescSets()
{
    auto layoutBinding = StorageBufferArray<Light>::DescriptorSetLayout(0);

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    CHECK_VK_SUCCESS(
            vkCreateDescriptorSetLayout(
                    logicalDev, &layoutInfo, nullptr, &lightShadowmapDesc.lightsAndShadowmapsLayout),
            "Cannot create Shadow map descriptor set layout!");


    VkDescriptorSetAllocateInfo VDSAcreateInfo = {};
    VDSAcreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    VDSAcreateInfo.descriptorPool = lightShadowmapDesc.lightsSmapDescPool;
    VDSAcreateInfo.descriptorSetCount = 1;
    VDSAcreateInfo.pSetLayouts = &lightShadowmapDesc.lightsAndShadowmapsLayout;

    CHECK_VK_SUCCESS(
            vkAllocateDescriptorSets(
                    logicalDev, &VDSAcreateInfo, &lightShadowmapDesc.lightAndShadowmapsDescSets),
            "Cannot create descriptor sets for shadow maps!"
    );
}

void Window::createShadowmapsDescPool()
{
    VkDescriptorPoolSize poolSize[1] = {{}};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = poolSize;
    createInfo.maxSets = 1;
    CHECK_VK_SUCCESS(
            vkCreateDescriptorPool(logicalDev, &createInfo, nullptr, &lightShadowmapDesc.lightsSmapDescPool),
            "Cannot create descriptor pools for shadow maps!"
    );
}

void Window::updateShadowmapBuffer()
{
    VkDescriptorBufferInfo sboBufferInfo = {};
    sboBufferInfo.buffer = lightShadowmapDesc.lightStorage->vertexBuffer;
    sboBufferInfo.offset = 0;
    sboBufferInfo.range = lightShadowmapDesc.lightStorage->getSize();

    VkWriteDescriptorSet descriptorWriteLight = {};
    descriptorWriteLight.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteLight.dstSet = lightShadowmapDesc.lightAndShadowmapsDescSets;
    descriptorWriteLight.dstBinding = 0;
    descriptorWriteLight.dstArrayElement = 0;
    descriptorWriteLight.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWriteLight.descriptorCount = 1;
    descriptorWriteLight.pBufferInfo = &sboBufferInfo;

    std::array<VkWriteDescriptorSet, 1> descSets = {descriptorWriteLight};

    vkUpdateDescriptorSets(
            logicalDev,
            descSets.size(), descSets.data(),
            0, nullptr);
}


