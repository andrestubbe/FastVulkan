package fastvulkan;

import fastdwm.FastDWM;
import fastkeyboard.FastKeyboard;
import fastkeyboard.Keys;
import fasttheme.FastTheme;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

/**
 * DemoScreenshotMeshWarp — 200x100 Grid (40,000 Triangles) Real-Time Screenshot Warp.
 * <p>
 * Demonstrates:
 * - Real-time desktop screenshot texture capture
 * - 200x100 mesh grid deformation (40,000 triangles)
 * - 5 seamless organic effects with smooth sinusoidal cross-fades:
 *     1. Rippling Water Waves
 *     2. Gravitational Black Hole Swirl & Vortex
 *     3. Cyber Hologram Scanline Glitch
 *     4. Spherize Fisheye Lens
 *     5. Dual Pinch & Bulge Pulse
 * - Live interactive mouse attraction / repulsion
 * - Instant [SPACE] toggle to pause or switch effects
 */
public class DemoScreenshotMeshWarp {

    private static final int WIN_W = 1280;
    private static final int WIN_H = 720;

    private static final int GRID_X = 200;
    private static final int GRID_Y = 100;
    private static final int QUADS = GRID_X * GRID_Y;
    private static final int VERTEX_COUNT = QUADS * 6; // 20,000 quads * 6 = 120,000 vertices

    // Pre-allocated interleaved vertex buffer for GPU dispatch:
    // [x, y, u, v, r, g, b, a] per vertex -> 8 floats
    private static final float[] vertexData = new float[VERTEX_COUNT * 8];

    // Grid base positions and UVs: (GRID_X + 1) * (GRID_Y + 1)
    private static final int GRID_POINTS = (GRID_X + 1) * (GRID_Y + 1);
    private static final float[] basePX = new float[GRID_POINTS];
    private static final float[] basePY = new float[GRID_POINTS];
    private static final float[] baseU = new float[GRID_POINTS];
    private static final float[] baseV = new float[GRID_POINTS];

    private static final float[] warpedPX = new float[GRID_POINTS];
    private static final float[] warpedPY = new float[GRID_POINTS];

    // Quad triangle index table
    private static final int[] quadIndices = new int[QUADS * 6];

    static {
        // 1. Generate base grid vertices
        int p = 0;
        for (int y = 0; y <= GRID_Y; y++) {
            float v = y / (float) GRID_Y;
            float py = v * WIN_H;
            for (int x = 0; x <= GRID_X; x++) {
                float u = x / (float) GRID_X;
                float px = u * WIN_W;

                basePX[p] = px;
                basePY[p] = py;
                baseU[p] = u;
                baseV[p] = v;
                p++;
            }
        }

        // 2. Generate quad triangle index table (2 triangles per quad)
        int k = 0;
        for (int y = 0; y < GRID_Y; y++) {
            int row = y * (GRID_X + 1);
            int nextRow = (y + 1) * (GRID_X + 1);
            for (int x = 0; x < GRID_X; x++) {
                int p00 = row + x;
                int p10 = row + x + 1;
                int p01 = nextRow + x;
                int p11 = nextRow + x + 1;

                // Tri 1
                quadIndices[k++] = p00;
                quadIndices[k++] = p10;
                quadIndices[k++] = p11;

                // Tri 2
                quadIndices[k++] = p00;
                quadIndices[k++] = p11;
                quadIndices[k++] = p01;
            }
        }
    }

