package fastvulkan;

import fastcore.FastCore;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class FastVulkanWindow implements AutoCloseable {

    static {
        FastCore.loadLibrary("fastvulkan");
    }

    private long nativeHandle;
    private final long hwnd;

    public FastVulkanWindow(String title, int width, int height) {
        this(title, width, height, 0.098f, 0.098f, 0.098f, 1.0f);
    }

    public FastVulkanWindow(String title, int width, int height, float clearR, float clearG, float clearB, float clearA) {
        this.nativeHandle = nCreateWindow(title, width, height, clearR, clearG, clearB, clearA);
        if (this.nativeHandle == 0) {
            throw new RuntimeException("Failed to create native Vulkan window");
        }
        this.hwnd = nGetHWND(this.nativeHandle);
    }

    // ═══════════════════════════════════════════════════════════
    // Events & Message Loop
    // ═══════════════════════════════════════════════════════════

    public boolean pollEvents() {
        if (nativeHandle == 0) return false;
        return nPollEvents(nativeHandle);
    }

    // ═══════════════════════════════════════════════════════════
    // Normal Methods (Window Control & Lifecycle)
    // ═══════════════════════════════════════════════════════════

    public void present() {
        if (nativeHandle != 0) {
            nRenderAndPresent(nativeHandle);
        }
    }

    public void centerOnScreen() {
        if (nativeHandle != 0) {
            nCenterOnScreen(nativeHandle);
        }
    }

    public void minimize() {
        if (nativeHandle != 0) {
            nMinimize(nativeHandle);
        }
    }

    public void maximize() {
        if (nativeHandle != 0) {
            nMaximize(nativeHandle);
        }
    }

    public void restore() {
        if (nativeHandle != 0) {
            nRestore(nativeHandle);
        }
    }

    @Override
    public void close() {
        if (nativeHandle != 0) {
            nDestroyWindow(nativeHandle);
            nativeHandle = 0;
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Package-Private Rendering & Resource Methods (Internal to FastVulkanGraphics)
    // ═══════════════════════════════════════════════════════════

    long createTexture(int[] pixels, int width, int height, boolean mipmaps) {
        if (nativeHandle == 0) return 0;
        return nCreateTexture(nativeHandle, pixels, width, height, mipmaps);
    }

    boolean updateTexture(long texHandle, int[] pixels, int width, int height) {
        if (nativeHandle == 0 || texHandle == 0) return false;
        return nUpdateTexture(nativeHandle, texHandle, pixels, width, height);
    }

    // FastJava Zero-Copy Direct Memory Pipeline (FastPointer / FastMemory / FastSharedMemory)
    boolean updateTexture(long texHandle, long nativePixelPtr, int width, int height) {
        if (nativeHandle == 0 || texHandle == 0 || nativePixelPtr == 0) return false;
        return nUpdateTexturePointer(nativeHandle, texHandle, nativePixelPtr, width, height);
    }

    void destroyTexture(long texHandle) {
        if (nativeHandle != 0 && texHandle != 0) {
            nDestroyTexture(nativeHandle, texHandle);
        }
    }

    void drawImage(long texHandle, float x, float y, float w, float h,
                   float u0, float v0, float u1, float v1,
                   float r, float g, float b, float a) {
        if (nativeHandle != 0 && texHandle != 0) {
            nDrawImage(nativeHandle, texHandle, x, y, w, h, u0, v0, u1, v1, r, g, b, a);
        }
    }

    void drawColoredRect(float x, float y, float w, float h, float r, float g, float b, float a) {
        if (nativeHandle != 0) {
            nDrawColoredRect(nativeHandle, x, y, w, h, r, g, b, a);
        }
    }

    public void drawColoredOval(float x, float y, float w, float h, float r, float g, float b, float a,
                                 boolean antialias, boolean outline, float strokeWidth) {
        if (nativeHandle != 0) {
            nDrawColoredOval(nativeHandle, x, y, w, h, r, g, b, a, antialias, outline, strokeWidth);
        }
    }

    void drawColoredRoundRect(float x, float y, float w, float h, float rx, float ry,
                              float r, float g, float b, float a,
                              boolean antialias, boolean outline, float strokeWidth) {
        if (nativeHandle != 0) {
            nDrawColoredRoundRect(nativeHandle, x, y, w, h, rx, ry, r, g, b, a, antialias, outline, strokeWidth);
        }
    }

    void drawBezierQuad(float p0x, float p0y, float p1x, float p1y, float p2x, float p2y,
                        float r, float g, float b, float a) {
        if (nativeHandle != 0) {
            nDrawBezierQuad(nativeHandle, p0x, p0y, p1x, p1y, p2x, p2y, r, g, b, a);
        }
    }

    public void drawTexturedTriangles(long texHandle, float[] vertexData, int vertexCount) {
        if (nativeHandle != 0 && vertexData != null && vertexCount > 0) {
            nDrawTexturedTriangles(nativeHandle, texHandle, vertexData, vertexCount);
        }
    }

    void flushBatch() {
        if (nativeHandle != 0) {
            nFlushBatch(nativeHandle);
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Is / Has
    // ═══════════════════════════════════════════════════════════

    public boolean isOpen() {
        return nativeHandle != 0;
    }

    public boolean isFullscreen() {
        if (nativeHandle == 0) return false;
        return nIsFullscreen(nativeHandle);
    }

    // ═══════════════════════════════════════════════════════════
    // Getter
    // ═══════════════════════════════════════════════════════════

    public long getHWND() {
        return hwnd;
    }

    public int getWidth() {
        if (nativeHandle == 0) return 0;
        return nGetWidth(nativeHandle);
    }

    public int getHeight() {
        if (nativeHandle == 0) return 0;
        return nGetHeight(nativeHandle);
    }

    public int getX() {
        if (nativeHandle == 0) return 0;
        return nGetX(nativeHandle);
    }

    public int getY() {
        if (nativeHandle == 0) return 0;
        return nGetY(nativeHandle);
    }

    // ═══════════════════════════════════════════════════════════
    // Setter
    // ═══════════════════════════════════════════════════════════

    public void setTitle(String title) {
        if (nativeHandle != 0) {
            nSetTitle(nativeHandle, title);
        }
    }

    public void setLocation(int x, int y) {
        if (nativeHandle != 0) {
            nSetLocation(nativeHandle, x, y);
        }
    }

    public void setSize(int width, int height) {
        if (nativeHandle != 0) {
            nSetDimensions(nativeHandle, width, height);
        }
    }

    public void setBounds(int x, int y, int width, int height) {
        if (nativeHandle != 0) {
            nSetBounds(nativeHandle, x, y, width, height);
        }
    }

    public void setVisible(boolean visible) {
        if (nativeHandle != 0) {
            nSetVisible(nativeHandle, visible);
        }
    }

    public void setResizable(boolean resizable) {
        if (nativeHandle != 0) {
            nSetResizable(nativeHandle, resizable);
        }
    }

    public void setAlwaysOnTop(boolean alwaysOnTop) {
        if (nativeHandle != 0) {
            nSetAlwaysOnTop(nativeHandle, alwaysOnTop);
        }
    }

    public void setFullscreen(boolean fullscreen) {
        if (nativeHandle != 0) {
            nSetFullscreen(nativeHandle, fullscreen);
        }
    }

    public void setMinimumSize(int minWidth, int minHeight) {
        if (nativeHandle != 0) {
            nSetMinSize(nativeHandle, minWidth, minHeight);
        }
    }

    public void setMaximumSize(int maxWidth, int maxHeight) {
        if (nativeHandle != 0) {
            nSetMaxSize(nativeHandle, maxWidth, maxHeight);
        }
    }

    public void setClearColor(float r, float g, float b, float a) {
        if (nativeHandle != 0) {
            nSetClearColor(nativeHandle, r, g, b, a);
        }
    }

    public void setIconImage(BufferedImage image) {
        if (nativeHandle != 0 && image != null) {
            int w = image.getWidth();
            int h = image.getHeight();
            int[] pixels;

            // Zero-Copy path: directly access underlying int buffer if TYPE_INT_ARGB
            if (image.getType() == BufferedImage.TYPE_INT_ARGB) {
                pixels = ((DataBufferInt) image.getRaster().getDataBuffer()).getData();
            } else {
                pixels = new int[w * h];
                image.getRGB(0, 0, w, h, pixels, 0, w);
            }
            nSetIcon(nativeHandle, pixels, w, h);
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Native Methods (JNI)
    // ═══════════════════════════════════════════════════════════

    private static native long nCreateWindow(String title, int width, int height, float r, float g, float b, float a);

    private static native void nDestroyWindow(long handle);

    private static native boolean nPollEvents(long handle);

    private static native void nRenderAndPresent(long handle);

    private static native void nSetClearColor(long handle, float r, float g, float b, float a);

    private static native void nSetTitle(long handle, String title);

    private static native long nGetHWND(long handle);

    private static native int nGetWidth(long handle);

    private static native int nGetHeight(long handle);

    private static native void nSetLocation(long handle, int x, int y);

    private static native int nGetX(long handle);

    private static native int nGetY(long handle);

    private static native void nSetDimensions(long handle, int width, int height);

    private static native void nSetBounds(long handle, int x, int y, int width, int height);

    private static native void nCenterOnScreen(long handle);

    private static native void nSetVisible(long handle, boolean visible);

    private static native void nSetResizable(long handle, boolean resizable);

    private static native void nSetAlwaysOnTop(long handle, boolean alwaysOnTop);

    private static native void nSetFullscreen(long handle, boolean fullscreen);

    private static native boolean nIsFullscreen(long handle);

    private static native void nMinimize(long handle);

    private static native void nMaximize(long handle);

    private static native void nRestore(long handle);

    private static native void nSetMinSize(long handle, int minW, int minH);

    private static native void nSetMaxSize(long handle, int maxW, int maxH);

    private static native void nSetIcon(long handle, int[] pixels, int width, int height);

    private static native long nCreateTexture(long handle, int[] pixels, int width, int height, boolean mipmaps);

    private static native boolean nUpdateTexture(long handle, long texHandle, int[] pixels, int width, int height);

    private static native boolean nUpdateTexturePointer(long handle, long texHandle, long nativePixelAddress, int width, int height);

    private static native void nDestroyTexture(long handle, long texHandle);

    private static native void nDrawImage(long handle, long texHandle, float x, float y, float w, float h, float u0, float v0, float u1, float v1, float r, float g, float b, float a);

    private static native void nFlushBatch(long handle);

    private static native void nDrawColoredRect(long handle, float x, float y, float w, float h, float r, float g, float b, float a);
    private static native void nDrawColoredOval(long handle, float x, float y, float w, float h, float r, float g, float b, float a, boolean antialias, boolean outline, float strokeWidth);
    private static native void nDrawColoredRoundRect(long handle, float x, float y, float w, float h, float rx, float ry, float r, float g, float b, float a, boolean antialias, boolean outline, float strokeWidth);
    private static native void nDrawBezierQuad(long handle, float p0x, float p0y, float p1x, float p1y, float p2x, float p2y, float r, float g, float b, float a);
    private static native void nDrawTexturedTriangles(long handle, long texHandle, float[] vertexData, int vertexCount);
}
