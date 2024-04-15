#include "vulkan.h"
#include <vector>
#include <queue>
#include <format>
#include <SDL_vulkan.h>
#include "core/file.h"

void Vulkan::initialize(const char *applicationName, SDL_Window *window) {
    createInstance(applicationName, window);
    createDebugUtilsMessenger();
    selectBestPhysicalDevice();
    createSurface(window);
    createDevice();
    createSwapChain();
    createRenderPass();
    createPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

Vulkan::~Vulkan() {
    vkDeviceWaitIdle(device);
    
    vkDestroyFence(device, inFlightFence, allocationCallbacks);
    vkDestroySemaphore(device, imageAvailableSemaphore, allocationCallbacks);
    vkDestroySemaphore(device, renderFinishedSemaphore, allocationCallbacks);

    vkDestroyCommandPool(device, commandPool, allocationCallbacks);

    for (auto &frameBuffer: frameBuffers) {
        vkDestroyFramebuffer(device, frameBuffer, allocationCallbacks);
    }

    vkDestroyPipeline(device, pipeline, allocationCallbacks);
    vkDestroyRenderPass(device, renderPass, allocationCallbacks);
    vkDestroyPipelineLayout(device, pipelineLayout, allocationCallbacks);

    for (auto &shaderModule: shaderModules) {
        vkDestroyShaderModule(device, shaderModule, allocationCallbacks);
    }

    for (auto &imageView: imageViews) {
        vkDestroyImageView(device, imageView, allocationCallbacks);
    }

    auto debugUtilsDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    debugUtilsDestroyFunc(instance, debugUtilsMessenger, allocationCallbacks);

    vkDestroySwapchainKHR(device, swapChain, allocationCallbacks);
    vkDestroyDevice(device, allocationCallbacks);
    vkDestroySurfaceKHR(instance, surface, allocationCallbacks);
    vkDestroyInstance(instance, allocationCallbacks);
}

bool Vulkan::isInstanceLayerAvailable(const char *layerName) {
    uint32_t availableLayerCount;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr))
    std::vector<VkLayerProperties> availableLayers(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()))

    auto found = std::find_if(availableLayers.begin(), availableLayers.end(),
                              [layerName](VkLayerProperties properties) -> bool {
                                  return std::string(properties.layerName) == std::string(layerName);
                              });

    return found != availableLayers.end();
}

uint32_t Vulkan::getApiVersion() const {
    uint32_t apiVersion;
    VK_CHECK(vkEnumerateInstanceVersion(&apiVersion))
    std::cout << std::format("Vulkan version: {}.{}.{}.{}", VK_API_VERSION_MAJOR(apiVersion),
                             VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion),
                             VK_API_VERSION_VARIANT(apiVersion)) << std::endl;
    return apiVersion;
}

void Vulkan::createInstance(const char *applicationName, SDL_Window *window) {
    uint32_t apiVersion = getApiVersion();
    uint32_t availableLayerCount;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr))
    std::vector<VkLayerProperties> availableLayers(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()))

    std::vector<const char *> requestedLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_profiles"};
    std::vector<const char *> layers;
    for (const char *layerName: requestedLayers) {
        if (isInstanceLayerAvailable(layerName)) {
            std::cout << "Enabling layer: " << layerName << std::endl;
            layers.push_back(layerName);
        }
    }

    uint32_t windowExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &windowExtensionCount, nullptr);
    std::vector<const char *> extensions(windowExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &windowExtensionCount, extensions.data());
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.apiVersion = apiVersion;
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = applicationName;
    applicationInfo.pEngineName = "Dark Star Engine";

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
    instanceCreateInfo.enabledExtensionCount = extensions.size();
    instanceCreateInfo.ppEnabledLayerNames = layers.data();
    instanceCreateInfo.enabledLayerCount = layers.size();
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, allocationCallbacks, &instance))
}

