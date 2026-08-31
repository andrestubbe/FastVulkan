package fastvulkan;

import fastcore.FastCore;

public class FastVulkanWindow implements AutoCloseable {

    static {
        FastCore.loadLibrary("fastvulkan");
    }

    private long nativeHandle;
    private final long hwnd;

    public FastVulkanWindow(String title, int width, int height) {
        this(title, width, height, 0.88f, 0.12f, 0.12f, 1.0f);
    }

    public FastVulkanWindow(String title, int width, int height, float clearR, float clearG, float clearB, float clearA) {
        this.nativeHandle = nCreateWindow(title, width, height, clearR, clearG, clearB, clearA);
        if (this.nativeHandle == 0) {
            throw new RuntimeException("Failed to create native Vulkan window");
        }
        this.hwnd = nGetHWND(this.nativeHandle);
    }

    public boolean pollEvents() {
        if (nativeHandle == 0) return false;
        return nPollEvents(nativeHandle);
    }

    public boolean isOpen() {
        return nativeHandle != 0;
    }

    public void setClearColor(float r, float g, float b, float a) {
        if (nativeHandle != 0) {
            nSetClearColor(nativeHandle, r, g, b, a);
        }
    }

    public void setTitle(String title) {
        if (nativeHandle != 0) {
            nSetTitle(nativeHandle, title);
        }
    }

    public long createTexture(int[] pixels, int width, int height, boolean mipmaps) {
        if (nativeHandle == 0) return 0;
        return nCreateTexture(nativeHandle, pixels, width, height, mipmaps);
    }

    public void destroyTexture(long texHandle) {
        if (nativeHandle != 0 && texHandle != 0) {
            nDestroyTexture(nativeHandle, texHandle);
        }
    }

    public void drawImage(long texHandle, float x, float y, float w, float h) {
        drawImage(texHandle, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    public void drawImage(long texHandle, float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          float r, float g, float b, float a) {
        if (nativeHandle != 0 && texHandle != 0) {
            nDrawImage(nativeHandle, texHandle, x, y, w, h, u0, v0, u1, v1, r, g, b, a);
        }
    }

    public void flushBatch() {
        if (nativeHandle != 0) {
            nFlushBatch(nativeHandle);
        }
    }

    public void present() {
        if (nativeHandle != 0) {
            nRenderAndPresent(nativeHandle);
        }
    }

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

    @Override
    public void close() {
        if (nativeHandle != 0) {
            nDestroyWindow(nativeHandle);
            nativeHandle = 0;
        }
    }

    // JNI Native methods
    private static native long nCreateWindow(String title, int width, int height, float r, float g, float b, float a);
    private static native void nDestroyWindow(long handle);
    private static native boolean nPollEvents(long handle);
    private static native void nRenderAndPresent(long handle);
    private static native void nSetClearColor(long handle, float r, float g, float b, float a);
    private static native void nSetTitle(long handle, String title);
    private static native long nGetHWND(long handle);
    private static native int nGetWidth(long handle);
    private static native int nGetHeight(long handle);
    private static native long nCreateTexture(long handle, int[] pixels, int width, int height, boolean mipmaps);
    private static native void nDestroyTexture(long handle, long texHandle);
    private static native void nDrawImage(long handle, long texHandle, float x, float y, float w, float h, float u0, float v0, float u1, float v1, float r, float g, float b, float a);
    private static native void nFlushBatch(long handle);
}
