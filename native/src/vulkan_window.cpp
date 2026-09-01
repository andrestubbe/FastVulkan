#include "vulkan_window.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <vector>

static const wchar_t* WINDOW_CLASS_NAME = L"FastVulkanWindowClass";

static void CleanupSwapChain(VulkanWindowContext* ctx) {
    if (!ctx || ctx->device == VK_NULL_HANDLE) return;

    for (auto fb : ctx->swapChainFramebuffers) {
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(ctx->device, fb, nullptr);
    }
    ctx->swapChainFramebuffers.clear();

    for (auto iv : ctx->swapChainImageViews) {
        if (iv != VK_NULL_HANDLE) vkDestroyImageView(ctx->device, iv, nullptr);
    }
    ctx->swapChainImageViews.clear();

    if (ctx->swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx->device, ctx->swapChain, nullptr);
        ctx->swapChain = VK_NULL_HANDLE;
    }
}

static void SelectSurfaceFormat(VulkanWindowContext* ctx) {
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, nullptr);
    if (formatCount == 0) {
        ctx->swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        return;
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, formats.data());

    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ctx->swapChainImageFormat = f.format;
            return;
        }
    }
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ctx->swapChainImageFormat = f.format;
            return;
        }
    }
    ctx->swapChainImageFormat = formats[0].format;
}

static void CreateSwapChain(VulkanWindowContext* ctx) {
    RECT rc;
    GetClientRect(ctx->hwnd, &rc);
    uint32_t winW = (std::max)(1, (int)(rc.right - rc.left));
    uint32_t winH = (std::max)(1, (int)(rc.bottom - rc.top));

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);

    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = (std::clamp)(winW, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = (std::clamp)(winH, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Select optimal Present Mode
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (presentModeCount > 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, presentModes.data());
    }

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed
    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    if (chosenPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) {
        for (const auto& mode : presentModes) {
            if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
                chosenPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                break;
            }
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = ctx->swapChainImageFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx->device, &createInfo, nullptr, &ctx->swapChain) != VK_SUCCESS) {
        return;
    }

    ctx->swapChainExtent = extent;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapChain, &count, nullptr);
    ctx->swapChainImages.resize(count);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapChain, &count, ctx->swapChainImages.data());

    ctx->swapChainImageViews.resize(ctx->swapChainImages.size());
    for (size_t i = 0; i < ctx->swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = ctx->swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = ctx->swapChainImageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(ctx->device, &viewInfo, nullptr, &ctx->swapChainImageViews[i]);
    }

    ctx->swapChainFramebuffers.resize(ctx->swapChainImageViews.size());
    for (size_t i = 0; i < ctx->swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { ctx->swapChainImageViews[i] };

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = ctx->renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = ctx->swapChainExtent.width;
        fbInfo.height = ctx->swapChainExtent.height;
        fbInfo.layers = 1;

        vkCreateFramebuffer(ctx->device, &fbInfo, nullptr, &ctx->swapChainFramebuffers[i]);
    }
}

