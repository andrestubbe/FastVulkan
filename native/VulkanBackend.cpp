#include "VulkanBackend.h"
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "user32.lib")

struct Vertex {
    float x, y, z;
    float u, v;
    float r, g, b, a;
};

struct PushConstants {
    float projection[16];
};

struct VKTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSetLinear = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSetPoint = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct VKState {
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapChainExtent{};
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT]{};

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;          // 1 = SRC_OVER alpha blending
    VkPipeline graphicsPipelineNone = VK_NULL_HANDLE;      // 0 = BlendMode.NONE (Copy / Overwrite)
    VkPipeline graphicsPipelineAdditive = VK_NULL_HANDLE;  // 2 = BlendMode.ADDITIVE
    VkPipeline graphicsPipelineMultiply = VK_NULL_HANDLE;  // 3 = BlendMode.MULTIPLY
    int32_t currentBlendMode = 1;                          // 0 = NONE, 1 = SRC_OVER, 2 = ADDITIVE, 3 = MULTIPLY
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    VkSampler samplerLinear = VK_NULL_HANDLE;
    VkSampler samplerPoint = VK_NULL_HANDLE;
    int32_t currentFilterMode = 0; // 0 = Linear/Bilinear, 1 = Point/Nearest Neighbor

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT]{};

    uint32_t currentImageIndex = 0;
    float clearColor[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
    PushConstants pushConstants{};

    // Dynamic Vertex and Index buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    void* vertexBufferMapped = nullptr;
    size_t vertexBufferSize = 0;
    size_t vertexBufferOffset = 0;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    void* indexBufferMapped = nullptr;
    size_t indexBufferSize = 0;
    size_t indexBufferOffset = 0;

    // Deferred deletion queues (reclaimed after in-flight GPU fence waits)
    struct RetiredBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        void* mapped;
    };
    std::vector<RetiredBuffer> retiredBuffers;
    std::vector<VKTexture*> retiredTextures;

    VKTexture* defaultWhiteTexture = nullptr;
    bool frameActive = false;
};

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

static const uint32_t g_embeddedVertShader[] =
#include "src/shader_vert.inc"
;

static const uint32_t g_embeddedFragShader[] =
#include "src/shader_frag.inc"
;

static std::vector<char> ReadShaderFile(const std::string& filename) {
    std::vector<std::string> searchPaths = {
        filename,
        "native/src/" + filename,
        "src/main/resources/shaders/" + filename,
        "shaders/" + filename,
        "../native/src/" + filename,
        "../src/main/resources/shaders/" + filename
    };

    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        }
    }
    return {};
}

