#include "vulkan_window.h"
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm>

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
        "src/main/resources/shaders/" + filename,
        "native/src/" + filename,
        "../../src/main/resources/shaders/" + filename,
        "../../native/src/" + filename,
        "../src/main/resources/shaders/" + filename,
        "../native/src/" + filename
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

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

void Init2DPipeline(VulkanWindowContext* ctx) {
    // 1. Descriptor Set Layout for Sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, nullptr, &ctx->descriptorSetLayout) != VK_SUCCESS) {
        return;
    }

    // 2. Push Constants for 4x4 Ortho Matrix (16 floats = 64 bytes)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 16;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &ctx->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, nullptr, &ctx->pipelineLayout) != VK_SUCCESS) {
        return;
    }

    // 3. Shaders
    auto vertCode = ReadShaderFile("shader_vert.spv");
    auto fragCode = ReadShaderFile("shader_frag.spv");
    VkShaderModule vertShaderModule = CreateShaderModule(ctx->device, vertCode);
    VkShaderModule fragShaderModule = CreateShaderModule(ctx->device, fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // 4. Vertex Input (x, y, u, v, r, g, b, a)
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex2D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[3]{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex2D, x);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex2D, u);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex2D, r);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

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
    multisampling.sampleShadingEnable = VK_FALSE;
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

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

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
    pipelineInfo.layout = ctx->pipelineLayout;
    pipelineInfo.renderPass = ctx->renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx->graphicsPipeline);

    if (vertShaderModule) vkDestroyShaderModule(ctx->device, vertShaderModule, nullptr);
    if (fragShaderModule) vkDestroyShaderModule(ctx->device, fragShaderModule, nullptr);

    // 5. Descriptor Pool for Textures
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 128;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 128;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &ctx->descriptorPool);

    // 6. Dynamic Vertex Buffer
    VkDeviceSize bufferSize = sizeof(Vertex2D) * ctx->MAX_VERTICES;
    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = bufferSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(ctx->device, &bufInfo, nullptr, &ctx->vertexBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx->device, ctx->vertexBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(ctx->physicalDevice, memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx->device, &allocInfo, nullptr, &ctx->vertexBufferMemory);
    vkBindBufferMemory(ctx->device, ctx->vertexBuffer, ctx->vertexBufferMemory, 0);
    vkMapMemory(ctx->device, ctx->vertexBufferMemory, 0, bufferSize, 0, &ctx->vertexBufferMapped);
}

