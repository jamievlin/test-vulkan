#include "Window.h"
#include "DisposableCmdBuffer.h"
#include "helpers.h"

#include <utility>
#include <chrono>

#if defined(ENABLE_VALIDATION_LAYERS)
#include "validationTargets.h"
#include "dbgCallBacks.h"
#endif

const std::vector<char const*> requiredDevExtension = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<Vertex> verts = {
        {{0.f, 0.f}, {1.f, 0.f, 0.f}},
        {{0.5f, 0.f}, {0.f, 1.f, 0.f}},
        {{0.f, 0.5f}, {0.f, 0.f, 1.f}},
        {{0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}},
};

const std::vector<uint32_t> idx = {
        0,1,2,
        1,3,2
};

std::vector<char const*> getRequiredExts()
{
    uint32_t glfwExtCount = 0;
    char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtCount);

#if ENABLE_VALIDATION_LAYERS == 1
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

std::vector<char const*> getRequiredDeviceExts()
{
    std::vector<char const*> extensions;
    std::copy(
            requiredDevExtension.begin(), requiredDevExtension.end(),
            std::back_inserter(extensions));

    return extensions;
}

bool checkDeviceExtensionSupport(
        VkPhysicalDevice const& dev,
        std::vector<char const*> const& requiredExts=requiredDevExtension)
{
    uint32_t extCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, extensions.data());

    std::unordered_set<std::string> extNames;
    for (auto const& ext : extensions)
    {
        extNames.emplace(ext.extensionName);
    }

    bool deviceSupported=true;

    for (auto const& extName : requiredExts)
    {
        if (extNames.find(std::string(extName)) == extNames.end())
        {
            deviceSupported = false;
            break;
        }
    }

    return deviceSupported;
}

bool deviceSuitable(VkPhysicalDevice const& dev)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(dev, &properties);
    vkGetPhysicalDeviceFeatures(dev, &features);

    return checkDeviceExtensionSupport(dev) && features.tessellationShader && features.geometryShader;
}