static VkShaderModule CreateShaderModuleFromWords(VkDevice device, const uint32_t* words, size_t wordCount) {
    if (!words || wordCount == 0) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = wordCount * sizeof(uint32_t);
    createInfo.pCode = words;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

static void CreateBuffer(VKState* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(state->device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(state->device, buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(state->physicalDevice, memReq.memoryTypeBits, properties);
    vkAllocateMemory(state->device, &allocInfo, nullptr, &bufferMemory);

    vkBindBufferMemory(state->device, buffer, bufferMemory, 0);
}

static void DestroyVKTexture(VKState* state, VKTexture* tex) {
    if (!state || !tex) return;
    if (state->device != VK_NULL_HANDLE) {
        if (tex->descriptorSetLinear != VK_NULL_HANDLE && state->descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(state->device, state->descriptorPool, 1, &tex->descriptorSetLinear);
        }
        if (tex->descriptorSetPoint != VK_NULL_HANDLE && state->descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(state->device, state->descriptorPool, 1, &tex->descriptorSetPoint);
        }
        if (tex->view != VK_NULL_HANDLE) {
            vkDestroyImageView(state->device, tex->view, nullptr);
        }
        if (tex->image != VK_NULL_HANDLE) {
            vkDestroyImage(state->device, tex->image, nullptr);
        }
        if (tex->memory != VK_NULL_HANDLE) {
            vkFreeMemory(state->device, tex->memory, nullptr);
        }
    }
    delete tex;
}

static VKTexture* CreateVKTexture(VKState* state, const void* pixels, uint32_t width, uint32_t height) {
    if (!state || !pixels || width == 0 || height == 0) return nullptr;

    auto tex = new VKTexture();
    tex->width = width;
    tex->height = height;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(state->device, &imageInfo, nullptr, &tex->image) != VK_SUCCESS) {
        delete tex;
        return nullptr;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(state->device, tex->image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(state->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(state->device, &allocInfo, nullptr, &tex->memory) != VK_SUCCESS) {
        vkDestroyImage(state->device, tex->image, nullptr);
        delete tex;
        return nullptr;
    }
    vkBindImageMemory(state->device, tex->image, tex->memory, 0);

    VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    CreateBuffer(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingMemory);

    void* data = nullptr;
    if (vkMapMemory(state->device, stagingMemory, 0, imageSize, 0, &data) == VK_SUCCESS) {
        memcpy(data, pixels, (size_t)imageSize);
        vkUnmapMemory(state->device, stagingMemory);
    }

    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandPool = state->commandPool;
    cmdAlloc.commandBufferCount = 1;

    VkCommandBuffer copyCmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(state->device, &cmdAlloc, &copyCmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(copyCmd);

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(state->device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(state->device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(state->device, fence, nullptr);
    vkFreeCommandBuffers(state->device, state->commandPool, 1, &copyCmd);
    vkDestroyBuffer(state->device, stagingBuffer, nullptr);
    vkFreeMemory(state->device, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(state->device, &viewInfo, nullptr, &tex->view) != VK_SUCCESS) {
        DestroyVKTexture(state, tex);
        return nullptr;
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = state->descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &state->descriptorSetLayout;

    // Linear descriptor set
    VkResult resLinear = vkAllocateDescriptorSets(state->device, &setAllocInfo, &tex->descriptorSetLinear);
    if (resLinear != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkAllocateDescriptorSets (linear) failed with %d\n", (int)resLinear);
        fflush(stderr);
        DestroyVKTexture(state, tex);
        return nullptr;
    }
    {
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo.imageView = tex->view;
        descImageInfo.sampler = state->samplerLinear;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = tex->descriptorSetLinear;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &descImageInfo;

        vkUpdateDescriptorSets(state->device, 1, &write, 0, nullptr);
    }

    // Point descriptor set
    VkResult resPoint = vkAllocateDescriptorSets(state->device, &setAllocInfo, &tex->descriptorSetPoint);
    if (resPoint != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkAllocateDescriptorSets (point) failed with %d\n", (int)resPoint);
        fflush(stderr);
        DestroyVKTexture(state, tex);
        return nullptr;
    }
    {
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo.imageView = tex->view;
        descImageInfo.sampler = state->samplerPoint;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = tex->descriptorSetPoint;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &descImageInfo;

        vkUpdateDescriptorSets(state->device, 1, &write, 0, nullptr);
    }

    return tex;
}

static void SelectSurfaceFormat(VKState* state) {
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, nullptr);
    if (formatCount == 0) {
        state->swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        return;
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats.data());

    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            state->swapChainImageFormat = f.format;
            return;
        }
    }
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            state->swapChainImageFormat = f.format;
            return;
        }
    }
    state->swapChainImageFormat = formats[0].format;
}

static void CleanupSwapChain(VKState* state) {
    if (!state || state->device == VK_NULL_HANDLE) return;
    for (auto fb : state->swapChainFramebuffers) {
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(state->device, fb, nullptr);
    }
    state->swapChainFramebuffers.clear();

    for (auto iv : state->swapChainImageViews) {
        if (iv != VK_NULL_HANDLE) vkDestroyImageView(state->device, iv, nullptr);
    }
    state->swapChainImageViews.clear();
}

static void CreateSwapChain(VKState* state, int w, int h) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &capabilities);

    if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.width > 0) {
        state->swapChainExtent = capabilities.currentExtent;
    } else {
        uint32_t width = (uint32_t)w;
        uint32_t height = (uint32_t)h;
        state->swapChainExtent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        state->swapChainExtent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (presentModeCount > 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentModeCount, presentModes.data());
    }

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    VkSwapchainKHR oldSwap = state->swapChain;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = state->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = state->swapChainImageFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = state->swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwap;

    VkSwapchainKHR newSwap = VK_NULL_HANDLE;
    VkResult scRes = vkCreateSwapchainKHR(state->device, &createInfo, nullptr, &newSwap);
    if (scRes != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkCreateSwapchainKHR failed with %d\n", (int)scRes);
        fflush(stderr);
    } else {
        printf("[FastVulkan] vkCreateSwapchainKHR succeeded (extent=%ux%u, images=%u)\n", 
            state->swapChainExtent.width, state->swapChainExtent.height, imageCount);
        fflush(stdout);
    }
    state->swapChain = newSwap;

    if (oldSwap != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(state->device, oldSwap, nullptr);
    }

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(state->device, state->swapChain, &count, nullptr);
    state->swapChainImages.resize(count);
    vkGetSwapchainImagesKHR(state->device, state->swapChain, &count, state->swapChainImages.data());

    state->swapChainImageViews.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = state->swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = state->swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(state->device, &viewInfo, nullptr, &state->swapChainImageViews[i]);
    }

    state->swapChainFramebuffers.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VkImageView attachments[] = { state->swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = state->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = state->swapChainExtent.width;
        framebufferInfo.height = state->swapChainExtent.height;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(state->device, &framebufferInfo, nullptr, &state->swapChainFramebuffers[i]);
    }
}