void Vulkan::createDebugUtilsMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    createInfo.pfnUserCallback = debugLog;
    createInfo.pUserData = nullptr;
    createInfo.flags = 0;

    auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(createFunc(instance, &createInfo, allocationCallbacks, &debugUtilsMessenger))
}

VkBool32
Vulkan::debugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    std::cout << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void Vulkan::selectBestPhysicalDevice() {
    uint32_t count;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr))
    std::vector<VkPhysicalDevice> devices(count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, devices.data()))

    auto cmp = [](PhysicalDevice a, PhysicalDevice b) -> int {
        return a.priority > b.priority;
    };

    std::priority_queue<PhysicalDevice, std::vector<PhysicalDevice>, decltype(cmp)> devicePriorities(cmp);

    for (const auto &it: devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(it, &properties);

        uint32_t priority = 0;
        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                priority = 0;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                priority = 3;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                priority = 4;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                priority = 2;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                priority = 1;
                break;
            default:
                priority = 0;
                break;
        }

        devicePriorities.push({
                                      .vkPhysicalDevice = it,
                                      .properties = properties,
                                      .priority = priority,
                              });
    }

    physicalDevice = devicePriorities.top();
    std::cout << "Selected physical device: " << physicalDevice.properties.deviceName << std::endl;
}

std::vector<QueueFamily> Vulkan::fetchAvailableQueueFamilies() {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.vkPhysicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.vkPhysicalDevice, &count, properties.data());

    std::vector<QueueFamily> result;

    for (uint32_t i = 0; i < count; ++i) {
        QueueFamily family = {
                .properties = properties[i],
                .index = i,
                .graphics = (bool) (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT),
                .compute = (bool) (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT),
        };

        VkBool32 presentSupported = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice.vkPhysicalDevice, i, surface, &presentSupported));
        family.present = presentSupported == VK_TRUE;

        result.push_back(family);
    }

    return result;
}

bool Vulkan::isDeviceExtensionAvailable(const std::string &extensionName) const {
    uint32_t count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.vkPhysicalDevice, nullptr, &count, nullptr));
    std::vector<VkExtensionProperties> availableExtensions(count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.vkPhysicalDevice, nullptr, &count,
                                                  availableExtensions.data()));

    for (const auto &extension: availableExtensions) {
        if (extensionName == std::string(extension.extensionName)) {
            return true;
        }
    }

    return false;
}

void Vulkan::createSurface(SDL_Window *window) {
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        std::cerr << "Failed to create Vulkan surface with SDL: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to initialize SDL");
    }
}

void Vulkan::createDevice() {
    queueFamilies = fetchAvailableQueueFamilies();
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float priority = 1.0f;
    for (auto &queueFamily: queueFamilies) {
        VkDeviceQueueCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        createInfo.queueFamilyIndex = queueFamily.index;
        createInfo.pQueuePriorities = &priority;
        createInfo.queueCount = 1;
        createInfo.flags = 0;
        queueCreateInfos.push_back(createInfo);
    }

    std::vector<const char *> extensions;
    if (!isDeviceExtensionAvailable(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        throw std::runtime_error(std::format("Extension unavailable: {}", VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    }

    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (isDeviceExtensionAvailable("VK_KHR_portability_subset")) {
        extensions.push_back("VK_KHR_portability_subset");
    }

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.flags = 0;

    VK_CHECK(vkCreateDevice(physicalDevice.vkPhysicalDevice, &createInfo, allocationCallbacks, &device))
}

VkSurfaceFormatKHR Vulkan::selectSurfaceFormat() {
    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.vkPhysicalDevice, surface, &formatCount, nullptr))
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.vkPhysicalDevice, surface, &formatCount,
                                                  formats.data()))

    for (const auto &format: formats) {
        std::cout << "Found surface format: " << string_VkColorSpaceKHR(format.colorSpace) << " - "
                  << string_VkFormat(format.format) << std::endl;

        // TODO is this the right choice?
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return format;
        }
    }

    return formats.front();
}