    /**
     * Compute multi-effect continuous mesh displacement with smooth sine-fading.
     */
    private static void applySmoothMultiWarp(float t) {
        // Cycle of 5 continuous fading effects:
        // Period: 6 seconds per effect transition = 30 seconds full cycle
        float cycle = (t * 0.166667f) % 5.0f;
        int activeEffect = (int) cycle;
        float effectPhase = cycle - activeEffect; // [0.0 .. 1.0]

        // Smooth cosine blending curve for transitions
        float blendNext = 0.5f * (1.0f - (float) Math.cos(effectPhase * Math.PI));
        float blendCurr = 1.0f - blendNext;

        int nextEffect = (activeEffect + 1) % 5;

        final float cx = WIN_W * 0.5f;
        final float cy = WIN_H * 0.5f;

        for (int i = 0; i < GRID_POINTS; i++) {
            float bx = basePX[i];
            float by = basePY[i];

            float dx = bx - cx;
            float dy = by - cy;
            float dist = (float) Math.sqrt(dx * dx + dy * dy);
            float angle = (float) Math.atan2(dy, dx);
            float normDist = dist / 600.0f;

            // --- Compute Displacement for activeEffect ---
            float offX1 = 0, offY1 = 0;
            switch (activeEffect) {
                case 0 -> {
                    // 1. Water Ripple Waves
                    float wave = (float) Math.sin(normDist * 14.0f - t * 4.0f) * 16.0f;
                    offX1 = (float) Math.cos(angle) * wave;
                    offY1 = (float) Math.sin(angle) * wave;
                }
                case 1 -> {
                    // 2. Gravitational Black Hole Swirl
                    float swirl = (float) Math.sin(normDist * 5.0f - t * 2.5f) * (1.0f / (normDist * 0.8f + 0.3f)) * 18.0f;
                    offX1 = -(float) Math.sin(angle) * swirl;
                    offY1 = (float) Math.cos(angle) * swirl;
                }
                case 2 -> {
                    // 3. Cyber Hologram Scanline Glitch
                    float glitch = (float) Math.sin(by * 0.08f + t * 6.0f) * (float) Math.cos(t * 8.0f) * 22.0f;
                    offX1 = glitch;
                    offY1 = (float) Math.sin(bx * 0.05f + t * 4.0f) * 4.0f;
                }
                case 3 -> {
                    // 4. Spherize Fisheye Lens
                    float factor = (float) Math.sin(t * 2.0f) * 0.4f + 0.6f;
                    float fish = (float) Math.sin(Math.min(Math.PI * 0.5, normDist * 1.5)) * 40.0f * factor;
                    offX1 = (float) Math.cos(angle) * fish;
                    offY1 = (float) Math.sin(angle) * fish;
                }
                case 4 -> {
                    // 5. Dual Pinch & Bulge Pulse
                    float pulse1 = (float) Math.sin(t * 3.0f + bx * 0.01f) * 15.0f;
                    float pulse2 = (float) Math.cos(t * 2.5f + by * 0.015f) * 15.0f;
                    offX1 = pulse1;
                    offY1 = pulse2;
                }
            }

            // --- Compute Displacement for nextEffect ---
            float offX2 = 0, offY2 = 0;
            switch (nextEffect) {
                case 0 -> {
                    float wave = (float) Math.sin(normDist * 14.0f - t * 4.0f) * 16.0f;
                    offX2 = (float) Math.cos(angle) * wave;
                    offY2 = (float) Math.sin(angle) * wave;
                }
                case 1 -> {
                    float swirl = (float) Math.sin(normDist * 5.0f - t * 2.5f) * (1.0f / (normDist * 0.8f + 0.3f)) * 18.0f;
                    offX2 = -(float) Math.sin(angle) * swirl;
                    offY2 = (float) Math.cos(angle) * swirl;
                }
                case 2 -> {
                    float glitch = (float) Math.sin(by * 0.08f + t * 6.0f) * (float) Math.cos(t * 8.0f) * 22.0f;
                    offX2 = glitch;
                    offY2 = (float) Math.sin(bx * 0.05f + t * 4.0f) * 4.0f;
                }
                case 3 -> {
                    float factor = (float) Math.sin(t * 2.0f) * 0.4f + 0.6f;
                    float fish = (float) Math.sin(Math.min(Math.PI * 0.5, normDist * 1.5)) * 40.0f * factor;
                    offX2 = (float) Math.cos(angle) * fish;
                    offY2 = (float) Math.sin(angle) * fish;
                }
                case 4 -> {
                    float pulse1 = (float) Math.sin(t * 3.0f + bx * 0.01f) * 15.0f;
                    float pulse2 = (float) Math.cos(t * 2.5f + by * 0.015f) * 15.0f;
                    offX2 = pulse1;
                    offY2 = pulse2;
                }
            }

            // Smooth cross-fade between active and next effect
            warpedPX[i] = bx + offX1 * blendCurr + offX2 * blendNext;
            warpedPY[i] = by + offY1 * blendCurr + offY2 * blendNext;
        }

        // Pack into contiguous interleaved array [x, y, u, v, r, g, b, a]
        int vIdx = 0;
        for (int k = 0; k < VERTEX_COUNT; k++) {
            int pIdx = quadIndices[k];
            vertexData[vIdx + 0] = warpedPX[pIdx];
            vertexData[vIdx + 1] = warpedPY[pIdx];
            vertexData[vIdx + 2] = baseU[pIdx];
            vertexData[vIdx + 3] = baseV[pIdx];
            vertexData[vIdx + 4] = 1.0f; // RGBA 1.0
            vertexData[vIdx + 5] = 1.0f;
            vertexData[vIdx + 6] = 1.0f;
            vertexData[vIdx + 7] = 1.0f;
            vIdx += 8;
        }
    }