extern "C" {

FASTVK_API int64_t fastvk_create(int64_t hwnd, int32_t w, int32_t h) {
    VKState* state = new VKState();
    state->hwnd = (HWND)hwnd;
    state->width = w;
    state->height = h;

    // 1. Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "FastVulkan Backend";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "FastJava";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = extensions;

    VkResult instRes = vkCreateInstance(&instInfo, nullptr, &state->instance);
    if (instRes != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkCreateInstance failed with %d\n", (int)instRes);
        fflush(stderr);
        delete state;
        return 0;
    }

    // 2. Win32 Surface
    VkWin32SurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hwnd = state->hwnd;
    sci.hinstance = GetModuleHandle(nullptr);
    VkResult surfRes = vkCreateWin32SurfaceKHR(state->instance, &sci, nullptr, &state->surface);
    if (surfRes != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkCreateWin32SurfaceKHR failed with %d on HWND %p\n", (int)surfRes, state->hwnd);
        fflush(stderr);
        vkDestroyInstance(state->instance, nullptr);
        delete state;
        return 0;
    }

    // 3. Physical Device Selection
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
        vkDestroyInstance(state->instance, nullptr);
        delete state;
        return 0;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, devices.data());

    for (const auto& dev : devices) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, state->surface, &presentSupport);
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
                state->physicalDevice = dev;
                state->queueFamilyIndex = i;
                break;
            }
        }
        if (state->physicalDevice != VK_NULL_HANDLE) break;
    }

    if (state->physicalDevice == VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
        vkDestroyInstance(state->instance, nullptr);
        delete state;
        return 0;
    }

    // 4. Logical Device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = state->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* devExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo devInfo{};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueCreateInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = devExtensions;

    if (vkCreateDevice(state->physicalDevice, &devInfo, nullptr, &state->device) != VK_SUCCESS) {
        vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
        vkDestroyInstance(state->instance, nullptr);
        delete state;
        return 0;
    }
    vkGetDeviceQueue(state->device, state->queueFamilyIndex, 0, &state->graphicsQueue);

    SelectSurfaceFormat(state);

    // 5. Render Pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = state->swapChainImageFormat;
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

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    vkCreateRenderPass(state->device, &renderPassInfo, nullptr, &state->renderPass);

    // 6. SwapChain
    CreateSwapChain(state, w, h);

    // 7. Descriptor Layout & Pipeline
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    vkCreateDescriptorSetLayout(state->device, &layoutInfo, nullptr, &state->descriptorSetLayout);

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &state->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    vkCreatePipelineLayout(state->device, &pipelineLayoutInfo, nullptr, &state->pipelineLayout);

    // Shaders: Use directly embedded SPIR-V bytecode with fallback to disk
    VkShaderModule vertModule = CreateShaderModuleFromWords(state->device, g_embeddedVertShader, sizeof(g_embeddedVertShader) / sizeof(uint32_t));
    if (vertModule == VK_NULL_HANDLE) {
        auto vertCode = ReadShaderFile("shader_vert.spv");
        vertModule = CreateShaderModule(state->device, vertCode);
    }

    VkShaderModule fragModule = CreateShaderModuleFromWords(state->device, g_embeddedFragShader, sizeof(g_embeddedFragShader) / sizeof(uint32_t));
    if (fragModule == VK_NULL_HANDLE) {
        auto fragCode = ReadShaderFile("shader_frag.spv");
        fragModule = CreateShaderModule(state->device, fragCode);
    }

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[3]{};
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset = 0;

    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[1].offset = offsetof(Vertex, u);

    attrDescs[2].binding = 0;
    attrDescs[2].location = 2;
    attrDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrDescs[2].offset = offsetof(Vertex, r);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = state->pipelineLayout;
    pipelineInfo.renderPass = state->renderPass;
    pipelineInfo.subpass = 0;

    // 1) Default Pipeline: SRC_OVER alpha blending
    vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->graphicsPipeline);

    // 2) Opaque Pipeline: BlendMode.NONE (Overwrite)
    colorBlendAttachment.blendEnable = VK_FALSE;
    vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->graphicsPipelineNone);

    // 3) Additive Pipeline: BlendMode.ADDITIVE (SrcAlpha + One)
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->graphicsPipelineAdditive);

    // 4) Multiply Pipeline: BlendMode.MULTIPLY (DstColor + Zero)
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->graphicsPipelineMultiply);

    vkDestroyShaderModule(state->device, vertModule, nullptr);
    vkDestroyShaderModule(state->device, fragModule, nullptr);

    // 8. Descriptor Pool (Scaled to 4096 to prevent pool exhaustion with large text/texture caches)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4096;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 4096;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &state->descriptorPool);

    // 8b. Texture Samplers (Linear and Point)
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vkCreateSampler(state->device, &samplerInfo, nullptr, &state->samplerLinear);

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    vkCreateSampler(state->device, &samplerInfo, nullptr, &state->samplerPoint);

    // 9. Command Pool & Buffer
    VkCommandPoolCreateInfo cpInfo{};
    cpInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpInfo.queueFamilyIndex = state->queueFamilyIndex;
    cpInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(state->device, &cpInfo, nullptr, &state->commandPool);

    VkCommandBufferAllocateInfo cbInfo{};
    cbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbInfo.commandPool = state->commandPool;
    cbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbInfo.commandBufferCount = VKState::MAX_FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(state->device, &cbInfo, state->commandBuffers);

    // 10. Sync Primitives for MAX_FRAMES_IN_FLIGHT
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < VKState::MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(state->device, &semInfo, nullptr, &state->imageAvailableSemaphores[i]);
        vkCreateSemaphore(state->device, &semInfo, nullptr, &state->renderFinishedSemaphores[i]);
        vkCreateFence(state->device, &fenceInfo, nullptr, &state->inFlightFences[i]);
    }

    // 11. 1x1 White Texture
    uint32_t whitePixel = 0xFFFFFFFF;
    state->defaultWhiteTexture = CreateVKTexture(state, &whitePixel, 1, 1);

    // 12. Persistent Vertex & Index Ring Buffers (16 MB VB, 8 MB IB)
    state->vertexBufferSize = 16 * 1024 * 1024;
    CreateBuffer(state, state->vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 state->vertexBuffer, state->vertexBufferMemory);
    vkMapMemory(state->device, state->vertexBufferMemory, 0, state->vertexBufferSize, 0, &state->vertexBufferMapped);

    state->indexBufferSize = 8 * 1024 * 1024;
    CreateBuffer(state, state->indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 state->indexBuffer, state->indexBufferMemory);
    vkMapMemory(state->device, state->indexBufferMemory, 0, state->indexBufferSize, 0, &state->indexBufferMapped);

    return (int64_t)state;
}

