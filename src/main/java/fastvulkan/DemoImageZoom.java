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
        // Auto-hide console if started from batch
        long consoleHwnd = FastTheme.getConsoleWindowHandle();
        if (consoleHwnd != 0) {
            FastTheme.setWindowTransparency(consoleHwnd, 0);
        }

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

            // Create smooth continuous FastTween looping between 1.0x (full cover) and 2.2x zoom
            Tween zoomTween = FastTween.to(1.0f, 2.2f, 3500)
                    .ease(Ease.CUBIC_IN_OUT)
                    .yoyo(true)
                    .repeat(-1)
                    .onUpdate(val -> currentZoom = val)
                    .start();

            long lastFpsTime = System.currentTimeMillis();
            int frames = 0;

            while (window.pollEvents()) {
                FastDWM.waitForVSync();

                zoomTween.update();

                float winW = (float)window.getWidth();
                float winH = (float)window.getHeight();
                if (winW <= 0) winW = 1280.0f;
                if (winH <= 0) winH = 720.0f;

                // Compute aspect-ratio fill scale so image ALWAYS completely covers the entire window without borders
                float scaleX = winW / (float)imgW;
                float scaleY = winH / (float)imgH;
                float baseScale = Math.max(scaleX, scaleY);

                float finalScale = baseScale * currentZoom;
                float renderW = (float)imgW * finalScale;
                float renderH = (float)imgH * finalScale;
                float posX = (winW - renderW) * 0.5f;
                float posY = (winH - renderH) * 0.5f;

                // Draw hardware-filtered antialiased textured quad
                window.drawImage(texture, posX, posY, renderW, renderH);

                window.present();

                frames++;
                long now = System.currentTimeMillis();
                if (now - lastFpsTime >= 1000) {
                    window.setTitle("FastVulkan FPS:" + frames);
                    frames = 0;
                    lastFpsTime = now;
                }
            }

            window.destroyTexture(texture);
        }
    }
}
