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
import java.util.Random;

/**
 * DemoInfiniteVectorCanvas — Infinite Microscopic Vector Canvas.
 * <p>
 * Demonstrates real-time vector transformation and procedural graph rendering:
 * - 4,000 hierarchical nodes with dynamic connection lines
 * - Smooth procedural camera zoom / pan across multi-scale coordinate space
 * - Toggle [SPACE] between FastVulkan Faces (SDF hardware) and Java2D
 */
public class DemoInfiniteVectorCanvas {

    private static final int WIN_W = 1280;
    private static final int WIN_H = 720;
    private static final int NODE_COUNT = 3_000;

    private static final float[] worldX = new float[NODE_COUNT];
    private static final float[] worldY = new float[NODE_COUNT];
    private static final float[] nodeRadius = new float[NODE_COUNT];
    private static final Color[] nodeColors = new Color[NODE_COUNT];
    private static final int[] linkTarget = new int[NODE_COUNT];

    private static boolean useVulkan = true;

    static {
        Random rng = new Random(4242);
        for (int i = 0; i < NODE_COUNT; i++) {
            worldX[i] = (rng.nextFloat() - 0.5f) * 6000.0f;
            worldY[i] = (rng.nextFloat() - 0.5f) * 4000.0f;
            nodeRadius[i] = 12.0f + rng.nextFloat() * 24.0f;
            linkTarget[i] = Math.max(0, i - (rng.nextInt(15) + 1));

            int r = 50 + rng.nextInt(180);
            int g = 100 + rng.nextInt(155);
            int b = 200 + rng.nextInt(55);
            nodeColors[i] = new Color(r, g, b, 210);
        }
    }

    private static void renderCanvasJava2D(Graphics2D g, float zoom, float camX, float camY) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(new Color(15, 17, 26));
        g.fillRect(0, 0, WIN_W, WIN_H);

        float halfW = WIN_W * 0.5f;
        float halfH = WIN_H * 0.5f;

        // Connections
        g.setColor(new Color(100, 160, 255, 60));
        for (int i = 0; i < NODE_COUNT; i++) {
            int target = linkTarget[i];
            float sx = (worldX[i] - camX) * zoom + halfW;
            float sy = (worldY[i] - camY) * zoom + halfH;
            float tx = (worldX[target] - camX) * zoom + halfW;
            float ty = (worldY[target] - camY) * zoom + halfH;

            if ((sx > -50 && sx < WIN_W + 50 && sy > -50 && sy < WIN_H + 50) ||
                (tx > -50 && tx < WIN_W + 50 && ty > -50 && ty < WIN_H + 50)) {
                g.drawLine((int) sx, (int) sy, (int) tx, (int) ty);
            }
        }