FASTVK_API void fastvk_resize(int64_t handle, int32_t w, int32_t h) {
    VKState* state = (VKState*)handle;
    if (!state || w <= 0 || h <= 0) return;

    if (state->graphicsQueue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(state->graphicsQueue);
    }
    state->width = w;
    state->height = h;

    CleanupSwapChain(state);
    CreateSwapChain(state, w, h);
}

FASTVK_API void fastvk_begin_frame(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state) return;
    state->frameActive = false;

    uint32_t frameIdx = state->currentFrame;
    vkWaitForFences(state->device, 1, &state->inFlightFences[frameIdx], VK_TRUE, UINT64_MAX);

    // Reclaim deferred buffers and textures after GPU execution of previous frame
    for (const auto& rb : state->retiredBuffers) {
        if (rb.buffer != VK_NULL_HANDLE) {
            if (rb.mapped != nullptr) {
                vkUnmapMemory(state->device, rb.memory);
            }
            vkDestroyBuffer(state->device, rb.buffer, nullptr);
            vkFreeMemory(state->device, rb.memory, nullptr);
        }
    }
    state->retiredBuffers.clear();

    for (auto* tex : state->retiredTextures) {
        if (tex) {
            DestroyVKTexture(state, tex);
        }
    }
    state->retiredTextures.clear();

    VkResult acquireRes = vkAcquireNextImageKHR(state->device, state->swapChain, UINT64_MAX,
                                                state->imageAvailableSemaphores[frameIdx], VK_NULL_HANDLE, &state->currentImageIndex);
    if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR || acquireRes == VK_SUBOPTIMAL_KHR) {
        printf("[FastVulkan] vkAcquireNextImageKHR returned %d (re-sizing)\n", (int)acquireRes);
        fflush(stdout);
        fastvk_resize(handle, state->width, state->height);
        return;
    }
    if (acquireRes != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkAcquireNextImageKHR failed with %d\n", (int)acquireRes);
        fflush(stderr);
        return;
    }

    vkResetFences(state->device, 1, &state->inFlightFences[frameIdx]);
    state->frameActive = true;
    state->vertexBufferOffset = 0;
    state->indexBufferOffset = 0;

    VkCommandBuffer cmd = state->commandBuffers[frameIdx];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = state->renderPass;
    rpInfo.framebuffer = state->swapChainFramebuffers[state->currentImageIndex];
    rpInfo.renderArea.offset = { 0, 0 };
    rpInfo.renderArea.extent = state->swapChainExtent;

    VkClearValue clearColor{};
    clearColor.color = { { state->clearColor[0], state->clearColor[1], state->clearColor[2], state->clearColor[3] } };
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)state->swapChainExtent.width;
    viewport.height = (float)state->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = state->swapChainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

