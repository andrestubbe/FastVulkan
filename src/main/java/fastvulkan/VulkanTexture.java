package fastvulkan;

import fastgraphics.backend.BackendTexture;

public class VulkanTexture implements BackendTexture {

    private final long handle;
    private final int width;
    private final int height;

    public VulkanTexture(long handle, int width, int height) {
        this.handle = handle;
        this.width = width;
        this.height = height;
    }

    @Override
    public long getHandle() {
        return handle;
    }

    @Override
    public int getWidth() {
        return width;
    }

    @Override
    public int getHeight() {
        return height;
    }

    @Override
    public void close() {
        // Managed by VulkanBackend
    }
}