VulkanTexture* CreateTexture(VulkanWindowContext* ctx, const uint32_t* pixels, uint32_t width, uint32_t height, bool generateMipmaps) {
    if (!ctx || !pixels || width == 0 || height == 0) return nullptr;

    auto tex = new VulkanTexture();
    tex->width = width;
    tex->height = height;

    uint32_t mipLevels = 1;
    if (generateMipmaps) {
        mipLevels = (uint32_t)(std::floor(std::log2((std::max)(width, height)))) + 1;
    }
    tex->mipLevels = mipLevels;

    // Create Image
    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(ctx->device, &imageInfo, nullptr, &tex->image) != VK_SUCCESS) {
        delete tex;
        return nullptr;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(ctx->device, tex->image, &memReqs);

    uint32_t imageMemType = FindMemoryType(ctx->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imageMemType == UINT32_MAX) {
        vkDestroyImage(ctx->device, tex->image, nullptr);
        delete tex;
        return nullptr;
    }

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = imageMemType;

    if (vkAllocateMemory(ctx->device, &allocInfo, nullptr, &tex->memory) != VK_SUCCESS) {
        vkDestroyImage(ctx->device, tex->image, nullptr);
        delete tex;
        return nullptr;
    }
    vkBindImageMemory(ctx->device, tex->image, tex->memory, 0);

    // Staging Buffer
    VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo stageBufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stageBufInfo.size = imageSize;
    stageBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stageBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx->device, &stageBufInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        DestroyTexture(ctx, tex);
        return nullptr;
    }
    VkMemoryRequirements stageReqs;
    vkGetBufferMemoryRequirements(ctx->device, stagingBuffer, &stageReqs);

    uint32_t stageMemType = FindMemoryType(ctx->physicalDevice, stageReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo stageAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    stageAlloc.allocationSize = stageReqs.size;
    stageAlloc.memoryTypeIndex = stageMemType;

    if (vkAllocateMemory(ctx->device, &stageAlloc, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
        DestroyTexture(ctx, tex);
        return nullptr;
    }
    vkBindBufferMemory(ctx->device, stagingBuffer, stagingMemory, 0);

    void* data = nullptr;
    if (vkMapMemory(ctx->device, stagingMemory, 0, imageSize, 0, &data) == VK_SUCCESS) {
        // Direct zero-overhead copy: Java 0xAARRGGBB in little-endian byte order is memory [B, G, R, A],
        // which matches VK_FORMAT_B8G8R8A8_UNORM pixel-for-pixel with 0 byte manipulation.
        memcpy(data, pixels, (size_t)imageSize);
        vkUnmapMemory(ctx->device, stagingMemory);
    }

    // Copy to Image using command buffer
    VkCommandBufferAllocateInfo cmdAlloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandPool = ctx->commandPool;
    cmdAlloc.commandBufferCount = 1;

    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(ctx->device, &cmdAlloc, &copyCmd);

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCmd, &beginInfo);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
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

    if (mipLevels == 1) {
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    } else {
        int32_t mipW = width;
        int32_t mipH = height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipW, mipH, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipW > 1 ? mipW / 2 : 1, mipH > 1 ? mipH / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(copyCmd, tex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipW > 1) mipW /= 2;
            if (mipH > 1) mipH /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    vkEndCommandBuffer(copyCmd);

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphicsQueue);

    vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &copyCmd);
    vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->device, stagingMemory, nullptr);

    // Create View
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = tex->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(ctx->device, &viewInfo, nullptr, &tex->view) != VK_SUCCESS) {
        DestroyTexture(ctx, tex);
        return nullptr;
    }

    // Create Sampler with queried Anisotropy
    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = ctx->anisotropySupported ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = ctx->anisotropySupported ? ctx->maxAnisotropy : 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float)mipLevels;

    if (vkCreateSampler(ctx->device, &samplerInfo, nullptr, &tex->sampler) != VK_SUCCESS) {
        DestroyTexture(ctx, tex);
        return nullptr;
    }

    // Preallocate & cache descriptor set once for this texture
    if (ctx->descriptorPool != VK_NULL_HANDLE && ctx->descriptorSetLayout != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo setAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        setAllocInfo.descriptorPool = ctx->descriptorPool;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &ctx->descriptorSetLayout;

        if (vkAllocateDescriptorSets(ctx->device, &setAllocInfo, &tex->descriptorSet) == VK_SUCCESS) {
            VkDescriptorImageInfo descImageInfo{};
            descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descImageInfo.imageView = tex->view;
            descImageInfo.sampler = tex->sampler;

            VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            write.dstSet = tex->descriptorSet;
            write.dstBinding = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &descImageInfo;

            vkUpdateDescriptorSets(ctx->device, 1, &write, 0, nullptr);
        }
    }

    return tex;
}

void DestroyTexture(VulkanWindowContext* ctx, VulkanTexture* tex) {
    if (!ctx || !tex) return;
    if (ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx->device);
        if (tex->descriptorSet != VK_NULL_HANDLE && ctx->descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(ctx->device, ctx->descriptorPool, 1, &tex->descriptorSet);
            tex->descriptorSet = VK_NULL_HANDLE;
        }
        if (tex->sampler) { vkDestroySampler(ctx->device, tex->sampler, nullptr); tex->sampler = VK_NULL_HANDLE; }
        if (tex->view) { vkDestroyImageView(ctx->device, tex->view, nullptr); tex->view = VK_NULL_HANDLE; }
        if (tex->image) { vkDestroyImage(ctx->device, tex->image, nullptr); tex->image = VK_NULL_HANDLE; }
        if (tex->memory) { vkFreeMemory(ctx->device, tex->memory, nullptr); tex->memory = VK_NULL_HANDLE; }
    }
    delete tex;
}

void DrawImage(VulkanWindowContext* ctx, VulkanTexture* tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1, float r, float g, float b, float a) {
    if (!ctx || w <= 0.0f || h <= 0.0f) return;

    if (ctx->drawBatches.empty() || ctx->drawBatches.back().texture != tex) {
        uint32_t first = (uint32_t)ctx->queuedVertices.size();
        ctx->drawBatches.push_back({ tex, first, 0 });
    }

    Vertex2D v00 = { x,     y,     u0, v0, r, g, b, a };
    Vertex2D v10 = { x + w, y,     u1, v0, r, g, b, a };
    Vertex2D v11 = { x + w, y + h, u1, v1, r, g, b, a };
    Vertex2D v01 = { x,     y + h, u0, v1, r, g, b, a };

    ctx->queuedVertices.push_back(v00);
    ctx->queuedVertices.push_back(v10);
    ctx->queuedVertices.push_back(v11);

    ctx->queuedVertices.push_back(v00);
    ctx->queuedVertices.push_back(v11);
    ctx->queuedVertices.push_back(v01);

    ctx->drawBatches.back().vertexCount += 6;
}

void FlushBatch(VulkanWindowContext* ctx) {
    // No-op here; batches are flushed and drawn in RenderAndPresent
}