FASTVK_API void fastvk_clear(int64_t handle, float r, float g, float b, float a) {
    VKState* state = (VKState*)handle;
    if (!state) return;
    state->clearColor[0] = r;
    state->clearColor[1] = g;
    state->clearColor[2] = b;
    state->clearColor[3] = a;

    if (state->frameActive) {
        VkClearAttachment clearAttachment{};
        clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachment.colorAttachment = 0;
        clearAttachment.clearValue.color = { {r, g, b, a} };

        VkClearRect clearRect{};
        clearRect.rect.offset = { 0, 0 };
        clearRect.rect.extent = state->swapChainExtent;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        VkCommandBuffer cmd = state->commandBuffers[state->currentFrame];
        vkCmdClearAttachments(cmd, 1, &clearAttachment, 1, &clearRect);
    }
}

FASTVK_API void fastvk_set_viewport(int64_t handle, int32_t x, int32_t y, int32_t w, int32_t h) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;

    VkViewport viewport{};
    viewport.x = (float)x;
    viewport.y = (float)(y + h);
    viewport.width = (float)w;
    viewport.height = -(float)h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkCommandBuffer cmd = state->commandBuffers[state->currentFrame];
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { x, y };
    scissor.extent = { (uint32_t)w, (uint32_t)h };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

