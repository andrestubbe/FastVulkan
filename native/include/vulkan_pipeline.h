#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
};

struct Vertex2D {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct VulkanWindowContext;
void Init2DPipeline(VulkanWindowContext* ctx);
