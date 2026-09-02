package fastvulkan;

import java.awt.Color;
import java.awt.Shape;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.util.Map;
import java.util.WeakHashMap;

/**
 * FastVulkanGraphics — Graphics2D-compatible 2D drawing API backed by FastVulkan.
 *
 * Supports:
 *  - setColor(Color)
 *  - fill(Shape) / draw(Shape) for Rectangle2D
 *  - drawImage(BufferedImage, ...) with automatic Vulkan Texture Caching
 */
public class FastVulkanGraphics {

    private final FastVulkanWindow window;
    private Color currentColor = Color.WHITE;
    private boolean antiAliased = true;
    private float strokeWidth = 1.0f;
    private final Map<BufferedImage, Long> textureCache = new WeakHashMap<>();

    public FastVulkanGraphics(FastVulkanWindow window) {
        this.window = window;
    }

    public void setColor(Color color) {
        this.currentColor = color != null ? color : Color.WHITE;
    }

    public Color getColor() {
        return currentColor;
    }

    public void setRenderingHint(java.awt.RenderingHints.Key hintKey, Object hintValue) {
        if (hintKey == java.awt.RenderingHints.KEY_ANTIALIASING) {
            this.antiAliased = (hintValue == java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
        }
    }

    public void setStrokeWidth(float strokeWidth) {
        this.strokeWidth = strokeWidth;
    }

    // --- fill(Shape) dispatcher ---

    public void fill(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            fillRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            fillOval((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            fillRoundRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight(), (float) rr.getArcWidth(), (float) rr.getArcHeight());
        }
    }

    // --- draw(Shape) dispatcher ---

    public void draw(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            drawRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            drawOval((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            drawRoundRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight(), (float) rr.getArcWidth(), (float) rr.getArcHeight());
        }
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

    // --- Image Rendering with Texture Caching ---

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
            float[] rgba = toRGBA(currentColor);
            window.drawImage(texHandle, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, rgba[0], rgba[1], rgba[2], rgba[3]);
        }
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

    // --- Primitives ---

    public void fillRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredRect(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    public void drawRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        float lw = 1.0f;
        // Java2D draw(Rectangle2D.Float(x, y, w, h)) draws with standard stroke covering [x..x+w] and [y..y+h] (w+1 and h+1 pixels total)
        // top
        window.drawColoredRect(x, y, w + 1.0f, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // bottom
        window.drawColoredRect(x, y + h, w + 1.0f, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // left
        window.drawColoredRect(x, y + lw, lw, (stdMax0(h - lw)), rgba[0], rgba[1], rgba[2], rgba[3]);
        // right
        window.drawColoredRect(x + w, y + lw, lw, (stdMax0(h - lw)), rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    private static float stdMax0(float v) {
        return v > 0.0f ? v : 0.0f;
    }

    // --- Utility ---

    private static float[] toRGBA(Color c) {
        return new float[]{
            c.getRed()   / 255.0f,
            c.getGreen() / 255.0f,
            c.getBlue()  / 255.0f,
            c.getAlpha() / 255.0f
        };
    }
}