FASTVK_API void fastvk_set_scissor(int64_t handle, int32_t x, int32_t y, int32_t w, int32_t h) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;

    VkRect2D scissor{};
    scissor.offset = { x, y };
    scissor.extent = { (uint32_t)w, (uint32_t)h };
    VkCommandBuffer cmd = state->commandBuffers[state->currentFrame];
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

FASTVK_API void fastvk_set_projection(int64_t handle, const float* matrix16) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive || !matrix16) return;

    memcpy(state->pushConstants.projection, matrix16, 16 * sizeof(float));
    VkCommandBuffer cmd = state->commandBuffers[state->currentFrame];
    vkCmdPushConstants(cmd, state->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &state->pushConstants);
}

FASTVK_API void fastvk_set_blend_mode(int64_t handle, int32_t mode) {
    VKState* state = (VKState*)handle;
    if (!state) return;
    state->currentBlendMode = mode;
}

FASTVK_API void fastvk_set_texture_filter(int64_t handle, int32_t filterMode) {
    VKState* state = (VKState*)handle;
    if (!state) return;
    state->currentFilterMode = filterMode;
}

FASTVK_API void fastvk_draw_triangles(int64_t handle,
                                     const float* vbData, int32_t vertexCount,
                                     const int32_t* ibData, int32_t indexCount,
                                     int64_t textureHandle) {

    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive || vertexCount == 0 || indexCount == 0 || !vbData || !ibData) return;

    size_t vbBytes = vertexCount * sizeof(Vertex);
    size_t ibBytes = indexCount * sizeof(uint32_t);

    // 16-byte alignment for vertex buffer offset, 4-byte for index buffer offset
    size_t alignedVbOffset = (state->vertexBufferOffset + 15) & ~15;
    size_t alignedIbOffset = (state->indexBufferOffset + 3) & ~3;

    size_t requiredVbSize = alignedVbOffset + vbBytes;
    size_t requiredIbSize = alignedIbOffset + ibBytes;

    if (state->vertexBufferSize < requiredVbSize) {
        if (state->vertexBuffer != VK_NULL_HANDLE) {
            state->retiredBuffers.push_back({ state->vertexBuffer, state->vertexBufferMemory, state->vertexBufferMapped });
        }
        state->vertexBufferSize = (std::max)(state->vertexBufferSize * 2, requiredVbSize + 65536);
        CreateBuffer(state, state->vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->vertexBuffer, state->vertexBufferMemory);
        vkMapMemory(state->device, state->vertexBufferMemory, 0, state->vertexBufferSize, 0, &state->vertexBufferMapped);
    }

    if (state->indexBufferSize < requiredIbSize) {
        if (state->indexBuffer != VK_NULL_HANDLE) {
            state->retiredBuffers.push_back({ state->indexBuffer, state->indexBufferMemory, state->indexBufferMapped });
        }
        state->indexBufferSize = (std::max)(state->indexBufferSize * 2, requiredIbSize + 65536);
        CreateBuffer(state, state->indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->indexBuffer, state->indexBufferMemory);
        vkMapMemory(state->device, state->indexBufferMemory, 0, state->indexBufferSize, 0, &state->indexBufferMapped);
    }

    memcpy((char*)state->vertexBufferMapped + alignedVbOffset, vbData, vbBytes);
    memcpy((char*)state->indexBufferMapped + alignedIbOffset, ibData, ibBytes);

    VkCommandBuffer cmd = state->commandBuffers[state->currentFrame];
    VkDeviceSize offsets[] = { (VkDeviceSize)alignedVbOffset };
    vkCmdBindVertexBuffers(cmd, 0, 1, &state->vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, state->indexBuffer, (VkDeviceSize)alignedIbOffset, VK_INDEX_TYPE_UINT32);

    VkPipeline pipeline = state->graphicsPipeline;
    switch (state->currentBlendMode) {
        case 0: pipeline = state->graphicsPipelineNone; break;
        case 1: pipeline = state->graphicsPipeline; break;
        case 2: pipeline = state->graphicsPipelineAdditive; break;
        case 3: pipeline = state->graphicsPipelineMultiply; break;
        default: pipeline = state->graphicsPipeline; break;
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    if (textureHandle != 0) {
        VKTexture* tex = (VKTexture*)textureHandle;
        VkDescriptorSet ds = (state->currentFilterMode == 1) ? tex->descriptorSetPoint : tex->descriptorSetLinear;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 0, 1, &ds, 0, nullptr);
    } else if (state->defaultWhiteTexture) {
        VkDescriptorSet ds = (state->currentFilterMode == 1) ? state->defaultWhiteTexture->descriptorSetPoint : state->defaultWhiteTexture->descriptorSetLinear;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 0, 1, &ds, 0, nullptr);
    }

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    state->vertexBufferOffset = alignedVbOffset + vbBytes;
    state->indexBufferOffset = alignedIbOffset + ibBytes;
}

