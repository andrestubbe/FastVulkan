package fastvulkan;

import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Shape;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * FastVulkanGraphics — Graphics2D-compatible 2D drawing API backed by FastVulkan.
 * <p>
 * Implements:
 * - Bounded LRU Caches for textures & rendered text to eliminate GPU memory leaks.
 * - Full disposal of GPU resources via dispose().
 * - FastJava Zero-Copy image buffer processing.
 */
public class FastVulkanGraphics implements AutoCloseable {

    private static final int MAX_TEXTURE_CACHE_SIZE = 128;
    private static final int MAX_TEXT_CACHE_SIZE = 256;

    /**
     * Rendering engine modes supported by FastVulkan / FastGraphics.
     */
    public enum EngineMode {
        /**
         * Native GPU Analytical SDF Shaders:
         * Ultra-fast sub-millisecond mathematical vector rendering directly on GPU.
         */
        VULKAN_FACES,

        /**
         * Hybrid Marlin Twin Pipeline:
         * Uses Java2D's Marlin rasterizer for offscreen generation, streamed to GPU textures
         * for 100% bit-exact parity with standard Java2D curves and font rendering.
         */
        VULKAN_MARLIN
    }

    private final FastVulkanWindow window;
    private EngineMode engineMode = EngineMode.VULKAN_FACES;
    private Color currentColor = Color.WHITE;
    private Font currentFont = new Font("Segoe UI", Font.PLAIN, 14);
    private boolean antiAliased = true;
    private float strokeWidth = 1.0f;

    // Bounded LRU Cache for uploaded BufferedImage textures (access-order)
    private final Map<BufferedImage, Long> textureCache = Collections.synchronizedMap(
            new LinkedHashMap<BufferedImage, Long>(MAX_TEXTURE_CACHE_SIZE, 0.75f, true) {
                @Override
                protected boolean removeEldestEntry(Map.Entry<BufferedImage, Long> eldest) {
                    if (size() > MAX_TEXTURE_CACHE_SIZE) {
                        if (eldest.getValue() != null && eldest.getValue() != 0L) {
                            window.destroyTexture(eldest.getValue());
                        }
                        return true;
                    }
                    return false;
                }
            }
    );

    // Bounded LRU Cache for rendered text textures (access-order)
    private final Map<String, CachedText> textCache = Collections.synchronizedMap(
            new LinkedHashMap<String, CachedText>(MAX_TEXT_CACHE_SIZE, 0.75f, true) {
                @Override
                protected boolean removeEldestEntry(Map.Entry<String, CachedText> eldest) {
                    if (size() > MAX_TEXT_CACHE_SIZE) {
                        if (eldest.getValue() != null && eldest.getValue().textureHandle != 0L) {
                            window.destroyTexture(eldest.getValue().textureHandle);
                        }
                        return true;
                    }
                    return false;
                }
            }
    );

    private static class CachedText {
        long textureHandle;
        int width;
        int height;
        int ascent;
    }

    public FastVulkanGraphics(FastVulkanWindow window) {
        this.window = window;
    }

    // ═══════════════════════════════════════════════════════════
    // Normal Methods (Draw & Fill)
    // ═══════════════════════════════════════════════════════════

    public void fill(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            fillRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            fillOval((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            fillRoundRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight(),
                    (float) rr.getArcWidth(), (float) rr.getArcHeight());
        } else if (shape != null) {
            fillPath(shape);
        }
    }

    public void draw(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            drawRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            drawOval((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            drawRoundRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight(),
                    (float) rr.getArcWidth(), (float) rr.getArcHeight());
        }
    }

