#pragma once

#include <SDL.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <map>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <glm/vec3.hpp>

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

enum QueueType {
    GRAPHICS,
    PRESENT,
    COMPUTE
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription description{};
        description.stride = sizeof(Vertex);
        description.binding = 0;
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> result{};

        result[0].binding = 0;
        result[0].location = 0;
        result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[0].offset = offsetof(Vertex, position);

        result[1].binding = 0;
        result[1].location = 1;
        result[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        result[1].offset = offsetof(Vertex, color);

        return result;
    }
};

class Vulkan {
public:
    Vulkan() = default;

    void initialize(const char *applicationName, SDL_Window *window);

    ~Vulkan();

    void update();

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
    std::multimap<QueueType, QueueFamily> queueFamilyMap;
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

    VkBuffer vertexBuffer;
    VkMemoryRequirements vertexBufferMemoryRequirements;
    VkDeviceMemory vertexBufferMemory;

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

    void createVertexBuffer(const std::vector<Vertex> &vertices);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createCommandPool();

    void createCommandBuffer();

    void createSyncObjects();

    void recordCommands(VkCommandBuffer &commandBuffer, uint32_t imageIndex);
};