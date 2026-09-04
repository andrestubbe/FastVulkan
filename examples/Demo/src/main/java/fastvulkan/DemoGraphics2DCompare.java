package fastvulkan;

import fastdwm.FastDWM;
import fastkeyboard.FastKeyboard;
import fastkeyboard.Keys;
import fasttheme.FastTheme;

import java.awt.*;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class DemoGraphics2DCompare {

    private static boolean showJava2D = true;

    // Homogeneous 100x100 Test Image
    private static BufferedImage createTestImage(int size) {
        BufferedImage img = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        // Nested circles
        g.setColor(new Color(40, 140, 240));
        g.fillOval(2, 2, size - 4, size - 4);
        g.setColor(new Color(255, 200, 50));
        g.fillOval(16, 16, size - 32, size - 32);
        g.setColor(new Color(240, 60, 60));
        g.fillOval(30, 30, size - 60, size - 60);

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 14));
        g.drawString("IMG", size / 2 - 14, size / 2 + 5);
        g.dispose();
        return img;
    }

    private static void renderJava2DScene(Graphics2D g, int w, int h, BufferedImage testImg) {
        g.setColor(new Color(20, 20, 20));
        g.fillRect(0, 0, w, h);

        final float S = 100.0f; // Homogeneous 100x100 size for all shapes
        final float Y1 = 40.0f; // Row 1 (AA)
        final float Y2 = 170.0f; // Row 2 (AA Outlines)
        final float Y3 = 300.0f; // Row 3 (Non-AA)

        // ═══════════════════════════════════════════════════════════
        // Row 1: Filled Shapes with 16x Subpixel AA (100x100)
        // ═══════════════════════════════════════════════════════════
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        // 1. Rectangle
        g.setColor(new Color(220, 50, 50));
        g.fill(new Rectangle2D.Float(40, Y1, S, S));

        // 2. RoundRectangle
        g.setColor(new Color(50, 200, 120));
        g.fill(new RoundRectangle2D.Float(170, Y1, S, S, 35, 35));

        // 3. Circle
        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(300, Y1, S, S));

        // 4. Test Image
        g.drawImage(testImg, 430, (int)Y1, null);

        // ═══════════════════════════════════════════════════════════
        // Row 2: Outlined Shapes with 16x Subpixel AA (100x100)
        // ═══════════════════════════════════════════════════════════
        g.setStroke(new BasicStroke(1.0f));

        // 5. Rectangle Outline
        g.setColor(new Color(220, 50, 50));
        g.draw(new Rectangle2D.Float(40, Y2, S, S));

        // 6. RoundRectangle Outline
        g.setColor(new Color(50, 200, 120));
        g.draw(new RoundRectangle2D.Float(170, Y2, S, S, 35, 35));

        // 7. Circle Outline
        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(300, Y2, S, S));

        // ═══════════════════════════════════════════════════════════
        // Row 3: Shapes WITHOUT AA (100x100) + Typography Comparison
        // ═══════════════════════════════════════════════════════════
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);

        // 8. Circle Fill (No AA)
        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(40, Y3, S, S));

        // 9. Circle Outline (No AA)
        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(170, Y3, S, S));

        // 10. Typography Comparison (DirectWrite / Subpixel ClearType vs Java2D)
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HRGB);
        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 18));
        g.drawString("FastJava Typography 1:1", 300, Y3 + 30);
        g.setFont(new Font("Segoe UI", Font.PLAIN, 13));
        g.setColor(new Color(180, 180, 180));
        g.drawString("Subpixel Anti-Aliasing (RGB ClearType)", 300, Y3 + 55);
        g.drawString("100% Hardware Accelerated Glyph Pipeline", 300, Y3 + 75);
    }

    private static void renderVulkanScene(FastVulkanGraphics g, BufferedImage testImg) {
        final float S = 100.0f; // Homogeneous 100x100 size for all shapes
        final float Y1 = 40.0f; // Row 1 (AA)
        final float Y2 = 170.0f; // Row 2 (AA Outlines)
        final float Y3 = 300.0f; // Row 3 (Non-AA)

        // ═══════════════════════════════════════════════════════════
        // Row 1: Filled Shapes with 16x Subpixel AA (100x100)
        // ═══════════════════════════════════════════════════════════
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        // 1. Rectangle
        g.setColor(new Color(220, 50, 50));
        g.fill(new Rectangle2D.Float(40, Y1, S, S));

        // 2. RoundRectangle
        g.setColor(new Color(50, 200, 120));
        g.fill(new RoundRectangle2D.Float(170, Y1, S, S, 35, 35));

        // 3. Circle
        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(300, Y1, S, S));

        // 4. Test Image
        g.setColor(Color.WHITE);
        g.drawImage(testImg, 430, Y1);

        // ═══════════════════════════════════════════════════════════
        // Row 2: Outlined Shapes with 16x Subpixel AA (100x100)
        // ═══════════════════════════════════════════════════════════
        g.setStrokeWidth(1.0f);

        // 5. Rectangle Outline
        g.setColor(new Color(220, 50, 50));
        g.draw(new Rectangle2D.Float(40, Y2, S, S));

        // 6. RoundRectangle Outline
        g.setColor(new Color(50, 200, 120));
        g.draw(new RoundRectangle2D.Float(170, Y2, S, S, 35, 35));

        // 7. Circle Outline
        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(300, Y2, S, S));

        // ═══════════════════════════════════════════════════════════
        // Row 3: Shapes WITHOUT AA (100x100) + Typography Comparison
        // ═══════════════════════════════════════════════════════════
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);

        // 8. Circle Fill (No AA)
        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(40, Y3, S, S));

        // 9. Circle Outline (No AA)
        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(170, Y3, S, S));

        // 10. Typography Comparison (Vulkan Native Cached Glyph Pipeline)
        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 18));
        g.drawString("FastJava Typography 1:1", 300, Y3 + 30);
        g.setFont(new Font("Segoe UI", Font.PLAIN, 13));
        g.setColor(new Color(180, 180, 180));
        g.drawString("Subpixel Anti-Aliasing (RGB ClearType)", 300, Y3 + 55);
        g.drawString("100% Hardware Accelerated Glyph Pipeline", 300, Y3 + 75);
    }

    public static void main(String[] args) throws Exception {
        final int winW = 600;
        final int winH = 440;

        BufferedImage testImg = createTestImage(100);

        // Java2D offscreen reference
        BufferedImage j2dImage = new BufferedImage(winW, winH, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = j2dImage.createGraphics();
        renderJava2DScene(g2d, winW, winH, testImg);
        g2d.dispose();

        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "FastVulkan vs Java2D 1:1 Pixel Calibration", winW, winH,
                0.0784f, 0.0784f, 0.0784f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, winW, winH, false);
            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            // Bind FastKeyboard to the window for zero-latency toggle
            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (isPressed && vKey == Keys.SPACE) {
                        showJava2D = !showJava2D;
                    }
                });

                long lastFpsTime = System.nanoTime();
                int frames = 0;

                while (window.pollEvents()) {
                    FastDWM.waitForVSync();

                    int curW = window.getWidth();
                    int curH = window.getHeight();
                    if (curW <= 0) curW = winW;
                    if (curH <= 0) curH = winH;

                    if (showJava2D) {
                        vkg.drawImage(j2dTexture, 0f, 0f, (float)winW, (float)winH);
                    } else {
                        renderVulkanScene(vkg, testImg);
                    }

                    window.present();

                    frames++;
                    long now = System.nanoTime();
                    if (now - lastFpsTime >= 1_000_000_000L) {
                        String mode = showJava2D ? "JAVA2D Reference" : "FASTVULKAN Native (16x Subpixel SDF)";
                        window.setTitle("[" + mode + "] FPS: " + frames + "  [SPACE] flip");
                        frames = 0;
                        lastFpsTime = now;
                    }
                }
            }

            window.destroyTexture(j2dTexture);
        }
    }
}