static void RecreateSwapChain(VulkanWindowContext* ctx) {
    RECT rc;
    GetClientRect(ctx->hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    vkDeviceWaitIdle(ctx->device);
    CleanupSwapChain(ctx);
    CreateSwapChain(ctx);
    ctx->resized = false;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    VulkanWindowContext* ctx = (VulkanWindowContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        if (ctx) {
            ctx->resized = true;
            if (wParam != SIZE_MINIMIZED) {
                RecreateSwapChain(ctx);
            }
        }
        return 0;

    case WM_SIZING:
        if (ctx) {
            ctx->resized = true;
            RecreateSwapChain(ctx);
        }
        return 0;

    case WM_GETMINMAXINFO:
        if (ctx) {
            LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
            if (ctx->minWidth > 0) mmi->ptMinTrackSize.x = ctx->minWidth;
            if (ctx->minHeight > 0) mmi->ptMinTrackSize.y = ctx->minHeight;
            if (ctx->maxWidth > 0) mmi->ptMaxTrackSize.x = ctx->maxWidth;
            if (ctx->maxHeight > 0) mmi->ptMaxTrackSize.y = ctx->maxHeight;
            return 0;
        }
        break;

    case WM_CLOSE:
        if (ctx) ctx->shouldClose = true;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

VulkanWindowContext* CreateVulkanWindow(const wchar_t* title, int width, int height, float clearR, float clearG, float clearB, float clearA) {
    auto ctx = new VulkanWindowContext();
    ctx->width = width;
    ctx->height = height;
    ctx->clearR = clearR;
    ctx->clearG = clearG;
    ctx->clearB = clearB;
    ctx->clearA = clearA;
    ctx->hInstance = GetModuleHandle(nullptr);

    // Register Win32 window class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ctx->hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    RegisterClassExW(&wc);

    RECT wr = { 0, 0, width, height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    ctx->hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        WINDOW_CLASS_NAME,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, ctx->hInstance, nullptr
    );

    if (!ctx->hwnd) {
        delete ctx;
        return nullptr;
    }

    SetWindowLongPtr(ctx->hwnd, GWLP_USERDATA, (LONG_PTR)ctx);

    // 1. Create Vulkan Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "FastVulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "FastVulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo createInfoInstance{};
    createInfoInstance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfoInstance.pApplicationInfo = &appInfo;
    createInfoInstance.enabledExtensionCount = 2;
    createInfoInstance.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfoInstance, nullptr, &ctx->instance) != VK_SUCCESS) {
        DestroyWindow(ctx->hwnd);
        delete ctx;
        return nullptr;
    }

    // 2. Win32 Surface
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = ctx->hwnd;
    surfaceInfo.hinstance = ctx->hInstance;

    if (vkCreateWin32SurfaceKHR(ctx->instance, &surfaceInfo, nullptr, &ctx->surface) != VK_SUCCESS) {
        vkDestroyInstance(ctx->instance, nullptr);
        DestroyWindow(ctx->hwnd);
        delete ctx;
        return nullptr;
    }

    // 3. Physical Device Selection
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        DestroyVulkanWindow(ctx);
        return nullptr;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices.data());

    for (const auto& dev : devices) {
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueCount; i++) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, ctx->surface, &presentSupport);

            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
                ctx->physicalDevice = dev;
                ctx->queueFamilyIndex = i;
                break;
            }
        }
        if (ctx->physicalDevice != VK_NULL_HANDLE) break;
    }

    if (ctx->physicalDevice == VK_NULL_HANDLE) {
        DestroyVulkanWindow(ctx);
        return nullptr;
    }

    // Query features & Anisotropy support
    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(ctx->physicalDevice, &supportedFeatures);
    ctx->anisotropySupported = (supportedFeatures.samplerAnisotropy == VK_TRUE);
    if (ctx->anisotropySupported) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(ctx->physicalDevice, &props);
        ctx->maxAnisotropy = props.limits.maxSamplerAnisotropy;
    }

    // 4. Logical Device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = ctx->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures deviceFeatures{};
    if (ctx->anisotropySupported) {
        deviceFeatures.samplerAnisotropy = VK_TRUE;
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (vkCreateDevice(ctx->physicalDevice, &deviceCreateInfo, nullptr, &ctx->device) != VK_SUCCESS) {
        DestroyVulkanWindow(ctx);
        return nullptr;
    }

    vkGetDeviceQueue(ctx->device, ctx->queueFamilyIndex, 0, &ctx->graphicsQueue);

    // Select Surface Format before Render Pass creation
    SelectSurfaceFormat(ctx);

    // 5. Render Pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = ctx->swapChainImageFormat;
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

    if (vkCreateRenderPass(ctx->device, &renderPassInfo, nullptr, &ctx->renderPass) != VK_SUCCESS) {
        DestroyVulkanWindow(ctx);
        return nullptr;
    }

    // 6. Swapchain
    CreateSwapChain(ctx);

    // 7. Command Pool & Command Buffers
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ctx->queueFamilyIndex;

    if (vkCreateCommandPool(ctx->device, &poolInfo, nullptr, &ctx->commandPool) != VK_SUCCESS) {
        DestroyVulkanWindow(ctx);
        return nullptr;
    }

    ctx->commandBuffers.resize(ctx->MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfoCmd{};
    allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.commandPool = ctx->commandPool;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = (uint32_t)ctx->commandBuffers.size();

    vkAllocateCommandBuffers(ctx->device, &allocInfoCmd, ctx->commandBuffers.data());

    // 8. Synchronization Objects
    ctx->imageAvailableSemaphores.resize(ctx->MAX_FRAMES_IN_FLIGHT);
    ctx->renderFinishedSemaphores.resize(ctx->MAX_FRAMES_IN_FLIGHT);
    ctx->inFlightFences.resize(ctx->MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < ctx->MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(ctx->device, &semInfo, nullptr, &ctx->imageAvailableSemaphores[i]);
        vkCreateSemaphore(ctx->device, &semInfo, nullptr, &ctx->renderFinishedSemaphores[i]);
        vkCreateFence(ctx->device, &fenceInfo, nullptr, &ctx->inFlightFences[i]);
    }

    // 9. Initialize 2D Shader Pipeline & Dynamic Vertex Buffers
    Init2DPipeline(ctx);

    ShowWindow(ctx->hwnd, SW_SHOW);
    UpdateWindow(ctx->hwnd);

    return ctx;
}

void DestroyVulkanWindow(VulkanWindowContext* ctx) {
    if (!ctx) return;

    if (ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx->device);

        if (ctx->graphicsPipeline) { vkDestroyPipeline(ctx->device, ctx->graphicsPipeline, nullptr); ctx->graphicsPipeline = VK_NULL_HANDLE; }
        if (ctx->pipelineLayout) { vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, nullptr); ctx->pipelineLayout = VK_NULL_HANDLE; }
        if (ctx->descriptorSetLayout) { vkDestroyDescriptorSetLayout(ctx->device, ctx->descriptorSetLayout, nullptr); ctx->descriptorSetLayout = VK_NULL_HANDLE; }
        if (ctx->descriptorPool) { vkDestroyDescriptorPool(ctx->device, ctx->descriptorPool, nullptr); ctx->descriptorPool = VK_NULL_HANDLE; }

        if (ctx->vertexBuffer) {
            if (ctx->vertexBufferMapped) {
                vkUnmapMemory(ctx->device, ctx->vertexBufferMemory);
                ctx->vertexBufferMapped = nullptr;
            }
            vkDestroyBuffer(ctx->device, ctx->vertexBuffer, nullptr);
            ctx->vertexBuffer = VK_NULL_HANDLE;
            vkFreeMemory(ctx->device, ctx->vertexBufferMemory, nullptr);
            ctx->vertexBufferMemory = VK_NULL_HANDLE;
        }

        for (int i = 0; i < ctx->MAX_FRAMES_IN_FLIGHT; i++) {
            if (ctx->imageAvailableSemaphores.size() > i && ctx->imageAvailableSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphores[i], nullptr);
            if (ctx->renderFinishedSemaphores.size() > i && ctx->renderFinishedSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphores[i], nullptr);
            if (ctx->inFlightFences.size() > i && ctx->inFlightFences[i] != VK_NULL_HANDLE)
                vkDestroyFence(ctx->device, ctx->inFlightFences[i], nullptr);
        }

        CleanupSwapChain(ctx);

        if (ctx->commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(ctx->device, ctx->commandPool, nullptr);
        }

        if (ctx->renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(ctx->device, ctx->renderPass, nullptr);
        }

        vkDestroyDevice(ctx->device, nullptr);
    }

    if (ctx->instance != VK_NULL_HANDLE) {
        if (ctx->surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
        }
        vkDestroyInstance(ctx->instance, nullptr);
    }

    if (ctx->hwnd) {
        DestroyWindow(ctx->hwnd);
    }

    delete ctx;
}