Window::Window(
        size_t const& width,
        size_t const& height,
        std::string windowTitle,
        float const& fov, float const& clipnear, float const& clipfar) :
    width(width), height(height), title(std::move(windowTitle)),
    fovDegrees(fov), clipNear(clipnear), clipFar(clipfar),
    projectMat(glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(width) / static_cast<float>(height),
            clipNear, clipFar))
{
    glfwInit();

    // window creation
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(
            static_cast<int>(width), static_cast<int>(height),
            title.c_str(), nullptr, nullptr);
    initCallbacks();

    // instance creation
    CHECK_VK_SUCCESS(initInstance(), ErrorMessages::FAILED_CANNOT_CREATE_INSTANCE);
    // setup messenger
#if ENABLE_VALIDATION_LAYERS == 1
    CHECK_VK_SUCCESS(setupDebugMessenger(), "Cannot set up debug messenger!");
#endif
    CHECK_VK_SUCCESS(createSurface(), ErrorMessages::FAILED_CANNOT_CREATE_SURFACE);
    dev = selectPhysicalDev();
    CHECK_VK_SUCCESS(createLogicalDevice(), ErrorMessages::FAILED_CANNOT_CREATE_LOGICAL_DEV);

    // end selection of logical device

    // create allocator
    CHECK_VK_SUCCESS(createAllocator(), ErrorMessages::FAILED_CANNOT_CREATE_ALLOCATOR)
    //

    swapchainComponent = std::make_unique<SwapchainComponents>(
            &logicalDev, dev,
            surface, std::make_pair(this->width, this->height));

    CHECK_VK_SUCCESS(createCommandPool(), ErrorMessages::CREATE_COMMAND_POOL_FAILED);
    CHECK_VK_SUCCESS(createTransferCmdPool(), "Cannot create transfer command pool!");

    uniformData = std::make_unique<SwapchainImageBuffers>(
            &logicalDev, &allocator, dev, *swapchainComponent, 0
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

    // for vertex buffer
    initBuffers();
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

#if defined(ENABLE_VALIDATION_LAYERS)
VkDebugUtilsMessengerCreateInfoEXT Window::createDebugInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {};
    dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgInfo.pfnUserCallback = debugCallback;
    dbgInfo.pUserData = reinterpret_cast<void*>(this);

    return dbgInfo;
}


VkResult Window::setupDebugMessenger()
{
    auto createInfo = createDebugInfo();
    auto createDbgFn = getVkExtension<
            PFN_vkCreateDebugUtilsMessengerEXT,
            VkInstance, VkDebugUtilsMessengerCreateInfoEXT const*,
            VkAllocationCallbacks*,
            VkDebugUtilsMessengerEXT*
            >("vkCreateDebugUtilsMessengerEXT");
    return createDbgFn(instance, &createInfo, nullptr, &this->dbgMessenger);
}
#endif

Window::~Window()
{
    idxBuffer.reset();
    vertexBuffer.reset();
    frameSemaphores.clear();
    graphicsPipeline.reset();

    uniformData.reset();
    swapchainComponent.reset();
    vkDestroyCommandPool(logicalDev, cmdTransferPool, nullptr);
    vkDestroyCommandPool(logicalDev, cmdPool, nullptr);

    vmaDestroyAllocator(allocator);
#if ENABLE_VALIDATION_LAYERS == 1
    auto destroyFn = getVkExtensionVoid<
            PFN_vkDestroyDebugUtilsMessengerEXT,
            VkInstance, VkDebugUtilsMessengerEXT,
            VkAllocationCallbacks const*
            >("vkDestroyDebugUtilsMessengerEXT");
    destroyFn(instance, dbgMessenger, nullptr);
#endif
    vkDestroyDevice(logicalDev, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkResult Window::initInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan!";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.pEngineName = "What?";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.apiVersion = VK_API_VERSION;

    auto extNeeded = getRequiredExts();

    uint32_t extCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extProperties(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProperties.data());

    std::unordered_set<std::string> extNames;
    for (auto const& ext : extProperties)
    {
        extNames.emplace(ext.extensionName);
    }

    for (auto const& extName : extNeeded)
    {
        if (extNames.find(std::string(extName)) == extNames.end())
        {
            std::string msg = "Extension " + std::string(extName) + " not found";
            throw std::runtime_error(msg);
        }
    }

#ifdef DEBUG
    std::cerr << "All GLFW Extensions supported" << std::endl;
#endif

#if ENABLE_VALIDATION_LAYERS == 1
    if (not checkValidationSupport())
    {
        throw std::runtime_error("Validation layers not supported.");
    }

    auto dbgCreateInfo = createDebugInfo();
#endif

    VkInstanceCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if ENABLE_VALIDATION_LAYERS == 1
    createInfo.pNext = &dbgCreateInfo;
#else
    createInfo.pNext = nullptr;
#endif
    createInfo.pApplicationInfo = &appInfo;
#if ENABLE_VALIDATION_LAYERS == 1
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extNeeded.size());
    createInfo.ppEnabledExtensionNames = extNeeded.data();

    return vkCreateInstance(&createInfo, nullptr, &instance);
}

// assume little endian here - x86+ARM are both little endian
#define NVIDIA_VENDOR_ID 0x000010DE
#define AMD_VENDOR_ID 0x00001002

bool GPUHigherPriority(VkPhysicalDevice const& pd1, VkPhysicalDevice const& pd2)
{
    if (pd2 == VK_NULL_HANDLE)
    {
        return true;
    }

    if (pd1 == pd2)
    {
        return false;
    }

    VkPhysicalDeviceProperties dv1 = {}, dv2 = {};
    vkGetPhysicalDeviceProperties(pd1, &dv1);
    vkGetPhysicalDeviceProperties(pd2, &dv2);

    // mostly heuristic here
    // try to avoid software GPU
    bool dv1IsNVIDIAorAMD = dv1.vendorID == NVIDIA_VENDOR_ID || dv1.vendorID == AMD_VENDOR_ID;
    bool dv21IsNVIDIAorAMD = dv2.vendorID == NVIDIA_VENDOR_ID || dv2.vendorID == AMD_VENDOR_ID;
    if (dv1IsNVIDIAorAMD && !dv21IsNVIDIAorAMD)
    {
        return true;
    }

    return false;
}

VkPhysicalDevice Window::selectPhysicalDev()
{
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0)
    {
        throw std::runtime_error("There is no GPU capable of Vulkan!");
    }

    std::vector<VkPhysicalDevice> devs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, devs.data());

    for (auto const& devList : devs)
    {

        QueueFamilies fam(devList, surface);
        if (deviceSuitable(devList) && fam.suitable() && GPUHigherPriority(devList, physDev))
        {
            physDev = devList;
        }
    }

    if (physDev == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Could not find a Vulkan-capable GPU!");
    }

    return physDev;
}

