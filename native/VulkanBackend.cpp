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
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
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

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    uint32_t currentImageIndex = 0;
    float clearColor[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
    PushConstants pushConstants{};

    // Dynamic Vertex and Index buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    void* vertexBufferMapped = nullptr;
    size_t vertexBufferSize = 0;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    void* indexBufferMapped = nullptr;
    size_t indexBufferSize = 0;

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
        if (tex->descriptorSet != VK_NULL_HANDLE && state->descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(state->device, state->descriptorPool, 1, &tex->descriptorSet);
        }
        if (tex->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(state->device, tex->sampler, nullptr);
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
    imageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
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
    viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(state->device, &viewInfo, nullptr, &tex->view) != VK_SUCCESS) {
        DestroyVKTexture(state, tex);
        return nullptr;
    }

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

    if (vkCreateSampler(state->device, &samplerInfo, nullptr, &tex->sampler) != VK_SUCCESS) {
        DestroyVKTexture(state, tex);
        return nullptr;
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = state->descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &state->descriptorSetLayout;

    if (vkAllocateDescriptorSets(state->device, &setAllocInfo, &tex->descriptorSet) == VK_SUCCESS) {
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo.imageView = tex->view;
        descImageInfo.sampler = tex->sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = tex->descriptorSet;
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

    state->swapChainExtent.width = (uint32_t)w;
    state->swapChainExtent.height = (uint32_t)h;

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
    vkCreateSwapchainKHR(state->device, &createInfo, nullptr, &newSwap);
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

    if (vkCreateInstance(&instInfo, nullptr, &state->instance) != VK_SUCCESS) {
        delete state;
        return 0;
    }

    // 2. Win32 Surface
    VkWin32SurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hwnd = state->hwnd;
    sci.hinstance = GetModuleHandle(nullptr);
    if (vkCreateWin32SurfaceKHR(state->instance, &sci, nullptr, &state->surface) != VK_SUCCESS) {
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

    // Shaders
    auto vertCode = ReadShaderFile("shader_vert.spv");
    auto fragCode = ReadShaderFile("shader_frag.spv");
    VkShaderModule vertModule = CreateShaderModule(state->device, vertCode);
    VkShaderModule fragModule = CreateShaderModule(state->device, fragCode);

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

    vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->graphicsPipeline);

    vkDestroyShaderModule(state->device, vertModule, nullptr);
    vkDestroyShaderModule(state->device, fragModule, nullptr);

    // 8. Descriptor Pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 128;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 128;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkCreateDescriptorPool(state->device, &poolInfo, nullptr, &state->descriptorPool);

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
    cbInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(state->device, &cbInfo, &state->commandBuffer);

    // 10. Sync Primitives
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(state->device, &semInfo, nullptr, &state->imageAvailableSemaphore);
    vkCreateSemaphore(state->device, &semInfo, nullptr, &state->renderFinishedSemaphore);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(state->device, &fenceInfo, nullptr, &state->inFlightFence);

    // 11. 1x1 White Texture
    uint32_t whitePixel = 0xFFFFFFFF;
    state->defaultWhiteTexture = CreateVKTexture(state, &whitePixel, 1, 1);

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

    vkWaitForFences(state->device, 1, &state->inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult acquireRes = vkAcquireNextImageKHR(state->device, state->swapChain, UINT64_MAX, state->imageAvailableSemaphore, VK_NULL_HANDLE, &state->currentImageIndex);
    if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR || acquireRes == VK_SUBOPTIMAL_KHR) {
        fastvk_resize(handle, state->width, state->height);
        return;
    }
    if (acquireRes != VK_SUCCESS) {
        return;
    }

    vkResetFences(state->device, 1, &state->inFlightFence);
    state->frameActive = true;

    vkResetCommandBuffer(state->commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(state->commandBuffer, &beginInfo);

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

    vkCmdBeginRenderPass(state->commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(state->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)state->swapChainExtent.width;
    viewport.height = (float)state->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(state->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = state->swapChainExtent;
    vkCmdSetScissor(state->commandBuffer, 0, 1, &scissor);
}

FASTVK_API void fastvk_clear(int64_t handle, float r, float g, float b, float a) {
    VKState* state = (VKState*)handle;
    if (!state) return;
    state->clearColor[0] = r;
    state->clearColor[1] = g;
    state->clearColor[2] = b;
    state->clearColor[3] = a;
}

FASTVK_API void fastvk_set_viewport(int64_t handle, int32_t x, int32_t y, int32_t w, int32_t h) {
    VKState* state = (VKState*)handle;
    if (!state) return;

    VkViewport viewport{};
    viewport.x = (float)x;
    viewport.y = (float)(y + h);
    viewport.width = (float)w;
    viewport.height = -(float)h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(state->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { x, y };
    scissor.extent = { (uint32_t)w, (uint32_t)h };
    vkCmdSetScissor(state->commandBuffer, 0, 1, &scissor);
}

FASTVK_API void fastvk_set_projection(int64_t handle, const float* matrix16) {
    VKState* state = (VKState*)handle;
    if (!state || !matrix16) return;

    memcpy(state->pushConstants.projection, matrix16, 16 * sizeof(float));
    vkCmdPushConstants(state->commandBuffer, state->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &state->pushConstants);
}

FASTVK_API void fastvk_set_blend_mode(int64_t handle, int32_t mode) {
    // Mode handling
}

FASTVK_API void fastvk_draw_triangles(int64_t handle,
                                     const float* vbData, int32_t vertexCount,
                                     const int32_t* ibData, int32_t indexCount,
                                     int64_t textureHandle) {

    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive || vertexCount == 0 || indexCount == 0 || !vbData || !ibData) return;

    size_t vbBytes = vertexCount * sizeof(Vertex);
    size_t ibBytes = indexCount * sizeof(uint32_t);

    if (state->vertexBufferSize < vbBytes) {
        if (state->vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(state->device, state->vertexBuffer, nullptr);
            vkFreeMemory(state->device, state->vertexBufferMemory, nullptr);
        }
        state->vertexBufferSize = vbBytes + 8192;
        CreateBuffer(state, state->vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->vertexBuffer, state->vertexBufferMemory);
        vkMapMemory(state->device, state->vertexBufferMemory, 0, state->vertexBufferSize, 0, &state->vertexBufferMapped);
    }

    if (state->indexBufferSize < ibBytes) {
        if (state->indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(state->device, state->indexBuffer, nullptr);
            vkFreeMemory(state->device, state->indexBufferMemory, nullptr);
        }
        state->indexBufferSize = ibBytes + 8192;
        CreateBuffer(state, state->indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->indexBuffer, state->indexBufferMemory);
        vkMapMemory(state->device, state->indexBufferMemory, 0, state->indexBufferSize, 0, &state->indexBufferMapped);
    }

    memcpy(state->vertexBufferMapped, vbData, vbBytes);
    memcpy(state->indexBufferMapped, ibData, ibBytes);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(state->commandBuffer, 0, 1, &state->vertexBuffer, offsets);
    vkCmdBindIndexBuffer(state->commandBuffer, state->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    if (textureHandle != 0) {
        VKTexture* tex = (VKTexture*)textureHandle;
        vkCmdBindDescriptorSets(state->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 0, 1, &tex->descriptorSet, 0, nullptr);
    } else if (state->defaultWhiteTexture) {
        vkCmdBindDescriptorSets(state->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 0, 1, &state->defaultWhiteTexture->descriptorSet, 0, nullptr);
    }

    vkCmdDrawIndexed(state->commandBuffer, indexCount, 1, 0, 0, 0);
}

FASTVK_API int64_t fastvk_create_texture(int64_t handle, int32_t w, int32_t h, const void* pixels) {
    VKState* state = (VKState*)handle;
    if (!state || w <= 0 || h <= 0 || !pixels) return 0;
    VKTexture* tex = CreateVKTexture(state, pixels, (uint32_t)w, (uint32_t)h);
    return (int64_t)tex;
}

FASTVK_API void fastvk_update_texture(int64_t handle, int64_t texHandle, const void* pixels) {
    // Sub-region updates can be added if needed
}

FASTVK_API void fastvk_destroy_texture(int64_t handle, int64_t texHandle) {
    VKState* state = (VKState*)handle;
    if (!state || !texHandle) return;
    VKTexture* tex = (VKTexture*)texHandle;
    DestroyVKTexture(state, tex);
}

FASTVK_API void fastvk_end_frame(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;

    vkCmdEndRenderPass(state->commandBuffer);
    vkEndCommandBuffer(state->commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { state->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state->commandBuffer;

    VkSemaphore signalSemaphores[] = { state->renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, state->inFlightFence);
}

FASTVK_API void fastvk_present(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state || !state->frameActive) return;
    state->frameActive = false;

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = { state->renderFinishedSemaphore };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { state->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &state->currentImageIndex;

    VkResult presentRes = vkQueuePresentKHR(state->graphicsQueue, &presentInfo);
    if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR) {
        fastvk_resize(handle, state->width, state->height);
    }
}

FASTVK_API void fastvk_destroy(int64_t handle) {
    VKState* state = (VKState*)handle;
    if (!state) return;

    vkDeviceWaitIdle(state->device);

    if (state->defaultWhiteTexture) {
        DestroyVKTexture(state, state->defaultWhiteTexture);
        state->defaultWhiteTexture = nullptr;
    }

    if (state->vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(state->device, state->vertexBuffer, nullptr);
        vkFreeMemory(state->device, state->vertexBufferMemory, nullptr);
    }
    if (state->indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(state->device, state->indexBuffer, nullptr);
        vkFreeMemory(state->device, state->indexBufferMemory, nullptr);
    }

    vkDestroySemaphore(state->device, state->imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(state->device, state->renderFinishedSemaphore, nullptr);
    vkDestroyFence(state->device, state->inFlightFence, nullptr);

    vkDestroyCommandPool(state->device, state->commandPool, nullptr);
    vkDestroyDescriptorPool(state->device, state->descriptorPool, nullptr);
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
