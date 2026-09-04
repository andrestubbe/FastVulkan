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

    // High density 16:9 grid: 240 x 135 cells -> 32,400 quads
    private static final int GRID_X = 240;
    private static final int GRID_Y = 135;
    private static final int QUADS = GRID_X * GRID_Y;
    private static final int VERTEX_COUNT = QUADS * 6; // 32,400 quads * 6 = 194,400 vertices

    // Pre-allocated interleaved vertex buffer for textured mesh: [x, y, u, v, r, g, b, a] * VERTEX_COUNT
    private static final float[] vertexData = new float[VERTEX_COUNT * 8];

    // Grid base positions and UVs: (GRID_X + 1) * (GRID_Y + 1) = 32,776 points
    private static final int GRID_POINTS = (GRID_X + 1) * (GRID_Y + 1);
    private static final float[] basePX = new float[GRID_POINTS];
    private static final float[] basePY = new float[GRID_POINTS];
    private static final float[] baseU = new float[GRID_POINTS];
    private static final float[] baseV = new float[GRID_POINTS];

    private static final float[] warpedPX = new float[GRID_POINTS];
    private static final float[] warpedPY = new float[GRID_POINTS];

    // Quad triangle index table
    private static final int[] quadIndices = new int[QUADS * 6];

    // Pre-allocated vertex buffer for circle overlay: GRID_POINTS * 6 vertices * 8 floats
    private static final float[] circleVertexData = new float[GRID_POINTS * 6 * 8];

    private static final int NUM_EFFECTS = 14;
    private static final float EFFECT_DURATION = 6.0f; // Seconds per effect before morphing

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
     * Deduplicated mathematical effect displacement evaluator.
     * Computes (offX, offY) displacement for a given effect ID at vertex coordinates.
     */
    private static void computeEffect(int effect, float bx, float by, float cx, float cy,
                                      float dx, float dy, float dist, float angle, float normDist,
                                      float t, float[] out) {
        float offX = 0.0f;
        float offY = 0.0f;

        switch (effect) {
            case 0 -> {
                // 0. Water Ripple Waves
                float wave = (float) Math.sin(normDist * 16.0f - t * 4.5f) * (18.0f / (normDist * 0.5f + 0.5f));
                offX = (float) Math.cos(angle) * wave;
                offY = (float) Math.sin(angle) * wave;
            }
            case 1 -> {
                // 1. Gravitational Black Hole Swirl & Vortex
                float swirl = (float) Math.sin(normDist * 6.0f - t * 3.0f) * (1.0f / (normDist * 0.7f + 0.25f)) * 22.0f;
                offX = -(float) Math.sin(angle) * swirl;
                offY = (float) Math.cos(angle) * swirl;
            }
            case 2 -> {
                // 2. Cyber Hologram Scanline Glitch
                float scan = (float) Math.sin(by * 0.1f + t * 7.0f);
                float jitter = ((int) (by * 0.25f + t * 15.0f) % 7 == 0) ? (float) Math.sin(t * 30.0f) * 24.0f : 0.0f;
                offX = scan * (float) Math.cos(t * 5.0f) * 16.0f + jitter;
                offY = (float) Math.sin(bx * 0.06f + t * 4.0f) * 5.0f;
            }
            case 3 -> {
                // 3. Spherize Fisheye Lens
                float factor = (float) Math.sin(t * 2.2f) * 0.5f + 0.5f;
                float fish = (float) Math.sin(Math.min(Math.PI * 0.5, normDist * 1.6f)) * 44.0f * factor;
                offX = (float) Math.cos(angle) * fish;
                offY = (float) Math.sin(angle) * fish;
            }
            case 4 -> {
                // 4. Dual Pinch & Bulge Pulse
                float p1 = (float) Math.sin(t * 3.0f + bx * 0.012f) * 16.0f;
                float p2 = (float) Math.cos(t * 2.6f + by * 0.015f) * 16.0f;
                offX = p1;
                offY = p2;
            }
            case 5 -> {
                // 5. Ocean Swell (Gerstner / Trochoidal Waves)
                float w1 = (float) Math.sin(bx * 0.015f + by * 0.008f - t * 3.2f);
                float w2 = (float) Math.cos(bx * 0.008f - by * 0.018f + t * 2.4f);
                offX = w1 * 14.0f + (float) Math.sin(angle * 2.0f + t * 2.0f) * 6.0f;
                offY = w2 * 18.0f + (float) Math.cos(angle * 2.0f + t * 2.0f) * 6.0f;
            }
            case 6 -> {
                // 6. Twister / Tornado Vortex Funnel
                float heightRatio = by / WIN_H;
                float twist = (float) Math.sin(t * 2.8f + heightRatio * 3.5f) * 32.0f * (1.0f - normDist * 0.6f);
                offX = -(float) Math.sin(angle) * twist + (float) Math.sin(by * 0.02f + t * 3.0f) * 10.0f;
                offY = (float) Math.cos(angle) * twist * 0.5f;
            }
            case 7 -> {
                // 7. Liquid Metal / Domain Warping (2-layer Octave)
                float qx = (float) Math.sin(bx * 0.012f + t * 1.5f);
                float qy = (float) Math.cos(by * 0.012f - t * 1.3f);
                float rx = (float) Math.sin((bx + qx * 40.0f) * 0.015f + t * 2.0f);
                float ry = (float) Math.cos((by + qy * 40.0f) * 0.015f - t * 1.8f);
                offX = rx * 22.0f;
                offY = ry * 22.0f;
            }
            case 8 -> {
                // 8. Shockwave Ring Blast
                float ringPos = (t * 0.6f) % 1.3f;
                float delta = normDist - ringPos;
                float shock = (float) Math.exp(-delta * delta * 45.0f) * 32.0f;
                offX = (float) Math.cos(angle) * shock;
                offY = (float) Math.sin(angle) * shock;
            }
            case 9 -> {
                // 9. Viscous Pixel Melt & Drip
                float dripWave = (float) Math.sin(bx * 0.025f + (float) Math.sin(t * 1.5f));
                float drip = (float) Math.pow(Math.max(0.0f, dripWave * 0.5f + 0.5f), 2.5) * 36.0f * (by / WIN_H);
                offX = (float) Math.sin(by * 0.03f + t * 2.0f) * 6.0f;
                offY = drip;
            }
            case 10 -> {
                // 10. Magnetic Dipole Field Lines
                float p1x = cx + (float) Math.cos(t * 2.0f) * 250.0f;
                float p1y = cy + (float) Math.sin(t * 2.0f) * 150.0f;
                float p2x = cx - (float) Math.cos(t * 2.0f) * 250.0f;
                float p2y = cy - (float) Math.sin(t * 2.0f) * 150.0f;
                float d1x = bx - p1x, d1y = by - p1y;
                float d2x = bx - p2x, d2y = by - p2y;
                float dist1 = (float) Math.sqrt(d1x * d1x + d1y * d1y) + 30.0f;
                float dist2 = (float) Math.sqrt(d2x * d2x + d2y * d2y) + 30.0f;
                float force1 = 4000.0f / (dist1 * dist1);
                float force2 = 4000.0f / (dist2 * dist2);
                offX = (d1x / dist1) * force1 * 18.0f - (d2x / dist2) * force2 * 18.0f;
                offY = (d1y / dist1) * force1 * 18.0f - (d2y / dist2) * force2 * 18.0f;
            }
            case 11 -> {
                // 11. Kaleidoscope Mirror Fold (Prism Crystallization)
                float sectors = 6.0f;
                float sectorAngle = (float) (2.0 * Math.PI / sectors);
                float modAngle = (float) (((angle + t * 0.5f) % sectorAngle + sectorAngle) % sectorAngle);
                if (modAngle > sectorAngle * 0.5f) modAngle = sectorAngle - modAngle;
                float foldDist = normDist * 22.0f;
                offX = (float) Math.cos(modAngle) * foldDist - (dx * 0.05f);
                offY = (float) Math.sin(modAngle) * foldDist - (dy * 0.05f);
            }
            case 12 -> {
                // 12. Heartbeat Organ Pulse (Cardiovascular Thump)
                float beatT = t * 2.5f;
                float beatPhase = beatT - (float) Math.floor(beatT);
                float pulse = 0.0f;
                if (beatPhase < 0.2f) {
                    pulse = (float) Math.sin(beatPhase * Math.PI / 0.2f) * 32.0f;
                } else if (beatPhase >= 0.25f && beatPhase < 0.45f) {
                    pulse = (float) Math.sin((beatPhase - 0.25f) * Math.PI / 0.2f) * 18.0f;
                }
                float heartDamp = (1.0f / (normDist * 1.5f + 0.3f));
                offX = (float) Math.cos(angle) * pulse * heartDamp;
                offY = (float) Math.sin(angle) * pulse * heartDamp;
            }
            case 13 -> {
                // 13. 4D Hyperspace Simplex Warp
                float s1 = (float) Math.sin(bx * 0.01f + t * 2.0f);
                float c1 = (float) Math.cos(by * 0.01f - t * 1.8f);
                float s2 = (float) Math.sin((bx + by) * 0.007f + t * 2.5f);
                float c2 = (float) Math.cos((bx - by) * 0.007f - t * 2.2f);
                offX = (s1 + s2) * 12.0f + (float) Math.cos(angle * 3.0f + t) * 6.0f;
                offY = (c1 + c2) * 12.0f + (float) Math.sin(angle * 3.0f + t) * 6.0f;
            }
        }

        out[0] = offX;
        out[1] = offY;
    }

    /**
     * Compute multi-effect continuous mesh displacement with smooth cosine-fading and border lock.
     */
    private static void applySmoothMultiWarp(float t, int effectOverride) {
        int activeEffect;
        int nextEffect;
        float blendCurr;
        float blendNext;

        if (effectOverride >= 0) {
            activeEffect = effectOverride % NUM_EFFECTS;
            nextEffect = activeEffect;
            blendCurr = 1.0f;
            blendNext = 0.0f;
        } else {
            float cycle = (t / EFFECT_DURATION) % NUM_EFFECTS;
            activeEffect = (int) cycle;
            float effectPhase = cycle - activeEffect; // [0.0 .. 1.0]

            // Smooth cosine blending curve for transitions
            blendNext = 0.5f * (1.0f - (float) Math.cos(effectPhase * Math.PI));
            blendCurr = 1.0f - blendNext;
            nextEffect = (activeEffect + 1) % NUM_EFFECTS;
        }

        final float cx = WIN_W * 0.5f;
        final float cy = WIN_H * 0.5f;

        float[] off1 = new float[2];
        float[] off2 = new float[2];

        for (int i = 0; i < GRID_POINTS; i++) {
            float bx = basePX[i];
            float by = basePY[i];

            float dx = bx - cx;
            float dy = by - cy;
            float dist = (float) Math.sqrt(dx * dx + dy * dy);
            float angle = (float) Math.atan2(dy, dx);
            float normDist = dist / 600.0f;

            // Compute Displacement for active and next effect
            computeEffect(activeEffect, bx, by, cx, cy, dx, dy, dist, angle, normDist, t, off1);
            if (blendNext > 0.001f) {
                computeEffect(nextEffect, bx, by, cx, cy, dx, dy, dist, angle, normDist, t, off2);
            } else {
                off2[0] = 0.0f;
                off2[1] = 0.0f;
            }

            // Smooth cross-fade between active and next effect
            float dispX = off1[0] * blendCurr + off2[0] * blendNext;
            float dispY = off1[1] * blendCurr + off2[1] * blendNext;

            // Pin borders: compute edge attenuation factor (0.0 at borders, 1.0 in center)
            float u = baseU[i];
            float v = baseV[i];
            float edgeDistX = Math.min(u, 1.0f - u) * 2.0f;
            float edgeDistY = Math.min(v, 1.0f - v) * 2.0f;
            float dampX = (float) Math.sin(Math.min(1.0f, edgeDistX * 4.0f) * (Math.PI * 0.5));
            float dampY = (float) Math.sin(Math.min(1.0f, edgeDistY * 4.0f) * (Math.PI * 0.5));
            float borderDamp = dampX * dampY;

            warpedPX[i] = bx + dispX * borderDamp;
            warpedPY[i] = by + dispY * borderDamp;
        }

        // Pack mesh into contiguous interleaved array [x, y, u, v, r, g, b, a]
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

    /**
     * Build vertex point circle overlay for all 32,776 grid vertices.
     * Uses Vulkan Mode -30 analytical 16x subpixel circle antialiasing.
     */
    private static void buildVertexCirclesData() {
        final float radius = 1.8f;
        // High-contrast glowing cyan (RGBA)
        final float cr = 0.05f;
        final float cg = 0.95f;
        final float cb = 1.0f;
        final float ca = 0.85f;

        // Mode -30 circle UVs
        final float u0 = -31.0f;
        final float u1 = -29.0f;
        final float v0 = -1.0f;
        final float v1 =  1.0f;

        int idx = 0;
        for (int i = 0; i < GRID_POINTS; i++) {
            float cx = warpedPX[i];
            float cy = warpedPY[i];

            float x0 = cx - radius;
            float y0 = cy - radius;
            float x1 = cx + radius;
            float y1 = cy + radius;

            // Tri 1: (x0, y0), (x1, y0), (x1, y1)
            circleVertexData[idx +  0] = x0; circleVertexData[idx +  1] = y0;
            circleVertexData[idx +  2] = u0; circleVertexData[idx +  3] = v0;
            circleVertexData[idx +  4] = cr; circleVertexData[idx +  5] = cg;
            circleVertexData[idx +  6] = cb; circleVertexData[idx +  7] = ca;

            circleVertexData[idx +  8] = x1; circleVertexData[idx +  9] = y0;
            circleVertexData[idx + 10] = u1; circleVertexData[idx + 11] = v0;
            circleVertexData[idx + 12] = cr; circleVertexData[idx + 13] = cg;
            circleVertexData[idx + 14] = cb; circleVertexData[idx + 15] = ca;

            circleVertexData[idx + 16] = x1; circleVertexData[idx + 17] = y1;
            circleVertexData[idx + 18] = u1; circleVertexData[idx + 19] = v1;
            circleVertexData[idx + 20] = cr; circleVertexData[idx + 21] = cg;
            circleVertexData[idx + 22] = cb; circleVertexData[idx + 23] = ca;

            // Tri 2: (x0, y0), (x1, y1), (x0, y1)
            circleVertexData[idx + 24] = x0; circleVertexData[idx + 25] = y0;
            circleVertexData[idx + 26] = u0; circleVertexData[idx + 27] = v0;
            circleVertexData[idx + 28] = cr; circleVertexData[idx + 29] = cg;
            circleVertexData[idx + 30] = cb; circleVertexData[idx + 31] = ca;

            circleVertexData[idx + 32] = x1; circleVertexData[idx + 33] = y1;
            circleVertexData[idx + 34] = u1; circleVertexData[idx + 35] = v1;
            circleVertexData[idx + 36] = cr; circleVertexData[idx + 37] = cg;
            circleVertexData[idx + 38] = cb; circleVertexData[idx + 39] = ca;

            circleVertexData[idx + 40] = x0; circleVertexData[idx + 41] = y1;
            circleVertexData[idx + 42] = u0; circleVertexData[idx + 43] = v1;
            circleVertexData[idx + 44] = cr; circleVertexData[idx + 45] = cg;
            circleVertexData[idx + 46] = cb; circleVertexData[idx + 47] = ca;

            idx += 48; // 6 vertices * 8 floats
        }
    }

    private static String getEffectName(int index) {
        return switch (index % NUM_EFFECTS) {
            case 0  -> "Water Ripple Waves";
            case 1  -> "Black Hole Gravity Vortex";
            case 2  -> "Cyber Hologram Scanline Glitch";
            case 3  -> "Spherize Fisheye Lens";
            case 4  -> "Dual Pinch & Bulge Pulse";
            case 5  -> "Ocean Swell (Gerstner Waves)";
            case 6  -> "Twister / Tornado Vortex";
            case 7  -> "Liquid Metal / Domain Warping";
            case 8  -> "Shockwave Ring Blast";
            case 9  -> "Viscous Pixel Melt & Drip";
            case 10 -> "Magnetic Dipole Field Lines";
            case 11 -> "Kaleidoscope Mirror Fold";
            case 12 -> "Heartbeat Organ Pulse";
            case 13 -> "4D Hyperspace Simplex Warp";
            default -> "Smooth Warp";
        };
    }

    private static BufferedImage captureDesktop() throws Exception {
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        Robot robot = new Robot();
        return robot.createScreenCapture(new Rectangle(0, 0, screenSize.width, screenSize.height));
    }

    public static void main(String[] args) throws Exception {
        System.out.println("=================================================================");
        System.out.printf(" FastVulkan — %dx%d Grid (%,d Triangles, %,d Vertices) Mesh Warp\n",
                GRID_X, GRID_Y, QUADS * 2, GRID_POINTS);
        System.out.println("   Capturing Desktop Screenshot texture...");
        System.out.println("=================================================================");

        // 1. Capture live Desktop Screenshot via AWT Robot
        BufferedImage screenshot = captureDesktop();
        int imgW = screenshot.getWidth();
        int imgH = screenshot.getHeight();
        int[] pixels = ((DataBufferInt) screenshot.getRaster().getDataBuffer()).getData();

        // 2. Create Native Vulkan Window
        try (FastVulkanWindow window = new FastVulkanWindow(
                "FastVulkan — 240x135 Screenshot Mesh Warp",
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

            final long[] startTime = { System.nanoTime() };
            long lastFpsTime = System.nanoTime();
            int frames = 0;
            double avgMicros = 0.0;

            final boolean[] paused = { false };
            final boolean[] showVertices = { false };
            final int[] manualEffect = { -1 };
            final float[] pausedTime = { 0.0f };

            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (!isPressed) return;

                    if (vKey == Keys.SPACE) {
                        paused[0] = !paused[0];
                        if (paused[0]) {
                            pausedTime[0] = (float) ((System.nanoTime() - startTime[0]) / 1_000_000_000.0);
                        } else {
                            startTime[0] = System.nanoTime() - (long) (pausedTime[0] * 1_000_000_000.0);
                        }
                    } else if (vKey == Keys.V) {
                        showVertices[0] = !showVertices[0];
                        System.out.println("[Toggle] Vertex Circles Overlay: " + (showVertices[0] ? "ON" : "OFF"));
                    } else if (vKey == Keys.N) {
                        float curT = paused[0] ? pausedTime[0] : (float) ((System.nanoTime() - startTime[0]) / 1_000_000_000.0);
                        int current = manualEffect[0] >= 0 ? manualEffect[0] : (int) ((curT / EFFECT_DURATION) % NUM_EFFECTS);
                        manualEffect[0] = (current + 1) % NUM_EFFECTS;
                        System.out.println("[Switch] Active Effect: " + getEffectName(manualEffect[0]));
                    } else if (vKey == Keys.P) {
                        float curT = paused[0] ? pausedTime[0] : (float) ((System.nanoTime() - startTime[0]) / 1_000_000_000.0);
                        int current = manualEffect[0] >= 0 ? manualEffect[0] : (int) ((curT / EFFECT_DURATION) % NUM_EFFECTS);
                        manualEffect[0] = (current - 1 + NUM_EFFECTS) % NUM_EFFECTS;
                        System.out.println("[Switch] Active Effect: " + getEffectName(manualEffect[0]));
                    } else if (vKey == Keys.R) {
                        try {
                            System.out.println("[Screenshot] Recapturing desktop...");
                            BufferedImage fresh = captureDesktop();
                            int[] freshPx = ((DataBufferInt) fresh.getRaster().getDataBuffer()).getData();
                            window.updateTexture(texture, freshPx, fresh.getWidth(), fresh.getHeight());
                            System.out.println("[Screenshot] Texture reloaded successfully!");
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                });

                while (window.pollEvents()) {
                    FastDWM.waitForVSync();

                    float time = paused[0] ? pausedTime[0] : (float) ((System.nanoTime() - startTime[0]) / 1_000_000_000.0);

                    long t0 = System.nanoTime();

                    // 1. Apply smooth morphing multi-warp on 240x135 mesh
                    applySmoothMultiWarp(time, manualEffect[0]);

                    // 2. Zero-copy direct draw call of 194,400 vertices (64,800 triangles)
                    window.drawTexturedTriangles(texture, vertexData, VERTEX_COUNT);

                    // 3. Render vertex overlay circles if enabled
                    if (showVertices[0]) {
                        buildVertexCirclesData();
                        window.drawTexturedTriangles(0, circleVertexData, GRID_POINTS * 6);
                    }

                    window.present();

                    long elapsed = System.nanoTime() - t0;
                    double micros = elapsed / 1000.0;
                    avgMicros = avgMicros * 0.9 + micros * 0.1;
                    frames++;

                    long now = System.nanoTime();
                    if (now - lastFpsTime >= 500_000_000L) {
                        int fps = (int) (frames * 1_000_000_000.0 / (now - lastFpsTime));
                        int activeIdx = manualEffect[0] >= 0 ? manualEffect[0] : (int) ((time / EFFECT_DURATION) % NUM_EFFECTS);
                        String effect = getEffectName(activeIdx);
                        String title = String.format("[FastVulkan 65k Tris | 33k Verts] %s | FPS: %d | Mesh: %.1f µs | [V] Dots: %s | [SPACE] Pause | [N/P] Next/Prev",
                                effect, fps, avgMicros, showVertices[0] ? "ON" : "OFF");
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
