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

/**
 * DemoHUDDashboard — Cyberpunk High-Tech Glass & Vector HUD.
 * <p>
 * Demonstrates complex UI composition:
 * - Dynamic Bézier waveforms & audio visualizer arcs
 * - Multi-layer transparent frosted glass cards (SDF RoundRects)
 * - Concentric animated reticles and target crosshairs
 * - Toggle [SPACE] between Vulkan Faces and Java2D to compare compute time
 */
public class DemoHUDDashboard {

    private static final int WIN_W = 1280;
    private static final int WIN_H = 720;

    private static boolean useVulkan = true;

    private static void renderHUDJava2D(Graphics2D g, float time) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(new Color(10, 14, 20));
        g.fillRect(0, 0, WIN_W, WIN_H);

        // Grid Background
        g.setColor(new Color(255, 255, 255, 12));
        for (int x = 0; x < WIN_W; x += 40) {
            g.drawLine(x, 0, x, WIN_H);
        }
        for (int y = 0; y < WIN_H; y += 40) {
            g.drawLine(0, y, WIN_W, y);
        }

        // Center HUD Radar Circles
        float cx = WIN_W * 0.5f;
        float cy = WIN_H * 0.48f;
        for (int i = 1; i <= 6; i++) {
            float r = i * 45.0f;
            g.setColor(new Color(0, 220, 255, 25 + i * 15));
            g.draw(new Ellipse2D.Float(cx - r, cy - r, r * 2f, r * 2f));
        }

        // Rotating Scanning Sweep Beam
        float angle = time * 2.0f;
        float sweepX = cx + (float) Math.cos(angle) * 270.0f;
        float sweepY = cy + (float) Math.sin(angle) * 270.0f;
        g.setColor(new Color(0, 255, 180, 180));
        g.drawLine((int) cx, (int) cy, (int) sweepX, (int) sweepY);

        // Frosted Glass HUD Cards (SDF RoundRects)
        drawCardJ2D(g, 40, 50, 320, 180, "SYSTEM TELEMETRY", time);
        drawCardJ2D(g, 40, 250, 320, 220, "QUANTUM FLUX MATRIX", time * 1.3f);
        drawCardJ2D(g, WIN_W - 360, 50, 320, 260, "TARGET ACQUISITION", time * 0.8f);
        drawCardJ2D(g, WIN_W - 360, 330, 320, 160, "VULKAN SWAPCHAIN", time * 1.5f);

        // Bottom Spectrum
        float waveY = WIN_H - 120;
        g.setColor(new Color(255, 50, 120, 200));
        for (int i = 0; i < 64; i++) {
            float bx = 80 + i * 17.5f;
            float h = (float) Math.sin(time * 5.0f + i * 0.25f) * 40.0f + 50.0f;
            g.fill(new RoundRectangle2D.Float(bx, waveY - h, 10, h * 2, 4, 4));
        }

