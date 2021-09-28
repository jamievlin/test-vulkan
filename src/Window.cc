//
// Created by Supakorn on 9/27/2021.
//
#include "Window.h"
#include "WindowBase.h"
#include "DisposableCmdBuffer.h"
#include "helpers.h"

#include <utility>
#include <chrono>

const std::vector<Vertex> verts = {
        {{0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f}},
        {{0.5f, 0.f}, {0.f, 1.f, 0.f}, {1.f, 0.f}},
        {{0.f, 0.5f}, {0.f, 0.f, 1.f}, {0.f, 1.f}},
        {{0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {1.f, 1.f}},
};

const std::vector<uint32_t> idx = {
        0,1,2,
        1,3,2
};

Window::Window(size_t const& width,
               size_t const& height,
               std::string windowTitle,
               float const& fov, float const& clipnear, float const& clipfar) :
        WindowBase(width, height, std::move(windowTitle)),
        fovDegrees(fov), clipNear(clipnear), clipFar(clipfar),
        projectMat(glm::perspective(
                glm::radians(fovDegrees),
                static_cast<float>(width) / static_cast<float>(height),
                clipNear, clipFar))
{
    initCallbacks();
    swapchainComponent = std::make_unique<SwapchainComponents>(
            &logicalDev, dev,
            surface, std::make_pair(this->width, this->height));

    CHECK_VK_SUCCESS(createCommandPool(), ErrorMessages::CREATE_COMMAND_POOL_FAILED);
    CHECK_VK_SUCCESS(createTransferCmdPool(), "Cannot create transfer command pool!");

    // for vertex buffer
    initBuffers();

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, img, 0
    );

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
            &logicalDev, dev, &cmdPool, surface,
            helpers::searchPath("main.vert.spv"), helpers::searchPath("main.frag.spv"),
            *swapchainComponent,
            std::vector<VkDescriptorSetLayout> {uniformData->descriptorSetLayout});

    for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frameSemaphores.emplace_back(&logicalDev);
    }
}

int Window::mainLoop()
{
    recordCommands();
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (not glfwWindowShouldClose(window))
    {
        auto newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = newTime - lastTime;

        updateFrame(duration.count());
        glfwPollEvents();
        drawFrame();

        lastTime = newTime;
    }

    vkDeviceWaitIdle(logicalDev);
    return 0;
}

Window::~Window()
{
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

void Window::recordCommands()
{
    //begin buffer recording
    for (size_t i=0; i < swapchainComponent->imageCount(); ++i)
    {
        auto const& cmdBuf = graphicsPipeline->cmdBuffers[i];

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
        renderPassBeginInfo.framebuffer = swapchainComponent->swapchainSupport[i].frameBuffer;

        renderPassBeginInfo.renderArea.offset = {0,0};
        renderPassBeginInfo.renderArea.extent = swapchainComponent->swapchainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipeline);

        VkBuffer vertBuffers[] = { vertexBuffer.vertexBuffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, idxBuffer.vertexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(
                cmdBuf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->pipelineLayout,
                0, 1, &(uniformData->descriptorSets[i]),
                0, nullptr);

        // actual drawing command :)
        // vkCmdDraw(cmdBuf, vertexBuffer->getSize(), 1, 0, 0);
        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(idx.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuf);

        CHECK_VK_SUCCESS(
                vkEndCommandBuffer(cmdBuf),
                "Cannot end command buffer!");

    }
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
    while (this->width == 0 && this->height == 0)
    {
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(logicalDev);

    graphicsPipeline.reset();
    uniformData.reset();
    swapchainComponent.reset();

    swapchainComponent = std::make_unique<SwapchainComponents>(
            &logicalDev, dev,
            surface, std::make_pair(this->width, this->height));

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, img, 0
    );

    graphicsPipeline = std::make_unique<GraphicsPipeline>(
            &logicalDev, dev, &cmdPool, surface,
            helpers::searchPath("main.vert.spv"), helpers::searchPath("main.frag.spv"),
            *swapchainComponent,
            std::vector<VkDescriptorSetLayout> {uniformData->descriptorSetLayout});

    recordCommands();
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
    createInfo.flags = 0;

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
    Buffers::StagingBuffer stagingBuffer(
            &logicalDev, &allocator, dev, verts.size() * sizeof(Vertex),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            queueFamilyIndex.queuesForTransfer());

    stagingBuffer.loadData(verts.data());

    vertexBuffer = Buffers::VertexBuffer<Vertex>(
            &logicalDev, &allocator, dev, verts.size(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffers::StagingBuffer stagingBufferIdx(
            &logicalDev, &allocator, dev, idx.size() * sizeof(uint32_t),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            queueFamilyIndex.queuesForTransfer());

    stagingBufferIdx.loadData(idx.data());

    idxBuffer = Buffers::IndexBuffer(
            &logicalDev, &allocator, dev, idx.size(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
            &logicalDev, &allocator, std::pair<uint32_t,uint32_t>(image.width, image.height),
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            samplerInfo);

    // submit in one batch
    DisposableCmdBuffer dcb(&logicalDev, &cmdTransferPool);

    vertexBuffer.cmdCopyDataFrom(stagingBuffer, dcb.commandBuffer());
    idxBuffer.cmdCopyDataFrom(stagingBufferIdx, dcb.commandBuffer());
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
    glm::vec3 Xup(1,0,0);
    glm::vec3 O(0,0,0);

    UniformObjects ubo = {};
    ubo.time = totalTime / 1000.f;
    ubo.model = glm::rotate(glm::mat4(1.f), totalTime / 1000.0f, Zup);
    ubo.view = glm::lookAt(
            glm::vec3(1.f, -1.f, 1.f),
            O,
            glm::vec3(1.f, -1.f, -1.f));
    ubo.proj = projectMat;

    CHECK_VK_SUCCESS(bufObject.loadData(ubo), "Cannot set uniforms!");
}

void Window::updateFrame(float const& deltaTime)
{
    totalTime += deltaTime;
}