package fastvulkan;

import fasttheme.FastTheme;

import javax.swing.*;
import java.awt.*;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.util.Random;

public class DemoStressBenchmark {

    private static final int WIN_W = 1280;
    private static final int WIN_H = 720;
    private static int shapeCount = 5_000;
    private static boolean useVulkan = true;

    // Pre-generated random shape properties for 100% fair comparison
    private static final int MAX_SHAPES = 100_000;
    private static final float[] posX = new float[MAX_SHAPES];
    private static final float[] posY = new float[MAX_SHAPES];
    private static final float[] velX = new float[MAX_SHAPES];
    private static final float[] velY = new float[MAX_SHAPES];
    private static final float[] sizes = new float[MAX_SHAPES];
    private static final int[] types = new int[MAX_SHAPES]; // 0=Rect, 1=RoundRect, 2=Circle
    private static final Color[] colors = new Color[MAX_SHAPES];

    static {
        Random rng = new Random(42);
        for (int i = 0; i < MAX_SHAPES; i++) {
            posX[i] = rng.nextFloat() * (WIN_W - 80);
            posY[i] = rng.nextFloat() * (WIN_H - 80);
            velX[i] = (rng.nextFloat() * 4.0f - 2.0f);
            velY[i] = (rng.nextFloat() * 4.0f - 2.0f);
            sizes[i] = 20.0f + rng.nextFloat() * 40.0f;
            types[i] = rng.nextInt(3);
            colors[i] = new Color(rng.nextInt(200) + 55, rng.nextInt(200) + 55, rng.nextInt(200) + 55, 180);
        }
    }

    private static void updatePhysics(int count) {
        for (int i = 0; i < count; i++) {
            posX[i] += velX[i];
            posY[i] += velY[i];
            if (posX[i] < 0 || posX[i] + sizes[i] > WIN_W) velX[i] = -velX[i];
            if (posY[i] < 0 || posY[i] + sizes[i] > WIN_H) velY[i] = -velY[i];
        }
    }

    private static void renderJava2D(Graphics2D g, int count) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(new Color(18, 18, 18));
        g.fillRect(0, 0, WIN_W, WIN_H);

        for (int i = 0; i < count; i++) {
            g.setColor(colors[i]);
            float s = sizes[i];
            switch (types[i]) {
                case 0 -> g.fill(new Rectangle2D.Float(posX[i], posY[i], s, s));
                case 1 -> g.fill(new RoundRectangle2D.Float(posX[i], posY[i], s, s, s * 0.35f, s * 0.35f));
                case 2 -> g.fill(new Ellipse2D.Float(posX[i], posY[i], s, s));
            }
        }
    }

    private static void renderVulkan(FastVulkanGraphics g, int count) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        for (int i = 0; i < count; i++) {
            g.setColor(colors[i]);
            float s = sizes[i];
            switch (types[i]) {
                case 0 -> g.fill(new Rectangle2D.Float(posX[i], posY[i], s, s));
                case 1 -> g.fill(new RoundRectangle2D.Float(posX[i], posY[i], s, s, s * 0.35f, s * 0.35f));
                case 2 -> g.fill(new Ellipse2D.Float(posX[i], posY[i], s, s));
            }
        }
    }

    public static void main(String[] args) throws Exception {
        BufferedImage j2dImage = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
        Graphics2D j2dGraphics = j2dImage.createGraphics();
        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "FastVulkan vs Java2D Stress Benchmark", WIN_W, WIN_H,
                0.07f, 0.07f, 0.07f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, WIN_W, WIN_H, false);
            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            long lastFpsTime = System.nanoTime();
            int frames = 0;

            System.out.println("=================================================================");
            System.out.println(" FastVulkan vs Java2D Stress Benchmark Controls:");
            System.out.println("   [SPACE]     : Toggle between FastVulkan (GPU) and Java2D (CPU)");
            System.out.println("   [UP / DOWN] : Increase / Decrease shape count (+/- 2,500)");
            System.out.println("=================================================================");

            try (fastkeyboard.FastKeyboard keyboard = fastkeyboard.FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    System.out.println("[KEY EVENT] vKey=" + vKey + " isPressed=" + isPressed + " hwnd=" + hwnd);
                    if (isPressed) {
                        if (vKey == fastkeyboard.Keys.SPACE) {
                            useVulkan = !useVulkan;
                            System.out.println("Switched engine to: " + (useVulkan ? "FastVulkan" : "Java2D"));
                        } else if (vKey == fastkeyboard.Keys.UP) {
                            shapeCount = Math.min(MAX_SHAPES, shapeCount + 2500);
                            System.out.println("Shape Count: " + shapeCount);
                        } else if (vKey == fastkeyboard.Keys.DOWN) {
                            shapeCount = Math.max(500, shapeCount - 2500);
                            System.out.println("Shape Count: " + shapeCount);
                        }
                    }
                });

                while (window.pollEvents()) {
                    updatePhysics(shapeCount);

                    if (useVulkan) {
                        renderVulkan(vkg, shapeCount);
                    } else {
                        renderJava2D(j2dGraphics, shapeCount);
                        // Fast GPU Texture Streaming: update pixels directly without reallocating VkImage or stalling
                        vkg.updateTexture(j2dTexture, j2dPixels, WIN_W, WIN_H);
                        vkg.drawImage(j2dTexture, 0f, 0f, (float)WIN_W, (float)WIN_H);
                    }

                    window.present();

                    frames++;
                    long now = System.nanoTime();
                    if (now - lastFpsTime >= 1_000_000_000L) {
                        String mode = useVulkan ? "FASTVULKAN (16x Subpixel GPU)" : "JAVA2D (CPU Marlin)";
                        window.setTitle(String.format("[%s] Shapes: %,d | FPS: %d | [SPACE] Switch | [UP/DOWN] Count",
                                mode, shapeCount, frames));
                        frames = 0;
                        lastFpsTime = now;
                    }
                }
            }

            j2dGraphics.dispose();
            window.destroyTexture(j2dTexture);
        }
    }
}