FASTVK_API int64_t fastvk_create_texture(int64_t handle, int32_t w, int32_t h, const void* pixels) {
    VKState* state = (VKState*)handle;
    if (!state || w <= 0 || h <= 0 || !pixels) return 0;
    VKTexture* tex = CreateVKTexture(state, pixels, (uint32_t)w, (uint32_t)h);
    return (int64_t)tex;
}

FASTVK_API void fastvk_update_texture(int64_t handle, int64_t texHandle, const void* pixels) {
    VKState* state = (VKState*)handle;
    if (!state || !texHandle || !pixels) return;
    VKTexture* tex = (VKTexture*)texHandle;
    if (tex->width == 0 || tex->height == 0 || tex->image == VK_NULL_HANDLE) return;

    VkDeviceSize imageSize = (VkDeviceSize)tex->width * tex->height * 4;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    CreateBuffer(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingMemory);

    void* data = nullptr;
    if (vkMapMemory(state->device, stagingMemory, 0, imageSize, 0, &data) == VK_SUCCESS) {
        memcpy(data, pixels, (size_t)imageSize);
        vkUnmapMemory(state->device, stagingMemory);
    }

    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandPool = state->commandPool;
    cmdAlloc.commandBufferCount = 1;

    VkCommandBuffer copyCmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(state->device, &cmdAlloc, &copyCmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { tex->width, tex->height, 1 };

    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(copyCmd);

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(state->device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(state->device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(state->device, fence, nullptr);
    vkFreeCommandBuffers(state->device, state->commandPool, 1, &copyCmd);
    vkDestroyBuffer(state->device, stagingBuffer, nullptr);
    vkFreeMemory(state->device, stagingMemory, nullptr);
}

FASTVK_API void fastvk_destroy_texture(int64_t handle, int64_t texHandle) {
    VKState* state = (VKState*)handle;
    if (!state || !texHandle) return;
    VKTexture* tex = (VKTexture*)texHandle;
    if (state->frameActive) {
        state->retiredTextures.push_back(tex);
    } else {
        DestroyVKTexture(state, tex);
    }
}

FASTVK_API void fastvk_end_frame(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;

    uint32_t frameIdx = state->currentFrame;
    VkCommandBuffer cmd = state->commandBuffers[frameIdx];

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { state->imageAvailableSemaphores[frameIdx] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { state->renderFinishedSemaphores[frameIdx] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, state->inFlightFences[frameIdx]);
}

FASTVK_API void fastvk_present(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;
    state->frameActive = false;

    uint32_t frameIdx = state->currentFrame;
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = { state->renderFinishedSemaphores[frameIdx] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { state->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &state->currentImageIndex;

    VkResult presentRes = vkQueuePresentKHR(state->graphicsQueue, &presentInfo);
    if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR) {
        printf("[FastVulkan] vkQueuePresentKHR returned %d (re-sizing)\n", (int)presentRes);
        fflush(stdout);
        fastvk_resize(handle, state->width, state->height);
    } else if (presentRes != VK_SUCCESS) {
        fprintf(stderr, "[FastVulkan] ERROR: vkQueuePresentKHR failed with %d\n", (int)presentRes);
        fflush(stderr);
    }

    state->currentFrame = (state->currentFrame + 1) % VKState::MAX_FRAMES_IN_FLIGHT;
}

FASTVK_API void fastvk_destroy(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state) return;

    vkDeviceWaitIdle(state->device);

    if (state->defaultWhiteTexture) {
        DestroyVKTexture(state, state->defaultWhiteTexture);
        state->defaultWhiteTexture = nullptr;
    }

    // Reclaim any remaining retired buffers and textures
    for (const auto& rb : state->retiredBuffers) {
        if (rb.buffer != VK_NULL_HANDLE) {
            if (rb.mapped != nullptr) {
                vkUnmapMemory(state->device, rb.memory);
            }
            vkDestroyBuffer(state->device, rb.buffer, nullptr);
            vkFreeMemory(state->device, rb.memory, nullptr);
        }
    }
    state->retiredBuffers.clear();

    for (auto* tex : state->retiredTextures) {
        if (tex) {
            DestroyVKTexture(state, tex);
        }
    }
    state->retiredTextures.clear();

    if (state->vertexBuffer != VK_NULL_HANDLE) {
        if (state->vertexBufferMapped != nullptr) {
            vkUnmapMemory(state->device, state->vertexBufferMemory);
            state->vertexBufferMapped = nullptr;
        }
        vkDestroyBuffer(state->device, state->vertexBuffer, nullptr);
        vkFreeMemory(state->device, state->vertexBufferMemory, nullptr);
    }
    if (state->indexBuffer != VK_NULL_HANDLE) {
        if (state->indexBufferMapped != nullptr) {
            vkUnmapMemory(state->device, state->indexBufferMemory);
            state->indexBufferMapped = nullptr;
        }
        vkDestroyBuffer(state->device, state->indexBuffer, nullptr);
        vkFreeMemory(state->device, state->indexBufferMemory, nullptr);
    }

    for (uint32_t i = 0; i < VKState::MAX_FRAMES_IN_FLIGHT; i++) {
        if (state->imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(state->device, state->imageAvailableSemaphores[i], nullptr);
        }
        if (state->renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(state->device, state->renderFinishedSemaphores[i], nullptr);
        }
        if (state->inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(state->device, state->inFlightFences[i], nullptr);
        }
    }

    vkDestroyCommandPool(state->device, state->commandPool, nullptr);
    if (state->samplerLinear != VK_NULL_HANDLE) {
        vkDestroySampler(state->device, state->samplerLinear, nullptr);
    }
    if (state->samplerPoint != VK_NULL_HANDLE) {
        vkDestroySampler(state->device, state->samplerPoint, nullptr);
    }
    vkDestroyDescriptorPool(state->device, state->descriptorPool, nullptr);
    if (state->graphicsPipelineNone != VK_NULL_HANDLE) {
        vkDestroyPipeline(state->device, state->graphicsPipelineNone, nullptr);
    }
    if (state->graphicsPipelineAdditive != VK_NULL_HANDLE) {
        vkDestroyPipeline(state->device, state->graphicsPipelineAdditive, nullptr);
    }
    if (state->graphicsPipelineMultiply != VK_NULL_HANDLE) {
        vkDestroyPipeline(state->device, state->graphicsPipelineMultiply, nullptr);
    }
    vkDestroyPipeline(state->device, state->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(state->device, state->pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(state->device, state->descriptorSetLayout, nullptr);

    CleanupSwapChain(state);
    if (state->swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(state->device, state->swapChain, nullptr);
        state->swapChain = VK_NULL_HANDLE;
    }
    vkDestroyRenderPass(state->device, state->renderPass, nullptr);
    vkDestroyDevice(state->device, nullptr);
    vkDestroySurfaceKHR(state->instance, state->surface, nullptr);
    vkDestroyInstance(state->instance, nullptr);

    delete state;
}

}
