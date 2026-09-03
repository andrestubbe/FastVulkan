package fastvulkan;

import fastdwm.FastDWM;
import fasttheme.FastTheme;
import fasttween.Ease;
import fasttween.FastTween;
import fasttween.Tween;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class DemoImageZoom {
    private static float currentZoom = 1.0f;

    public static void main(String[] args) throws Exception {
        // Capture Desktop Screenshot via Robot
        System.out.println("Capturing Desktop screenshot...");
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        Robot robot = new Robot();
        BufferedImage screenshot = robot.createScreenCapture(new Rectangle(0, 0, screenSize.width, screenSize.height));
        
        int imgW = screenshot.getWidth();
        int imgH = screenshot.getHeight();
        int[] pixels = ((DataBufferInt) screenshot.getRaster().getDataBuffer()).getData();

        // Open Vulkan Window
        try (FastVulkanWindow window = new FastVulkanWindow("FastVulkan — Image Zoom & Antialiasing", 1280, 720, 0.08f, 0.08f, 0.08f, 1.0f)) {
            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            // Upload Screenshot to Vulkan Texture with Mipmap Generation
            long texture = window.createTexture(pixels, imgW, imgH, true);
            if (texture == 0) {
                throw new RuntimeException("Failed to upload Vulkan texture!");
            }

            // Create smooth continuous FastTween looping back and forth
            final Tween[] zoomTweenHolder = new Tween[1];
            final boolean[] forward = { true };

            Runnable startNextTween = new Runnable() {
                @Override
                public void run() {
                    float from = forward[0] ? 1.0f : 2.2f;
                    float to = forward[0] ? 2.2f : 1.0f;
                    forward[0] = !forward[0];

                    zoomTweenHolder[0] = FastTween.to(from, to, 3500)
                            .ease(Ease.CUBIC_IN_OUT)
                            .onUpdate(val -> currentZoom = val)
                            .onComplete(this)
                            .start();
                }
            };
            startNextTween.run();

            // Set native round window icon
            java.awt.image.BufferedImage icon = new java.awt.image.BufferedImage(64, 64, java.awt.image.BufferedImage.TYPE_INT_ARGB);
            java.awt.Graphics2D gIcon = icon.createGraphics();
            gIcon.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
            gIcon.setColor(java.awt.Color.WHITE);
            gIcon.fillOval(4, 4, 56, 56);
            gIcon.dispose();
            window.setIconImage(icon);

            FastVulkanGraphics vkg = new FastVulkanGraphics(window);
            long lastFpsTime = System.nanoTime();
            int frames = 0;

            while (window.pollEvents()) {
                FastDWM.waitForVSync();

                if (zoomTweenHolder[0] != null) {
                    zoomTweenHolder[0].update();
                }

                float winW = (float)window.getWidth();
                float winH = (float)window.getHeight();

                // Compute aspect-ratio fill scale so image ALWAYS completely covers the entire window without borders
                float scaleX = winW / (float)imgW;
                float scaleY = winH / (float)imgH;
                float baseScale = Math.max(scaleX, scaleY);

                float finalScale = baseScale * currentZoom;
                float renderW = (float)imgW * finalScale;
                float renderH = (float)imgH * finalScale;
                float posX = (winW - renderW) * 0.5f;
                float posY = (winH - renderH) * 0.5f;

                // Draw hardware-filtered antialiased textured quad using FastVulkanGraphics API
                vkg.drawImage(texture, posX, posY, renderW, renderH);

                window.present();

                frames++;
                long now = System.nanoTime();
                if (now - lastFpsTime >= 1_000_000_000L) {
                    window.setTitle("FastVulkan FPS: " + frames);
                    frames = 0;
                    lastFpsTime = now;
                }
            }

            window.destroyTexture(texture);
        }
    }
}
