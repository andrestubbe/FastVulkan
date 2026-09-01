package fastvulkan;

import fastdwm.FastDWM;
import fasttheme.FastTheme;

import java.awt.*;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class DemoGraphics2DCompare {

    private static boolean showJava2D = true;

    private static void renderJava2DScene(Graphics2D g, int w, int h) {
        g.setColor(new Color(20, 20, 20));
        g.fillRect(0, 0, w, h);
        g.setColor(new Color(220, 50, 50));
        g.fill(new Rectangle2D.Float(200, 150, 200, 200));
    }

    private static void renderVulkanScene(FastVulkanGraphics g) {
        g.setColor(new Color(220, 50, 50));
        g.fill(new Rectangle2D.Float(200, 150, 200, 200));
    }

    public static void main(String[] args) throws Exception {
        final int winW = 800;
        final int winH = 500;

        // Java2D offscreen
        BufferedImage j2dImage = new BufferedImage(winW, winH, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = j2dImage.createGraphics();
        renderJava2DScene(g2d, winW, winH);
        g2d.dispose();

        // Verify Java2D rendered something
        int centerPixel = j2dImage.getRGB(300, 250); // inside red square
        int bgPixel     = j2dImage.getRGB(10,  10);  // background
        System.out.printf("Java2D center pixel: 0x%08X (expect red ~0xFFDC3232)%n", centerPixel);
        System.out.printf("Java2D bg pixel:     0x%08X (expect dark ~0xFF141414)%n", bgPixel);

        int[] j2dPixels = ((DataBufferInt) j2dImage.getRaster().getDataBuffer()).getData();

        try (FastVulkanWindow window = new FastVulkanWindow(
                "FastVulkan vs Java2D — [SPACE] to flip", winW, winH,
                0.0784f, 0.0784f, 0.0784f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            long j2dTexture = window.createTexture(j2dPixels, winW, winH, false);
            System.out.println("Texture handle: " + j2dTexture);

            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            long lastFpsTime = System.nanoTime();
            int frames = 0;

            while (window.pollEvents()) {
                FastDWM.waitForVSync();

                if (window.isKeyJustPressed(0x20)) {
                    showJava2D = !showJava2D;
                    System.out.println("Mode: " + (showJava2D ? "Java2D" : "Vulkan"));
                }

                if (showJava2D) {
                    window.drawImage(j2dTexture, 0f, 0f, (float)winW, (float)winH);
                } else {
                    renderVulkanScene(vkg);
                }

                window.present();

                frames++;
                long now = System.nanoTime();
                if (now - lastFpsTime >= 1_000_000_000L) {
                    String mode = showJava2D ? "JAVA2D" : "VULKAN";
                    window.setTitle("[" + mode + "] FPS: " + frames + "  [SPACE] flip");
                    frames = 0;
                    lastFpsTime = now;
                }
            }

            window.destroyTexture(j2dTexture);
        }
    }
}