VkPresentModeKHR Vulkan::selectPresentMode() {
    uint32_t presentModeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.vkPhysicalDevice, surface, &presentModeCount,
                                                       nullptr))
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.vkPhysicalDevice, surface, &presentModeCount,
                                                       presentModes.data()))

    for (const auto &presentMode: presentModes) {
        std::cout << "Found present mode: " << string_VkPresentModeKHR(presentMode) << std::endl;

        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        } else if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return presentMode;
        }
    }

    return presentModes.front();
}

void Vulkan::createSwapChain() {
    surfaceFormat = selectSurfaceFormat();

    auto found = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const QueueFamily &family) -> bool {
        return family.present;
    });

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.vkPhysicalDevice, surface, &surfaceCapabilities))

    VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = surface;
    createInfo.minImageCount = std::min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount);
    createInfo.oldSwapchain = nullptr;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = false;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = selectPresentMode();
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &found->index;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;

    swapChainExtent = surfaceCapabilities.currentExtent;

    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, allocationCallbacks, &swapChain))

    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr))
    images = std::vector<VkImage>(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()))

    for (auto &image: images) {
        VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        VkImageView imageView;
        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, allocationCallbacks, &imageView))
        imageViews.push_back(imageView);
    }

    for (auto &queueFamily: queueFamilies) {
        vkGetDeviceQueue(device, queueFamily.index, 0, &queueFamily.queue);
    }
}

VkShaderModule Vulkan::createShaderModule(const std::string &shaderFilePath) {
    auto shaderCode = readBinaryFile(shaderFilePath);
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.data());

    VkShaderModule result;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, allocationCallbacks, &result));
    return result;
}

void Vulkan::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, allocationCallbacks, &renderPass));

}

void Vulkan::createPipeline() {
    auto vertShaderModule = createShaderModule("../basic.vert.spv");
    auto fragShaderModule = createShaderModule("../basic.frag.spv");
    shaderModules.push_back(vertShaderModule);
    shaderModules.push_back(fragShaderModule);

    VkPipelineShaderStageCreateInfo vertStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertStageCreateInfo.module = vertShaderModule;
    vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragStageCreateInfo.module = fragShaderModule;
    fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageCreateInfo, fragStageCreateInfo};

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.lineWidth = 1.0;
    rasterizer.depthClampEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocationCallbacks, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(VkPipelineShaderStageCreateInfo);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, allocationCallbacks, &pipeline))
}

void Vulkan::createFrameBuffers() {
    frameBuffers.resize(imageViews.size());

    for (int i = 0; i < imageViews.size(); ++i) {
        VkImageView attachments[] = {imageViews[i]};
        VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        createInfo.renderPass = renderPass;
        createInfo.pAttachments = attachments;
        createInfo.attachmentCount = 1;
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device, &createInfo, allocationCallbacks, &frameBuffers[i]));
    }
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    auto found = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const QueueFamily &family) {
        return family.graphics;
    });

    createInfo.queueFamilyIndex = found->index;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, allocationCallbacks, &commandPool));
}

void Vulkan::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer))
}

void Vulkan::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, allocationCallbacks, &imageAvailableSemaphore))
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, allocationCallbacks, &renderFinishedSemaphore))
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, allocationCallbacks, &inFlightFence))
}

void Vulkan::recordCommands(VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo))

    VkClearValue clearValue = {
            .color = {{0.01f, 0.01f, 0.01f, 1.0f}},
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK(vkEndCommandBuffer(commandBuffer))
}

void Vulkan::renderFrame() {
    VK_CHECK(vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX))
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex))

    vkResetCommandBuffer(commandBuffer, 0);
    recordCommands(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    auto found = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const QueueFamily &family) {
        return family.graphics;
    });

    VK_CHECK(vkQueueSubmit(found->queue, 1, &submitInfo, inFlightFence))

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    auto presentQueueFound = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const QueueFamily &family) {
        return family.present;
    });

    VK_CHECK(vkQueuePresentKHR(presentQueueFound->queue, &presentInfo));
}