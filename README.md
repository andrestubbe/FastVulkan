# FastVulkan 0.1.0 [ALPHA-2026-08] — High-Performance Native Vulkan 2D Rendering & Window Engine for Java

[![Status](https://img.shields.io/badge/status-0.1.0-brightgreen.svg)](https://github.com/andrestubbe/FastVulkan/releases/tag/0.1.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Java](https://img.shields.io/badge/Java-17+-blue.svg)](https://www.java.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010+-lightgrey.svg)]()
[![JitPack](https://img.shields.io/badge/JitPack-0.1.0-green.svg)](https://jitpack.io/#andrestubbe/FastVulkan)

---

**🌋 Ultra-fast native Vulkan 2D batch rendering and Win32 zero-latency window engine for Java, designed to power FastUI.** Built for maximum framerates, zero JVM Garbage Collection overhead, and seamless live window resizing.

FastVulkan provides a low-overhead GPU-accelerated 2D pipeline (instanced shapes, quad batching, texture rendering, and smooth zoom/transforms) with native C++ window management.

---

## Quick Start — Example

```java
import fastvulkan.FastVulkanWindow;
import fastvulkan.FastVulkanGraphics;

public class Demo {
    public static void main(String[] args) {
        try (FastVulkanWindow window = new FastVulkanWindow("FastVulkan Demo", 1280, 720)) {
            FastVulkanGraphics g = window.getGraphics();
            
            while (window.isOpen()) {
                window.pollEvents();
                
                g.clear(0.08f, 0.08f, 0.08f);
                
                // Batch-rendered primitives
                g.setColor(1.0f, 0.2f, 0.2f, 1.0f);
                g.fillRect(50, 50, 200, 100);
                
                g.setColor(0.2f, 0.6f, 1.0f, 1.0f);
                g.fillRoundRect(300, 50, 200, 100, 16.0f);
                
                window.present();
            }
        }
    }
}
```

---

## Table of Contents

- [Why FastVulkan?](#why-fastvulkan)
- [Key Features](#key-features)
- [Architecture & Progressive Roadmap](#architecture--progressive-roadmap)
- [Installation](#installation)
- [Documentation](#documentation)
- [Platform Support](#platform-support)
- [License](#license)
- [Related Projects](#related-projects)

---

## Why FastVulkan?

Standard Java GUI toolkits suffer from thread synchronization overhead, laggy window resizing, and CPU rasterization bottlenecks. FastVulkan solves this with:

- **FastGPU Core Foundation**: Leverages FastGPU for low-overhead Vulkan device initialization, off-heap memory management, and swapchain coordination.
- **Native Win32 Message Loop**: Latency-free live window resize (`WM_SIZE` / `WM_SIZING`) without Java thread blocking.
- **Instanced Quad Batching**: Thousands of shapes rendered in a single Vulkan command buffer dispatch.
- **Zero-GC Architecture**: Off-heap vertex generation and direct native buffer exchanges.
- **GPU Texture & Zoom Pipeline**: High-speed image uploads and smooth bilinear sampling.

---

## Key Features

- 🌋 **Vulkan 1.3 2D Render Engine**: Built on top of **FastGPU** with ultra-low overhead SPIR-V shaders for real-time shapes and texture mapping.
- 🪟 **Native Win32 Windowing**: Native `CreateWindowExW` loop with crisp DPI awareness and direct frame presents.
- ⚡ **Instanced Batching**: Batch thousands of textured quads, rectangles, and rounded shapes in 1 draw call.
- 🔍 **Smooth Zoom & Viewport Transform**: High-performance pan/zoom matrix math executed entirely on GPU.
- 📦 **FastJava Ecosystem Ready**: Interoperates seamlessly with **FastGPU**, **FastImage**, **FastUI**, and **FastCore**.

---

## Architecture & Progressive Roadmap

1. ✅ **Repository Scaffolding & CI**: Standalone native build system and project layout.
2. 🔄 **Native Window & SwapChain**: Win32 window creation and flicker-free resize handling.
3. 🔄 **Texture & Image Pipeline**: Direct memory bitmap uploads, scaling, and viewport zooming.
4. 🔄 **2D Shape Batching**: Rectangles, rounded corners, circles, anti-aliased lines.
5. 🔄 **FastGraphics / FastUI Integration**: Exporting clean abstraction layers for downstream FastJava modules.

## Real-World Use Cases

- 🖥️ **High-FPS UI Frameworks ([FastUI](https://github.com/andrestubbe/FastUI))**: Power complex desktop dashboards, rich vector controls, and live animations with zero GC pauses.
- 🖼️ **Real-Time Image & Video Viewports**: Seamlessly render, pan, and smoothly zoom 4K/8K bitmaps streamed from **[FastImage](https://github.com/andrestubbe/FastImage)** and **[FastScreen](https://github.com/andrestubbe/FastScreen)**.
- 🎮 **2D Game Engines & Particle Canvas**: Render tens of thousands of batch-instanced sprites, lines, and HUD shapes at 1000+ FPS.
- 📊 **Scientific & Financial Charting**: Real-time high-frequency candlestick, waveform, and scatter data visualization with instant window resizing.

---

## Installation

### Option 1: Maven (Recommended)

Add the JitPack repository and the dependency stack to your `pom.xml`:

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependencies>
    <!-- FastVulkan 2D Engine & Windowing -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastVulkan</artifactId>
        <version>0.1.0</version>
    </dependency>

    <!-- FastGPU Native Acceleration Engine -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>fastgpu</artifactId>
        <version>0.1.1</version>
    </dependency>

    <!-- FastDWM Native VSync & Precision Timing -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastDWM</artifactId>
        <version>0.1.0</version>
    </dependency>

    <!-- FastExecution Precision Scheduling Engine -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastExecution</artifactId>
        <version>0.1.0</version>
    </dependency>

    <!-- FastTheme Native Styling & Dynamic Theming -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastTheme</artifactId>
        <version>0.1.4</version>
    </dependency>

    <!-- FastCore Unified JNI Loader -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastCore</artifactId>
        <version>0.1.0</version>
    </dependency>
</dependencies>
```

### Option 2: Gradle (via JitPack)

```groovy
repositories {
    maven { url 'https://jitpack.io' }
}

dependencies {
    implementation 'com.github.andrestubbe:FastVulkan:0.1.0'
    implementation 'com.github.andrestubbe:fastgpu:0.1.1'
    implementation 'com.github.andrestubbe:FastDWM:0.1.0'
    implementation 'com.github.andrestubbe:FastExecution:0.1.0'
    implementation 'com.github.andrestubbe:FastTheme:0.1.4'
    implementation 'com.github.andrestubbe:FastCore:0.1.0'
}
```

### Option 3: Direct Download (No Build Tool)

Download the required JARs directly to add them to your classpath:

1. 📦 **[FastVulkan-0.1.0.jar](https://github.com/andrestubbe/FastVulkan/releases/download/0.1.0/FastVulkan-0.1.0.jar)** (The Core Library)
2. ⚡ **[fastgpu-0.1.1.jar](https://github.com/andrestubbe/FastGPU/releases/download/0.1.1/fastgpu-0.1.1.jar)** (GPU Compute & Memory Foundation)
3. ⏱️ **[fastdwm-0.1.0.jar](https://github.com/andrestubbe/FastDWM/releases/download/0.1.0/fastdwm-0.1.0.jar)** (Hardware VSync & Timing)
4. ⚙️ **[fastexecution-0.1.0.jar](https://github.com/andrestubbe/FastExecution/releases/download/0.1.0/fastexecution-0.1.0.jar)** (Precision Scheduling)
5. 🎨 **[FastTheme-0.1.4.jar](https://github.com/andrestubbe/FastTheme/releases/download/0.1.4/FastTheme-0.1.4.jar)** (Native Window Styling)
6. 🚀 **[fastcore-0.1.0.jar](https://github.com/andrestubbe/FastCore/releases/download/0.1.0/fastcore-0.1.0.jar)** (Mandatory Native Loader)

> [!IMPORTANT]
> All JARs must be included in your classpath for the native Vulkan JNI bindings to function correctly.

---

## Documentation

- **[CHANGELOG.md](docs/CHANGELOG.md)**: Release notes and version history.
- **[REFERENCE.md](docs/REFERENCE.md)**: API contract and methods.
- **[PHILOSOPHY.md](docs/PHILOSOPHY.md)**: Design principles and Zero-GC architecture.
- **[COMPILE.md](docs/COMPILE.md)**: Native C++ compilation guide.
- **[ROADMAP.md](docs/ROADMAP.md)**: Detailed feature roadmap.

---

## Platform Support

| Platform | Status | Notes |
|:---|:---:|:---|
| Windows 10/11 (x64) | ✅ Supported | Native Win32 + Vulkan 1.3 |
| Linux | 🔄 Planned | X11 / Wayland + Vulkan |
| macOS | 🔄 Planned | MoltenVK |

---

## License

MIT License — Free for commercial and personal use. See [LICENSE](LICENSE) for details.

---

## Related Projects

- [FastGPU](https://github.com/andrestubbe/FastGPU) — Native GPU compute engine
- [FastGraphics](https://github.com/andrestubbe/FastGraphics) — Hardware-accelerated DirectX graphics
- [FastImage](https://github.com/andrestubbe/FastImage) — Native SIMD image processing engine
- [FastCore](https://github.com/andrestubbe/FastCore) — Native JNI loader

---

Part of the FastJava Ecosystem — Making the JVM faster. Small package. Maximum speed. Zero bloat. ⚡
