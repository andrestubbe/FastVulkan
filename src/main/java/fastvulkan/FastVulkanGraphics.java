package fastvulkan;

import java.awt.Color;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.geom.Ellipse2D;
import java.awt.Shape;

/**
 * FastVulkanGraphics — Graphics2D-compatible 2D drawing API backed by FastVulkan.
 *
 * Usage:
 *   FastVulkanGraphics g = new FastVulkanGraphics(window);
 *   g.setColor(new Color(235, 75, 75));
 *   g.fill(new Rectangle2D.Float(60, 60, 160, 100));
 */
public class FastVulkanGraphics {

    private final FastVulkanWindow window;
    private Color currentColor = Color.WHITE;

    public FastVulkanGraphics(FastVulkanWindow window) {
        this.window = window;
    }

    public void setColor(Color color) {
        this.currentColor = color;
    }

    public Color getColor() {
        return currentColor;
    }

    // --- fill(Shape) dispatcher ---

    public void fill(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            fillRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            // TODO: native rounded rect
            fillRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            // TODO: native ellipse
            fillRect((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        }
    }

    // --- draw(Shape) dispatcher ---

    public void draw(Shape shape) {
        if (shape instanceof Rectangle2D r) {
            drawRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), (float) r.getHeight());
        } else if (shape instanceof RoundRectangle2D rr) {
            drawRect((float) rr.getX(), (float) rr.getY(), (float) rr.getWidth(), (float) rr.getHeight());
        } else if (shape instanceof Ellipse2D e) {
            drawRect((float) e.getX(), (float) e.getY(), (float) e.getWidth(), (float) e.getHeight());
        }
    }

    // --- Primitives ---

    public void fillRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        window.drawColoredRect(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    public void drawRect(float x, float y, float w, float h) {
        float[] rgba = toRGBA(currentColor);
        float lw = 1.0f;
        // top
        window.drawColoredRect(x, y, w, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // bottom
        window.drawColoredRect(x, y + h - lw, w, lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // left
        window.drawColoredRect(x, y + lw, lw, h - 2 * lw, rgba[0], rgba[1], rgba[2], rgba[3]);
        // right
        window.drawColoredRect(x + w - lw, y + lw, lw, h - 2 * lw, rgba[0], rgba[1], rgba[2], rgba[3]);
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
