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

    public enum CompareEngine {
        VULKAN_FACES("1/3: Vulkan Faces (Native GPU 16x SDF)"),
        VULKAN_MARLIN("2/3: Vulkan Märlin (Offscreen Texture Twin)"),
        JAVA2D("3/3: Java2D Reference (AWT Baseline)");

        public final String label;
        CompareEngine(String label) { this.label = label; }
    }

    private static CompareEngine currentEngine = CompareEngine.VULKAN_FACES;

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

            System.out.println("=================================================================");
            System.out.println(" FastVulkan vs Marlin vs Java2D 1:1 Engine Comparison:");
            System.out.println("   [1/3] Vulkan Faces   : Pure Native GPU 16x Subpixel SDF Shaders");
            System.out.println("   [2/3] Vulkan Märlin  : Offscreen Marlin Rasterization -> GPU Texture Twin");
            System.out.println("   [3/3] Java2D         : Standard Java2D / AWT Baseline");
            System.out.println("   [SPACE]              : Cycle through all three engines");
            System.out.println("=================================================================");

            // Bind FastKeyboard to the window for zero-latency toggle
            final boolean[] dirty = new boolean[]{true};
            final double[] lastCalcMicros = new double[]{0.0};

            Runnable triggerCalculation = () -> {
                long t0 = System.nanoTime();
                switch (currentEngine) {
                    case VULKAN_FACES -> {
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_FACES);
                        renderVulkanScene(vkg, testImg);
                        vkg.flush();
                        lastCalcMicros[0] = (System.nanoTime() - t0) / 1000.0;
                    }
                    case VULKAN_MARLIN -> {
                        // Single-pass: Java2D/Marlin rasterization + Vulkan GPU texture upload
                        Graphics2D offG = j2dImage.createGraphics();
                        renderJava2DScene(offG, winW, winH, testImg);
                        offG.dispose();
                        window.updateTexture(j2dTexture, j2dPixels, winW, winH);
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_MARLIN);
                        vkg.drawImage(j2dTexture, 0f, 0f, (float)winW, (float)winH);
                        vkg.flush();
                        lastCalcMicros[0] = (System.nanoTime() - t0) / 1000.0;
                    }
                    case JAVA2D -> {
                        // Single-pass: Pure Java2D CPU baseline rasterization
                        Graphics2D offG = j2dImage.createGraphics();
                        renderJava2DScene(offG, winW, winH, testImg);
                        offG.dispose();
                        lastCalcMicros[0] = (System.nanoTime() - t0) / 1000.0;
                        vkg.drawImage(j2dTexture, 0f, 0f, (float)winW, (float)winH);
                        vkg.flush();
                    }
                }
                String title = String.format("[%s] Single-Pass Compute: %.2f µs (%.3f ms)  [SPACE] switch",
                        currentEngine.label, lastCalcMicros[0], lastCalcMicros[0] / 1000.0);
                window.setTitle(title);
                System.out.printf("=> %-32s | Single-Pass Time: %8.2f µs (%6.3f ms)%n",
                        currentEngine.label, lastCalcMicros[0], lastCalcMicros[0] / 1000.0);
            };

            // Render function that redraws current scene without timing
            Runnable renderActiveScene = () -> {
                switch (currentEngine) {
                    case VULKAN_FACES -> {
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_FACES);
                        renderVulkanScene(vkg, testImg);
                    }
                    case VULKAN_MARLIN, JAVA2D -> {
                        vkg.drawImage(j2dTexture, 0f, 0f, (float)winW, (float)winH);
                    }
                }
            };

            // Run initial measurement
            triggerCalculation.run();

            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (isPressed && vKey == Keys.SPACE) {
                        currentEngine = CompareEngine.values()[(currentEngine.ordinal() + 1) % CompareEngine.values().length];
                        dirty[0] = true;
                    }
                });

                while (window.pollEvents()) {
                    FastDWM.waitForVSync();

                    if (dirty[0]) {
                        dirty[0] = false;
                        triggerCalculation.run();
                    } else {
                        // Keep the image on the screen by submitting the draw calls for each frame
                        renderActiveScene.run();
                    }

                    window.present();
                }
            }

            window.destroyTexture(j2dTexture);
        }
    }
}