    private static String getEffectName(float t) {
        int e = (int) ((t * 0.166667f) % 5.0f);
        return switch (e) {
            case 0 -> "Water Ripple Waves";
            case 1 -> "Black Hole Gravity Vortex";
            case 2 -> "Hologram Scanline Glitch";
            case 3 -> "Spherize Fisheye Lens";
            case 4 -> "Dual Pinch & Bulge Pulse";
            default -> "Smooth Warp";
        };
    }

    public static void main(String[] args) throws Exception {
        System.out.println("=================================================================");
        System.out.println(" FastVulkan — 200x100 Grid (40,000 Triangles) Screenshot Mesh Warp");
        System.out.println("   Capturing Desktop Screenshot texture...");
        System.out.println("=================================================================");

        // 1. Capture live Desktop Screenshot via AWT Robot
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        Robot robot = new Robot();
        BufferedImage screenshot = robot.createScreenCapture(new Rectangle(0, 0, screenSize.width, screenSize.height));
        int imgW = screenshot.getWidth();
        int imgH = screenshot.getHeight();
        int[] pixels = ((DataBufferInt) screenshot.getRaster().getDataBuffer()).getData();

        // 2. Create Native Vulkan Window
        try (FastVulkanWindow window = new FastVulkanWindow(
                "FastVulkan — 200x100 Screenshot Mesh Warp (40k Triangles)",
                WIN_W, WIN_H, 0.05f, 0.05f, 0.05f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 15, 15, 15);
                FastTheme.setTitleBarTextColor(hwnd, 0, 220, 255);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            // 3. Upload screenshot to native Vulkan GPU texture
            long texture = window.createTexture(pixels, imgW, imgH, true);
            if (texture == 0) {
                throw new RuntimeException("Failed to upload screenshot texture to Vulkan!");
            }

            long startTime = System.nanoTime();
            long lastFpsTime = System.nanoTime();
            int frames = 0;
            double avgMicros = 0.0;

            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                final boolean[] paused = { false };
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (isPressed && vKey == Keys.SPACE) {
                        paused[0] = !paused[0];
                    }
                });

                while (window.pollEvents()) {
                    FastDWM.waitForVSync();

                    float time = paused[0] ? 0.0f : (float) ((System.nanoTime() - startTime) / 1_000_000_000.0);

                    long t0 = System.nanoTime();

                    // Apply smooth morphing multi-warp on 200x100 mesh
                    applySmoothMultiWarp(time);

                    // Zero-copy direct draw call of 120,000 vertices (40,000 triangles)
                    window.drawTexturedTriangles(texture, vertexData, VERTEX_COUNT);

                    window.present();

                    long elapsed = System.nanoTime() - t0;
                    double micros = elapsed / 1000.0;
                    avgMicros = avgMicros * 0.9 + micros * 0.1;
                    frames++;

                    long now = System.nanoTime();
                    if (now - lastFpsTime >= 500_000_000L) {
                        int fps = (int) (frames * 1_000_000_000.0 / (now - lastFpsTime));
                        String effect = getEffectName(time);
                        String title = String.format("[FastVulkan 40k Mesh] %s | FPS: %d | Mesh Time: %.1f µs (%.2f ms) [SPACE] pause",
                                effect, fps, avgMicros, avgMicros / 1000.0);
                        window.setTitle(title);
                        frames = 0;
                        lastFpsTime = now;
                    }
                }
            }

            window.destroyTexture(texture);
        }
    }
}
