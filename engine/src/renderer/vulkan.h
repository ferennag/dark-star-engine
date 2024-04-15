#pragma once

#include <SDL.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(expr) {                            \
    VkResult _result = expr;                         \
    if (_result != VK_SUCCESS) {                     \
        std::stringstream message;                  \
        message << "Vulkan error: ";                \
        message << string_VkResult(_result);         \
        std::cerr << message.str() << std::endl;    \
        throw std::runtime_error(message.str());    \
    }\
}\

typedef struct PhysicalDevice {
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties properties;
    uint32_t priority;
} PhysicalDevice;

typedef struct QueueFamily {
    VkQueueFamilyProperties properties;
    uint32_t index;
    bool graphics;
    bool present;
    bool compute;
    VkQueue queue;
} QueueFamily;

class Vulkan {
public:
    Vulkan() = default;

    void initialize(const char *applicationName, SDL_Window *window);

    ~Vulkan();

    void renderFrame();

private:
    VkAllocationCallbacks *allocationCallbacks = nullptr;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    VkInstance instance;
    PhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkDevice device;

    VkSurfaceFormatKHR surfaceFormat;
    VkSwapchainKHR swapChain;
    VkExtent2D swapChainExtent;

    std::vector<QueueFamily> queueFamilies;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> frameBuffers;

    std::vector<VkShaderModule> shaderModules;
    VkRenderPass renderPass;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    static VkBool32 debugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                             void *pUserData);

    bool isInstanceLayerAvailable(const char *layerName);

    bool isDeviceExtensionAvailable(const std::string &extensionName) const;

    uint32_t getApiVersion() const;

    void createInstance(const char *applicationName, SDL_Window *window);

    void createDebugUtilsMessenger();

    void selectBestPhysicalDevice();

    void createSurface(SDL_Window *window);

    std::vector<QueueFamily> fetchAvailableQueueFamilies();

    void createDevice();

    VkSurfaceFormatKHR selectSurfaceFormat();

    VkPresentModeKHR selectPresentMode();

    void createSwapChain();

    void cleanupSwapChain();

    void recreateSwapChain();

    VkShaderModule createShaderModule(const std::string &shaderFilePath);

    void createRenderPass();

    void createPipeline();

    void createFrameBuffers();

    void createCommandPool();

    void createCommandBuffer();

    void createSyncObjects();

    void recordCommands(VkCommandBuffer &commandBuffer, uint32_t imageIndex);
};