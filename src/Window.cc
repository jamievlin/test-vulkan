//
// Created by Supakorn on 9/27/2021.
//
#include "Window.h"
#include "WindowBase.h"
#include "DisposableCmdBuffer.h"
#include "helpers.h"
#include "Mesh.h"

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
    mesh = Mesh(&logicalDev, &allocator, &dev, helpers::searchPath("assets/teapot.obj"));

    // for vertex buffer
    initBuffers();

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, img, 0
    );

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
            &logicalDev, dev, &cmdPool,
            helpers::searchPath("main.vert.spv"), helpers::searchPath("main.frag.spv"),
            swapchainComponent->swapchainExtent, swapchainComponent->imageCount(),
            swapchainComponent->renderPass,
            std::vector<VkDescriptorSetLayout> {
                uniformData->descriptorSetLayout,
                uniformData->meshDescriptorSetLayout}, true);

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frameSemaphores.emplace_back(&logicalDev);
    }

    uniformData->configureMeshBuffers(0, *meshUniformGroup);


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

    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipeline);

    VkDescriptorSet descSets[2] = {uniformData->descriptorSets[imageIdx],
                                   uniformData->meshDescriptorSets[imageIdx]};

    meshUniformGroup->beginFenceGroup(imageIdx, submissionFence);

    // setting new uniform
    glm::mat4 baseMat = glm::scale(glm::transpose(glm::mat4(
            0, 0, 1, 0,
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
    )), glm::vec3(0.15f));

    MeshUniform unif(glm::rotate(baseMat, totalTime / 1000.0f, glm::vec3(0,1,0)));
    unif.baseColor = glm::vec4(1,0.5, 0, 1);
    uint32_t offset_val = meshUniformGroup->placeNextData(unif);
    uint32_t offsetvals[1] = { offset_val };

    MeshUniform unif2(glm::translate(glm::rotate(baseMat, -totalTime / 1000.0f, glm::vec3(0,1,0))
            , glm::vec3(0,-3,0)));
    unif2.baseColor = glm::vec4(0,1, 0, 1);
    uint32_t offset_val2 = meshUniformGroup->placeNextData(unif2);
    uint32_t offsetvals2[1] = { offset_val2 };

    vkCmdBindDescriptorSets(
            cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->pipelineLayout,
            0,
            2, descSets,
            1, offsetvals);

    VkBuffer vertBuffers[] = { mesh.buf.vertexBuffer };
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, mesh.buf.vertexBuffer, mesh.idxOffset(), VK_INDEX_TYPE_UINT32);
    // actual drawing command :)
    // vkCmdDraw(cmdBuf, vertexBuffer->getSize(), 1, 0, 0);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(mesh.idxCount()), 1, 0, 0, 0);

    vkCmdBindDescriptorSets(
            cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->pipelineLayout,
            1,
            1, &uniformData->meshDescriptorSets[imageIdx],
            1, offsetvals2);
    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(mesh.idxCount()), 1, 0, 0, 0);
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

    vkResetCommandBuffer(graphicsPipeline->cmdBuffers[imgIndex], 0);
    recordCmd(imgIndex, inFlightFence);

    // wait then for img to become available
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSems[] = { imgAvailable };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

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

    setUniforms((*uniformData)[imgIndex].first);
    setLights(uniformData->lightSBOs[imgIndex]);


    vkResetFences(logicalDev, 1, &inFlightFence);
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

    auto meshStgBuffer = mesh.stagingBuffer(queueFamilyIndex.queuesForTransfer());

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
    mesh.buf.cmdCopyDataFrom(*meshStgBuffer, dcb.commandBuffer());
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

    glm::mat4 baseMat = glm::scale(glm::transpose(glm::mat4(
            0, 0, 1, 0,
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
    )), glm::vec3(0.15f));
    ubo.time = totalTime / 1000.f;


    ubo.view = glm::lookAt(
            cameraPos,
            O,
            glm::vec3(1.f, -1.f, -1.f));
    ubo.proj = projectMat;
    ubo.cameraPos = glm::vec4(cameraPos, 1);
    CHECK_VK_SUCCESS(bufObject.loadData(ubo), "Cannot set uniforms!");
}

void Window::setLights(StorageBufferArray<Light>& storageObj)
{
    float t = sin(totalTime / 500);

    std::vector<Light> li(2);
    li[0].lightType = LightType::POINT_LIGHT;
    li[0].position = glm::vec4(t,2,2,1);
    li[0].color = glm::vec4(1,1,1,1) * (t + 1);
    li[0].intensity = 5.f;

    li[1].lightType = LightType::POINT_LIGHT;
    li[1].position = glm::vec4(2,t,3,1);
    li[1].color = glm::vec4(0,1,0,1);
    li[1].intensity = 2.f;

    storageObj.loadDataAndSetSize(li);
}

void Window::updateFrame(float const& deltaTime)
{
    totalTime += deltaTime;
}