bool PollWindowEvents(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return false;

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            ctx->shouldClose = true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return !ctx->shouldClose;
}

void SetClearColor(VulkanWindowContext* ctx, float r, float g, float b, float a) {
    if (!ctx) return;
    ctx->clearR = r;
    ctx->clearG = g;
    ctx->clearB = b;
    ctx->clearA = a;
}

void SetWindowTitle(VulkanWindowContext* ctx, const wchar_t* title) {
    if (!ctx || !ctx->hwnd || !title) return;
    SetWindowTextW(ctx->hwnd, title);
}

int GetWindowWidth(VulkanWindowContext* ctx) {
    if (!ctx) return 0;
    return (int)ctx->swapChainExtent.width;
}

int GetWindowHeight(VulkanWindowContext* ctx) {
    if (!ctx) return 0;
    return (int)ctx->swapChainExtent.height;
}

void SetWindowLocation(VulkanWindowContext* ctx, int x, int y) {
    if (!ctx || !ctx->hwnd) return;
    SetWindowPos(ctx->hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

int GetWindowX(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return 0;
    RECT rc;
    GetWindowRect(ctx->hwnd, &rc);
    return rc.left;
}

int GetWindowY(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return 0;
    RECT rc;
    GetWindowRect(ctx->hwnd, &rc);
    return rc.top;
}

void SetWindowDimensions(VulkanWindowContext* ctx, int width, int height) {
    if (!ctx || !ctx->hwnd) return;
    RECT wr = { 0, 0, width, height };
    AdjustWindowRect(&wr, GetWindowLong(ctx->hwnd, GWL_STYLE), FALSE);
    SetWindowPos(ctx->hwnd, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void SetWindowBounds(VulkanWindowContext* ctx, int x, int y, int width, int height) {
    if (!ctx || !ctx->hwnd) return;
    RECT wr = { 0, 0, width, height };
    AdjustWindowRect(&wr, GetWindowLong(ctx->hwnd, GWL_STYLE), FALSE);
    SetWindowPos(ctx->hwnd, nullptr, x, y, wr.right - wr.left, wr.bottom - wr.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

void CenterWindowOnScreen(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return;
    RECT rc;
    GetWindowRect(ctx->hwnd, &rc);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    HMONITOR hMon = MonitorFromWindow(ctx->hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(hMon, &mi)) {
        int monW = mi.rcWork.right - mi.rcWork.left;
        int monH = mi.rcWork.bottom - mi.rcWork.top;
        int posX = mi.rcWork.left + (monW - winW) / 2;
        int posY = mi.rcWork.top + (monH - winH) / 2;
        SetWindowPos(ctx->hwnd, nullptr, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void SetWindowVisible(VulkanWindowContext* ctx, bool visible) {
    if (!ctx || !ctx->hwnd) return;
    ShowWindow(ctx->hwnd, visible ? SW_SHOW : SW_HIDE);
    if (visible) UpdateWindow(ctx->hwnd);
}

void SetWindowResizable(VulkanWindowContext* ctx, bool resizable) {
    if (!ctx || !ctx->hwnd) return;
    LONG_PTR style = GetWindowLongPtr(ctx->hwnd, GWL_STYLE);
    if (resizable) {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    SetWindowLongPtr(ctx->hwnd, GWL_STYLE, style);
    SetWindowPos(ctx->hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void SetWindowAlwaysOnTop(VulkanWindowContext* ctx, bool alwaysOnTop) {
    if (!ctx || !ctx->hwnd) return;
    SetWindowPos(ctx->hwnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SetWindowFullscreen(VulkanWindowContext* ctx, bool fullscreen) {
    if (!ctx || !ctx->hwnd || ctx->isFullscreen == fullscreen) return;

    if (fullscreen) {
        GetWindowRect(ctx->hwnd, &ctx->savedWindowRect);
        ctx->savedStyle = (DWORD)GetWindowLong(ctx->hwnd, GWL_STYLE);
        ctx->savedExStyle = (DWORD)GetWindowLong(ctx->hwnd, GWL_EXSTYLE);

        HMONITOR hMon = MonitorFromWindow(ctx->hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(hMon, &mi)) {
            SetWindowLong(ctx->hwnd, GWL_STYLE, ctx->savedStyle & ~(WS_CAPTION | WS_THICKFRAME));
            SetWindowLong(ctx->hwnd, GWL_EXSTYLE, ctx->savedExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
            SetWindowPos(ctx->hwnd, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            ctx->isFullscreen = true;
        }
    } else {
        SetWindowLong(ctx->hwnd, GWL_STYLE, ctx->savedStyle);
        SetWindowLong(ctx->hwnd, GWL_EXSTYLE, ctx->savedExStyle);
        SetWindowPos(ctx->hwnd, HWND_NOTOPMOST,
                     ctx->savedWindowRect.left, ctx->savedWindowRect.top,
                     ctx->savedWindowRect.right - ctx->savedWindowRect.left,
                     ctx->savedWindowRect.bottom - ctx->savedWindowRect.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        ctx->isFullscreen = false;
    }
}

bool IsWindowFullscreen(VulkanWindowContext* ctx) {
    if (!ctx) return false;
    return ctx->isFullscreen;
}

void MinimizeWindow(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return;
    ShowWindow(ctx->hwnd, SW_MINIMIZE);
}

void MaximizeWindow(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return;
    ShowWindow(ctx->hwnd, SW_MAXIMIZE);
}

void RestoreWindow(VulkanWindowContext* ctx) {
    if (!ctx || !ctx->hwnd) return;
    ShowWindow(ctx->hwnd, SW_RESTORE);
}

void SetWindowMinSize(VulkanWindowContext* ctx, int minW, int minH) {
    if (!ctx) return;
    ctx->minWidth = minW;
    ctx->minHeight = minH;
}

void SetWindowMaxSize(VulkanWindowContext* ctx, int maxW, int maxH) {
    if (!ctx) return;
    ctx->maxWidth = maxW;
    ctx->maxHeight = maxH;
}

void RenderAndPresent(VulkanWindowContext* ctx) {
    if (!ctx || ctx->device == VK_NULL_HANDLE || ctx->swapChain == VK_NULL_HANDLE) return;

    vkWaitForFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        ctx->device,
        ctx->swapChain,
        UINT64_MAX,
        ctx->imageAvailableSemaphores[ctx->currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(ctx);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return;
    }

    vkResetFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame]);

    VkCommandBuffer cmd = ctx->commandBuffers[ctx->currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(cmd, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx->renderPass;
    renderPassInfo.framebuffer = ctx->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = ctx->swapChainExtent;

    VkClearValue clearColor = { {{ctx->clearR, ctx->clearG, ctx->clearB, ctx->clearA}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 2D Rendering Pipeline Dispatch
    if (ctx->graphicsPipeline != VK_NULL_HANDLE && !ctx->queuedVertices.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)ctx->swapChainExtent.width;
        viewport.height = (float)ctx->swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = ctx->swapChainExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Ortho Projection Matrix
        float left = 0.0f, right = (float)ctx->swapChainExtent.width;
        float top = 0.0f, bottom = (float)ctx->swapChainExtent.height;
        float ortho[16] = {
            2.0f / (right - left), 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / (bottom - top), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -(right + left) / (right - left), -(bottom + top) / (bottom - top), 0.0f, 1.0f
        };

        vkCmdPushConstants(cmd, ctx->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ortho), ortho);

        size_t count = (std::min)(ctx->queuedVertices.size(), ctx->MAX_VERTICES);
        memcpy(ctx->vertexBufferMapped, ctx->queuedVertices.data(), sizeof(Vertex2D) * count);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, &ctx->vertexBuffer, offsets);

        // Bind cached Descriptor Set from texture directly (no per-frame allocations)
        if (ctx->currentTexture && ctx->currentTexture->descriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    ctx->pipelineLayout, 0, 1,
                                    &ctx->currentTexture->descriptorSet, 0, nullptr);
        }

        vkCmdDraw(cmd, (uint32_t)count, 1, 0, 0);
        ctx->queuedVertices.clear();
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { ctx->imageAvailableSemaphores[ctx->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { ctx->renderFinishedSemaphores[ctx->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFences[ctx->currentFrame]) != VK_SUCCESS) {
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { ctx->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(ctx->graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || ctx->resized) {
        ctx->resized = false;
        RecreateSwapChain(ctx);
    }

    ctx->currentFrame = (ctx->currentFrame + 1) % ctx->MAX_FRAMES_IN_FLIGHT;
}