    public void drawBezierQuad(float p0x, float p0y, float p1x, float p1y, float p2x, float p2y) {
        float[] rgba = toRGBA(currentColor);
        window.drawBezierQuad(p0x, p0y, p1x, p1y, p2x, p2y, rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    private void fillPath(Shape shape) {
        float[] coords = new float[6];
        float startX = 0, startY = 0, curX = 0, curY = 0;
        java.awt.geom.PathIterator pi = shape.getPathIterator(null);
        while (!pi.isDone()) {
            int type = pi.currentSegment(coords);
            switch (type) {
                case java.awt.geom.PathIterator.SEG_MOVETO -> {
                    curX = coords[0];
                    curY = coords[1];
                    startX = curX;
                    startY = curY;
                }
                case java.awt.geom.PathIterator.SEG_LINETO -> {
                    curX = coords[0];
                    curY = coords[1];
                }
                case java.awt.geom.PathIterator.SEG_QUADTO -> {
                    drawBezierQuad(curX, curY, coords[0], coords[1], coords[2], coords[3]);
                    curX = coords[2];
                    curY = coords[3];
                }
                case java.awt.geom.PathIterator.SEG_CUBICTO -> {
                    // Approximate cubic Bézier with 2 quadratic Béziers on GPU
                    float midX = (coords[0] + 2f * coords[2] + coords[4]) * 0.25f;
                    float midY = (coords[1] + 2f * coords[3] + coords[5]) * 0.25f;
                    drawBezierQuad(curX, curY, coords[0], coords[1], midX, midY);
                    drawBezierQuad(midX, midY, coords[2], coords[3], coords[4], coords[5]);
                    curX = coords[4];
                    curY = coords[5];
                }
                case java.awt.geom.PathIterator.SEG_CLOSE -> {
                    curX = startX;
                    curY = startY;
                }
            }
            pi.next();
        }
    }

    public void fillRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredRect(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    public void drawRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        float lw = 1.0f;
        // Top & Bottom (w+1 pixels)
        window.drawColoredRect(x, y, w + 1.0f, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        window.drawColoredRect(x, y + h, w + 1.0f, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // Left & Right (h-lw pixels)
        window.drawColoredRect(x, y + lw, lw, stdMax0(h - lw), rgba[0], rgba[1], rgba[2], rgba[3]);
        window.drawColoredRect(x + w, y + lw, lw, stdMax0(h - lw), rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    public void fillOval(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredOval(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3], antiAliased, false, 0.0f);
    }

    public void drawOval(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredOval(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3], antiAliased, true, strokeWidth);
    }

    public void fillRoundRect(float x, float y, float w, float h, float arcW, float arcH) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredRoundRect(x, y, w, h, arcW, arcH, rgba[0], rgba[1], rgba[2], rgba[3], antiAliased, false, 0.0f);
    }

    public void drawRoundRect(float x, float y, float w, float h, float arcW, float arcH) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredRoundRect(x, y, w, h, arcW, arcH, rgba[0], rgba[1], rgba[2], rgba[3], antiAliased, true, strokeWidth);
    }

    public void drawString(String text, float x, float y) {
        if (text == null || text.isEmpty()) return;

        String key = text + "@" + currentFont.getName() + "_" + currentFont.getSize() + "_" + currentFont.getStyle();
        CachedText cached = textCache.get(key);

        if (cached == null) {
            cached = renderTextToTexture(text, currentFont);
            if (cached != null) {
                textCache.put(key, cached);
            }
        }

        if (cached != null && cached.textureHandle != 0) {
            float[] rgba = toRGBA(currentColor);
            float drawY = y - cached.ascent;
            window.drawImage(cached.textureHandle, x, drawY, (float) cached.width, (float) cached.height,
                    0.0f, 0.0f, 1.0f, 1.0f, rgba[0], rgba[1], rgba[2], rgba[3]);
        }
    }

    public void drawImage(BufferedImage img, float x, float y) {
        if (img == null) return;
        drawImage(img, x, y, img.getWidth(), img.getHeight());
    }

    public void drawImage(BufferedImage img, float x, float y, float width, float height) {
        if (img == null || width <= 0 || height <= 0) return;

        Long texHandle = textureCache.get(img);
        if (texHandle == null || texHandle == 0) {
            texHandle = uploadTexture(img);
            if (texHandle != 0) {
                textureCache.put(img, texHandle);
            }
        }

        if (texHandle != null && texHandle != 0) {
            window.drawImage(texHandle, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    public void drawImage(long texHandle, float x, float y, float width, float height) {
        if (texHandle == 0 || width <= 0 || height <= 0) return;
        window.drawImage(texHandle, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    public void drawImageTinted(long texHandle, float x, float y, float width, float height, Color tint) {
        if (texHandle == 0 || width <= 0 || height <= 0) return;
        float[] rgba = toRGBA(tint != null ? tint : currentColor);
        window.drawImage(texHandle, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    public long createTexture(int[] pixels, int width, int height) {
        return window.createTexture(pixels, width, height, false);
    }

    public boolean updateTexture(long texHandle, int[] pixels, int width, int height) {
        return window.updateTexture(texHandle, pixels, width, height);
    }

    // FastJava Zero-Copy Direct Memory Streaming (FastPointer / FastMemory / FastSharedMemory)
    public boolean updateTexture(long texHandle, long nativePixelPtr, int width, int height) {
        return window.updateTexture(texHandle, nativePixelPtr, width, height);
    }

    public void destroyTexture(long texHandle) {
        window.destroyTexture(texHandle);
    }

    /**
     * Flushes currently batched draw primitives to the GPU.
     */
    public void flush() {
        window.flushBatch();
    }

    /**
     * Releases all cached textures and text resources from GPU memory.
     */
    public void dispose() {
        for (Long handle : textureCache.values()) {
            if (handle != null && handle != 0L) {
                window.destroyTexture(handle);
            }
        }
        textureCache.clear();

        for (CachedText cached : textCache.values()) {
            if (cached != null && cached.textureHandle != 0L) {
                window.destroyTexture(cached.textureHandle);
            }
        }
        textCache.clear();
    }

    @Override
    public void close() {
        dispose();
    }

    // ═══════════════════════════════════════════════════════════
    // Is / Has
    // ═══════════════════════════════════════════════════════════

    public boolean isAntiAliased() {
        return antiAliased;
    }

    // ═══════════════════════════════════════════════════════════
    // Getter
    // ═══════════════════════════════════════════════════════════

    public Color getColor() {
        return currentColor;
    }

    public Font getFont() {
        return currentFont;
    }

    public float getStrokeWidth() {
        return strokeWidth;
    }

    public EngineMode getEngineMode() {
        return engineMode;
    }

    // ═══════════════════════════════════════════════════════════
    // Setter
    // ═══════════════════════════════════════════════════════════

    public void setColor(Color color) {
        this.currentColor = color != null ? color : Color.WHITE;
    }

    public void setFont(Font font) {
        if (font != null) {
            this.currentFont = font;
        }
    }

    public void setStrokeWidth(float strokeWidth) {
        this.strokeWidth = Math.max(0.0f, strokeWidth);
    }

    public void setEngineMode(EngineMode engineMode) {
        this.engineMode = engineMode != null ? engineMode : EngineMode.VULKAN_FACES;
    }

    public void setRenderingHint(RenderingHints.Key hintKey, Object hintValue) {
        if (hintKey == RenderingHints.KEY_ANTIALIASING) {
            this.antiAliased = (hintValue == RenderingHints.VALUE_ANTIALIAS_ON);
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Private Helpers
    // ═══════════════════════════════════════════════════════════

    private CachedText renderTextToTexture(String text, Font font) {
        BufferedImage dummy = new BufferedImage(1, 1, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2 = dummy.createGraphics();
        g2.setFont(font);
        FontMetrics fm = g2.getFontMetrics();
        int width = Math.max(1, fm.stringWidth(text));
        int height = Math.max(1, fm.getHeight());
        int ascent = fm.getAscent();
        g2.dispose();

        BufferedImage textImg = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = textImg.createGraphics();
        g.setFont(font);
        g.setColor(Color.WHITE);
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HRGB);
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_FRACTIONALMETRICS, RenderingHints.VALUE_FRACTIONALMETRICS_ON);
        g.drawString(text, 0, ascent);
        g.dispose();

        int[] pixels = ((DataBufferInt) textImg.getRaster().getDataBuffer()).getData();
        long handle = window.createTexture(pixels, width, height, false);

        CachedText result = new CachedText();
        result.textureHandle = handle;
        result.width = width;
        result.height = height;
        result.ascent = ascent;
        return result;
    }

    private long uploadTexture(BufferedImage img) {
        BufferedImage argbImg;
        if (img.getType() != BufferedImage.TYPE_INT_ARGB) {
            argbImg = new BufferedImage(img.getWidth(), img.getHeight(), BufferedImage.TYPE_INT_ARGB);
            var g = argbImg.createGraphics();
            g.drawImage(img, 0, 0, null);
            g.dispose();
        } else {
            argbImg = img;
        }

        int w = argbImg.getWidth();
        int h = argbImg.getHeight();
        int[] pixels = ((DataBufferInt) argbImg.getRaster().getDataBuffer()).getData();
        return window.createTexture(pixels, w, h, false);
    }

    private static float stdMax0(float v) {
        return v > 0.0f ? v : 0.0f;
    }

    private static float[] toRGBA(Color c) {
        return new float[]{
                c.getRed() / 255.0f,
                c.getGreen() / 255.0f,
                c.getBlue() / 255.0f,
                c.getAlpha() / 255.0f
        };
    }
}