        // Nodes
        for (int i = 0; i < NODE_COUNT; i++) {
            float sx = (worldX[i] - camX) * zoom + halfW;
            float sy = (worldY[i] - camY) * zoom + halfH;
            float r = nodeRadius[i] * zoom;

            if (sx + r < 0 || sx - r > WIN_W || sy + r < 0 || sy - r > WIN_H) continue;

            g.setColor(nodeColors[i]);
            g.fill(new RoundRectangle2D.Float(sx - r, sy - r, r * 2f, r * 2f, r * 0.5f, r * 0.5f));
            g.setColor(Color.WHITE);
            g.draw(new RoundRectangle2D.Float(sx - r, sy - r, r * 2f, r * 2f, r * 0.5f, r * 0.5f));
        }

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 15));
        g.drawString(String.format("CANVAS ZOOM: %.3fx | ACTIVE NODES: %d", zoom, NODE_COUNT), 40, 40);
    }

    private static void renderCanvasVulkan(FastVulkanGraphics g, float zoom, float camX, float camY) {
        float halfW = WIN_W * 0.5f;
        float halfH = WIN_H * 0.5f;

        // Connections
        g.setColor(new Color(100, 160, 255, 60));
        for (int i = 0; i < NODE_COUNT; i++) {
            int target = linkTarget[i];
            float sx = (worldX[i] - camX) * zoom + halfW;
            float sy = (worldY[i] - camY) * zoom + halfH;
            float tx = (worldX[target] - camX) * zoom + halfW;
            float ty = (worldY[target] - camY) * zoom + halfH;

            if ((sx > -50 && sx < WIN_W + 50 && sy > -50 && sy < WIN_H + 50) ||
                (tx > -50 && tx < WIN_W + 50 && ty > -50 && ty < WIN_H + 50)) {
                g.fillRect(Math.min(sx, tx), Math.min(sy, ty), Math.abs(tx - sx) + 1.5f, Math.abs(ty - sy) + 1.5f);
            }
        }

        // Nodes
        for (int i = 0; i < NODE_COUNT; i++) {
            float sx = (worldX[i] - camX) * zoom + halfW;
            float sy = (worldY[i] - camY) * zoom + halfH;
            float r = nodeRadius[i] * zoom;

            if (sx + r < 0 || sx - r > WIN_W || sy + r < 0 || sy - r > WIN_H) continue;

            g.setColor(nodeColors[i]);
            g.fillRoundRect(sx - r, sy - r, r * 2f, r * 2f, r * 0.5f, r * 0.5f);
            g.setColor(Color.WHITE);
            g.drawRoundRect(sx - r, sy - r, r * 2f, r * 2f, r * 0.5f, r * 0.5f);
        }

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 15));
        g.drawString(String.format("CANVAS ZOOM: %.3fx | ACTIVE NODES: %d", zoom, NODE_COUNT), 40, 40);
    }

    public static void main(String[] args) throws Exception {
        BufferedImage j2dImage = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
        Graphics2D j2dGraphics = j2dImage.createGraphics();
        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "Infinite Vector Canvas — FastVulkan vs Java2D", WIN_W, WIN_H,
                0.058f, 0.066f, 0.10f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 15, 17, 26);
                FastTheme.setTitleBarTextColor(hwnd, 220, 230, 255);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, WIN_W, WIN_H, false);
            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            System.out.println("=================================================================");
            System.out.println(" DEMO 3: Infinite Vector Canvas");
            System.out.println("   [SPACE] : Toggle between FastVulkan Faces (GPU) and Java2D (CPU)");
            System.out.println("=================================================================");

            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (isPressed && vKey == Keys.SPACE) {
                        useVulkan = !useVulkan;
                    }
                });

                long startTime = System.nanoTime();
                long lastFpsTime = System.nanoTime();
                int frames = 0;
                double avgMicros = 0.0;

                while (window.pollEvents()) {
                    float t = (float) ((System.nanoTime() - startTime) / 1_000_000_000.0);
                    // Animated sinusoidal camera pan and zoom
                    float zoom = 0.6f + (float) Math.sin(t * 0.8f) * 0.45f;
                    float camX = (float) Math.sin(t * 0.5f) * 800.0f;
                    float camY = (float) Math.cos(t * 0.4f) * 500.0f;

                    long t0 = System.nanoTime();

                    if (useVulkan) {
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_FACES);
                        renderCanvasVulkan(vkg, zoom, camX, camY);
                        vkg.flush();
                    } else {
                        renderCanvasJava2D(j2dGraphics, zoom, camX, camY);
                        window.updateTexture(j2dTexture, j2dPixels, WIN_W, WIN_H);
                        vkg.drawImage(j2dTexture, 0f, 0f, (float) WIN_W, (float) WIN_H);
                        vkg.flush();
                    }

                    window.present();

                    long elapsed = System.nanoTime() - t0;
                    double micros = elapsed / 1000.0;
                    avgMicros = avgMicros * 0.9 + micros * 0.1;
                    frames++;

                    long now = System.nanoTime();
                    if (now - lastFpsTime >= 500_000_000L) {
                        String mode = useVulkan ? "FastVulkan GPU Faces" : "Java2D CPU Reference";
                        int fps = (int) (frames * 1_000_000_000.0 / (now - lastFpsTime));
                        String title = String.format("[%s] FPS: %d | Graph Compute: %.1f µs (%.2f ms)  [SPACE] toggle",
                                mode, fps, avgMicros, avgMicros / 1000.0);
                        window.setTitle(title);
                        frames = 0;
                        lastFpsTime = now;
                    }
                }
            }

            window.destroyTexture(j2dTexture);
        }
    }
}
