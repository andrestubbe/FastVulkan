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

    public void present() {
        if (nativeHandle != 0) {
            nRenderAndPresent(nativeHandle);
        }
    }

    public long getHWND() {
        return hwnd;
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
}