        // Top Status Header
        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 16));
        g.drawString("FASTJAVA // CYBERPUNK HUD INTERACTION MATRIX", 50, 35);
    }

    private static void drawCardJ2D(Graphics2D g, float x, float y, float w, float h, String title, float t) {
        g.setColor(new Color(16, 26, 38, 210));
        g.fill(new RoundRectangle2D.Float(x, y, w, h, 16, 16));
        g.setColor(new Color(0, 200, 255, 90));
        g.draw(new RoundRectangle2D.Float(x, y, w, h, 16, 16));

        g.setColor(new Color(0, 255, 200, 220));
        g.fill(new RoundRectangle2D.Float(x + 12, y + 12, 6, 16, 3, 3));

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 12));
        g.drawString(title, x + 26, y + 25);

        g.setColor(new Color(0, 200, 255, 140));
        for (int i = 0; i < 8; i++) {
            float barW = (float) Math.abs(Math.sin(t + i * 0.4f)) * (w - 50);
            g.fill(new RoundRectangle2D.Float(x + 20, y + 45 + i * 14, barW, 6, 3, 3));
        }
    }

    private static void renderHUDVulkan(FastVulkanGraphics g, float time) {
        float cx = WIN_W * 0.5f;
        float cy = WIN_H * 0.48f;
        for (int i = 1; i <= 6; i++) {
            float r = i * 45.0f;
            g.setColor(new Color(0, 220, 255, 25 + i * 15));
            g.drawOval(cx - r, cy - r, r * 2f, r * 2f);
        }

        float angle = time * 2.0f;
        float sweepX = cx + (float) Math.cos(angle) * 270.0f;
        float sweepY = cy + (float) Math.sin(angle) * 270.0f;
        g.setColor(new Color(0, 255, 180, 180));
        g.fillRect(Math.min(cx, sweepX), Math.min(cy, sweepY), Math.abs(sweepX - cx) + 1f, 2f);

        drawCardVulkan(g, 40, 50, 320, 180, "SYSTEM TELEMETRY", time);
        drawCardVulkan(g, 40, 250, 320, 220, "QUANTUM FLUX MATRIX", time * 1.3f);
        drawCardVulkan(g, WIN_W - 360, 50, 320, 260, "TARGET ACQUISITION", time * 0.8f);
        drawCardVulkan(g, WIN_W - 360, 330, 320, 160, "VULKAN SWAPCHAIN", time * 1.5f);

        float waveY = WIN_H - 120;
        g.setColor(new Color(255, 50, 120, 200));
        for (int i = 0; i < 64; i++) {
            float bx = 80 + i * 17.5f;
            float h = (float) Math.sin(time * 5.0f + i * 0.25f) * 40.0f + 50.0f;
            g.fillRoundRect(bx, waveY - h, 10, h * 2, 4, 4);
        }

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 16));
        g.drawString("FASTJAVA // CYBERPUNK HUD INTERACTION MATRIX", 50, 35);
    }

    private static void drawCardVulkan(FastVulkanGraphics g, float x, float y, float w, float h, String title, float t) {
        g.setColor(new Color(16, 26, 38, 210));
        g.fillRoundRect(x, y, w, h, 16, 16);
        g.setColor(new Color(0, 200, 255, 90));
        g.drawRoundRect(x, y, w, h, 16, 16);

        g.setColor(new Color(0, 255, 200, 220));
        g.fillRoundRect(x + 12, y + 12, 6, 16, 3, 3);

        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 12));
        g.drawString(title, x + 26, y + 25);

        g.setColor(new Color(0, 200, 255, 140));
        for (int i = 0; i < 8; i++) {
            float barW = (float) Math.abs(Math.sin(t + i * 0.4f)) * (w - 50);
            g.fillRoundRect(x + 20, y + 45 + i * 14, barW, 6, 3, 3);
        }
    }

    public static void main(String[] args) throws Exception {
        BufferedImage j2dImage = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
        Graphics2D j2dGraphics = j2dImage.createGraphics();
        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "Cyberpunk Glass HUD — FastVulkan vs Java2D", WIN_W, WIN_H,
                0.039f, 0.055f, 0.078f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 10, 14, 20);
                FastTheme.setTitleBarTextColor(hwnd, 0, 220, 255);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, WIN_W, WIN_H, false);
            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            System.out.println("=================================================================");
            System.out.println(" DEMO 1: Cyberpunk Glass & Vector HUD");
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
                    float time = (float) ((System.nanoTime() - startTime) / 1_000_000_000.0);
                    long t0 = System.nanoTime();

                    if (useVulkan) {
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_FACES);
                        renderHUDVulkan(vkg, time);
                        vkg.flush();
                    } else {
                        renderHUDJava2D(j2dGraphics, time);
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
                        String title = String.format("[%s] FPS: %d | Frame Compute: %.1f µs (%.2f ms)  [SPACE] toggle",
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
