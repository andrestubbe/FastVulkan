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

---

## Installation

### Maven (via JitPack)

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependencies>
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastVulkan</artifactId>
        <version>0.1.0</version>
    </dependency>
</dependencies>
```

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