VkResult Window::createLogicalDevice()
{
    queueFamilyIndex = QueueFamilies(dev, surface);
    if (not queueFamilyIndex.suitable())
    {
        throw std::runtime_error("Cannot create device queue on GPU.");
    }


    std::vector<VkDeviceQueueCreateInfo> queueCreateList;

    float priority = 1.f;
    VkDeviceQueueCreateInfo createQueueInfo = {};
    createQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    createQueueInfo.queueFamilyIndex = queueFamilyIndex.graphicsFamily.value();
    createQueueInfo.queueCount = 1;
    createQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo presentationQueueInfo = {};
    presentationQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentationQueueInfo.queueFamilyIndex = queueFamilyIndex.presentationFamily.value();
    presentationQueueInfo.queueCount = 1;
    presentationQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo transferQueueInfo = {};
    transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transferQueueInfo.queueFamilyIndex = queueFamilyIndex.transferQueueFamily();
    transferQueueInfo.queueCount = 1;
    transferQueueInfo.pQueuePriorities = &priority;

    VkDeviceQueueCreateInfo queues[] = {createQueueInfo, presentationQueueInfo, transferQueueInfo};

    VkPhysicalDeviceFeatures feat = {};

    auto deviceExts = getRequiredDeviceExts();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 3;
    createInfo.pQueueCreateInfos = queues;
    createInfo.pEnabledFeatures = &feat;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    createInfo.ppEnabledExtensionNames = deviceExts.data();

    VkResult result = vkCreateDevice(dev, &createInfo, nullptr, &logicalDev);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.presentationFamily.value(), 0, &presentQueue);
    vkGetDeviceQueue(logicalDev, queueFamilyIndex.transferQueueFamily(), 0, &transferQueue);


    return result;
}

VkResult Window::createSurface()
{
#if defined(__linux__)
    return glfwCreateWindowSurface(instance, window, nullptr, &surface);
#else
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(window);
    createInfo.hinstance = GetModuleHandle(nullptr);

    return vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#endif
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

        VkBuffer vertBuffers[] = { vertexBuffer->vertexBuffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, idxBuffer->vertexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
            &logicalDev, &allocator, dev, *swapchainComponent, 0
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

void Window::initCallbacks()
{
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, Window::onWindowSizeChange);
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

    vertexBuffer = std::make_unique<Buffers::VertexBuffer<Vertex>>(
            &logicalDev, &allocator, dev, verts.size(),
            queueFamilyIndex.queuesForTransfer(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffers::StagingBuffer stagingBufferIdx(
            &logicalDev, &allocator, dev, idx.size() * sizeof(uint32_t),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            queueFamilyIndex.queuesForTransfer());

    stagingBufferIdx.loadData(idx.data());

    idxBuffer = std::make_unique<Buffers::IndexBuffer>(
            &logicalDev, &allocator, dev, idx.size(),
            queueFamilyIndex.queuesForTransfer(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    helpers::img_r8g8b8a8 image = helpers::fromPng(helpers::searchPath("assets/square_floor_diff_2k.png"));
    Buffers::StagingBuffer imageStgBuffer(
            &logicalDev, &allocator, dev, image.totalSize(),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            queueFamilyIndex.queuesForTransfer());
    imageStgBuffer.loadData(image.imgData.data());



    // submit in one batch
    DisposableCmdBuffer dcb(&logicalDev, &cmdTransferPool);
    vertexBuffer->cmdCopyDataFrom(stagingBuffer, dcb.commandBuffer());
    idxBuffer->cmdCopyDataFrom(stagingBufferIdx, dcb.commandBuffer());
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

VkResult Window::createAllocator()
{
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.vulkanApiVersion = VK_API_VERSION;
    createInfo.physicalDevice = dev;
    createInfo.device = logicalDev;
    createInfo.instance = instance;

    return vmaCreateAllocator(&createInfo, &allocator);
}