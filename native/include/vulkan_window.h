#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <string>

struct Vertex2D {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;   // cached
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
};

struct VulkanWindowContext {
    // Win32
    HWND hwnd = nullptr;
    HINSTANCE hInstance = nullptr;
    bool shouldClose = false;
    bool resized = false;
    bool isFullscreen = false;
    RECT savedWindowRect{};
    DWORD savedStyle = 0;
    DWORD savedExStyle = 0;
    int minWidth = 0, minHeight = 0;
    int maxWidth = 0, maxHeight = 0;
    int width = 0, height = 0;

    // Clear color
    float clearR = 0.0f, clearG = 0.0f, clearB = 0.0f, clearA = 1.0f;
    bool firstFramePresented = false;

    // Vulkan core
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    // Swapchain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{};
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Render
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;

    // 2D Pipeline
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Dynamic vertex buffer
    static constexpr size_t MAX_VERTICES = 65536;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    void* vertexBufferMapped = nullptr;

    // Batching
    std::vector<Vertex2D> queuedVertices;
    VulkanTexture* currentTexture = nullptr;

    // Features
    bool anisotropySupported = false;
    float maxAnisotropy = 1.0f;
};

// Public API
VulkanWindowContext* CreateVulkanWindow(const wchar_t* title, int width, int height,
                                        float clearR, float clearG, float clearB, float clearA);
void DestroyVulkanWindow(VulkanWindowContext* ctx);
bool PollWindowEvents(VulkanWindowContext* ctx);
void RenderAndPresent(VulkanWindowContext* ctx);

void SetClearColor(VulkanWindowContext* ctx, float r, float g, float b, float a);
void SetWindowTitle(VulkanWindowContext* ctx, const wchar_t* title);
int  GetWindowWidth(VulkanWindowContext* ctx);
int  GetWindowHeight(VulkanWindowContext* ctx);
void SetWindowLocation(VulkanWindowContext* ctx, int x, int y);
int  GetWindowX(VulkanWindowContext* ctx);
int  GetWindowY(VulkanWindowContext* ctx);
void SetWindowDimensions(VulkanWindowContext* ctx, int width, int height);
void SetWindowBounds(VulkanWindowContext* ctx, int x, int y, int width, int height);
void CenterWindowOnScreen(VulkanWindowContext* ctx);
void SetWindowVisible(VulkanWindowContext* ctx, bool visible);
void SetWindowResizable(VulkanWindowContext* ctx, bool resizable);
void SetWindowAlwaysOnTop(VulkanWindowContext* ctx, bool alwaysOnTop);
void SetWindowFullscreen(VulkanWindowContext* ctx, bool fullscreen);
bool IsWindowFullscreen(VulkanWindowContext* ctx);
void MinimizeWindow(VulkanWindowContext* ctx);
void MaximizeWindow(VulkanWindowContext* ctx);
void RestoreWindow(VulkanWindowContext* ctx);
void SetWindowMinSize(VulkanWindowContext* ctx, int minW, int minH);
void SetWindowMaxSize(VulkanWindowContext* ctx, int maxW, int maxH);
void SetWindowIcon(VulkanWindowContext* ctx, const uint32_t* pixels, int width, int height);

// Texture / drawing
VulkanTexture* CreateTexture(VulkanWindowContext* ctx, const uint32_t* pixels,
                             uint32_t width, uint32_t height, bool generateMipmaps);
void DestroyTexture(VulkanWindowContext* ctx, VulkanTexture* tex);
void DrawImage(VulkanWindowContext* ctx, VulkanTexture* tex,
               float x, float y, float w, float h,
               float u0, float v0, float u1, float v1,
               float r, float g, float b, float a);
void FlushBatch(VulkanWindowContext* ctx);
void Init2DPipeline(VulkanWindowContext* ctx);
