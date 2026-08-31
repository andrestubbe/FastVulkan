#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <string>
#include <cstdint>

struct VulkanWindowContext {
    HWND hwnd = nullptr;
    HINSTANCE hInstance = nullptr;
    int width = 0;
    int height = 0;
    bool shouldClose = false;
    bool resized = false;

    // Vulkan Core
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    // Swapchain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapChainExtent = {0, 0};
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Render Pass & Pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Clear Color (Default: Red as requested)
    float clearR = 0.85f;
    float clearG = 0.15f;
    float clearB = 0.15f;
    float clearA = 1.0f;
};

VulkanWindowContext* CreateVulkanWindow(const wchar_t* title, int width, int height);
void DestroyVulkanWindow(VulkanWindowContext* ctx);
bool PollWindowEvents(VulkanWindowContext* ctx);
void RenderAndPresent(VulkanWindowContext* ctx);
void SetClearColor(VulkanWindowContext* ctx, float r, float g, float b, float a);
