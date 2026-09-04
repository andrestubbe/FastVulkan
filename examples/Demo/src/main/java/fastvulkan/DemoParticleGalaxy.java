package fastvulkan;

import fastdwm.FastDWM;
import fastkeyboard.FastKeyboard;
import fastkeyboard.Keys;
import fasttheme.FastTheme;

import java.awt.*;
import java.awt.geom.Ellipse2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.util.Random;

/**
 * DemoParticleGalaxy — 50,000 Particle Vector Singularity & Spiral Galaxy.
 * <p>
 * Demonstrates raw GPU batch throughput:
 * - 50,000 animated particles forming a rotating spiral galaxy
 * - Dynamic gravitational pull and orbital velocity
 * - Toggle [SPACE] between FastVulkan Faces (600-1000+ FPS) and Java2D (~5-12 FPS)
 */
public class DemoParticleGalaxy {

    private static final int WIN_W = 1280;
    private static final int WIN_H = 720;
    private static final int PARTICLE_COUNT = 50_000;

    private static final float[] radius = new float[PARTICLE_COUNT];
    private static final float[] angle = new float[PARTICLE_COUNT];
    private static final float[] speed = new float[PARTICLE_COUNT];
    private static final float[] pSize = new float[PARTICLE_COUNT];
    private static final Color[] pColor = new Color[PARTICLE_COUNT];

    private static boolean useVulkan = true;

    static {
        Random rng = new Random(1337);
        for (int i = 0; i < PARTICLE_COUNT; i++) {
            // Spiral distribution
            float r = (float) Math.pow(rng.nextFloat(), 0.6) * 340.0f + 10.0f;
            float a = rng.nextFloat() * 6.28318f;
            float sp = (0.015f + 0.03f / (r * 0.01f + 1.0f));

            radius[i] = r;
            angle[i] = a;
            speed[i] = sp;
            pSize[i] = 1.5f + rng.nextFloat() * 3.0f;

            // Color gradient from core (golden/cyan) to outer arms (magenta/violet)
            float normR = r / 350.0f;
            int cr = (int) (40 + 215 * normR);
            int cg = (int) (180 - 100 * normR);
            int cb = (int) (255 - 40 * normR);
            pColor[i] = new Color(Math.min(255, cr), Math.max(0, cg), Math.min(255, cb), 200);
        }
    }

    private static void updateGalaxy() {
        for (int i = 0; i < PARTICLE_COUNT; i++) {
            angle[i] += speed[i] * 0.4f;
        }
    }

    private static void renderJava2D(Graphics2D g) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF); // Fast mode for Java2D
        g.setColor(new Color(6, 8, 14));
        g.fillRect(0, 0, WIN_W, WIN_H);

        float cx = WIN_W * 0.5f;
        float cy = WIN_H * 0.5f;

        for (int i = 0; i < PARTICLE_COUNT; i++) {
            float r = radius[i];
            float a = angle[i];
            float spiralA = a + r * 0.015f; // logarithmic arm twist
            float px = cx + (float) Math.cos(spiralA) * r;
            float py = cy + (float) Math.sin(spiralA) * r * 0.55f; // inclined 3D perspective

            g.setColor(pColor[i]);
            float s = pSize[i];
            g.fill(new Ellipse2D.Float(px, py, s, s));
        }

        // Center Core glow
        g.setColor(new Color(255, 240, 200, 240));
        g.fill(new Ellipse2D.Float(cx - 8, cy - 8, 16, 16));
    }

    private static void renderVulkan(FastVulkanGraphics g) {
        float cx = WIN_W * 0.5f;
        float cy = WIN_H * 0.5f;

        for (int i = 0; i < PARTICLE_COUNT; i++) {
            float r = radius[i];
            float a = angle[i];
            float spiralA = a + r * 0.015f;
            float px = cx + (float) Math.cos(spiralA) * r;
            float py = cy + (float) Math.sin(spiralA) * r * 0.55f;

            g.setColor(pColor[i]);
            float s = pSize[i];
            g.fillOval(px, py, s, s);
        }

        // Center Core
        g.setColor(new Color(255, 240, 200, 240));
        g.fillOval(cx - 8, cy - 8, 16, 16);
    }

    public static void main(String[] args) throws Exception {
        BufferedImage j2dImage = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
        Graphics2D j2dGraphics = j2dImage.createGraphics();
        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "50,000 Particle Galaxy Singularity — FastVulkan vs Java2D", WIN_W, WIN_H,
                0.023f, 0.031f, 0.055f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 6, 8, 14);
                FastTheme.setTitleBarTextColor(hwnd, 200, 200, 255);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, WIN_W, WIN_H, false);
            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            System.out.println("=================================================================");
            System.out.println(" DEMO 2: 50,000 Particle Galaxy Singularity");
            System.out.println("   [SPACE] : Toggle between FastVulkan Faces (GPU) and Java2D (CPU)");
            System.out.println("=================================================================");

            try (FastKeyboard keyboard = FastKeyboard.openForWindow(hwnd)) {
                keyboard.startListening((dev, vKey, makeCode, isPressed, isE0, ts, keyChar) -> {
                    if (isPressed && vKey == Keys.SPACE) {
                        useVulkan = !useVulkan;
                    }
                });

                long lastFpsTime = System.nanoTime();
                int frames = 0;
                double avgMicros = 0.0;

                while (window.pollEvents()) {
                    updateGalaxy();
                    long t0 = System.nanoTime();

                    if (useVulkan) {
                        vkg.setEngineMode(FastVulkanGraphics.EngineMode.VULKAN_FACES);
                        renderVulkan(vkg);
                        vkg.flush();
                    } else {
                        renderJava2D(j2dGraphics);
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
                        String mode = useVulkan ? "FastVulkan GPU (50k Batch)" : "Java2D CPU Reference";
                        int fps = (int) (frames * 1_000_000_000.0 / (now - lastFpsTime));
                        String title = String.format("[%s] FPS: %d | 50,000 Particles: %.1f µs (%.2f ms)  [SPACE] toggle",
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
