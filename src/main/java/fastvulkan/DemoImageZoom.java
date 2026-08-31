package fastvulkan;

import fastdwm.FastDWM;
import fasttheme.FastTheme;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

public class DemoImageZoom {
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

        System.out.println("Screenshot captured: " + imgW + "x" + imgH);

        // Open Vulkan Window (Dark Background)
        try (FastVulkanWindow window = new FastVulkanWindow("FastVulkan — Image Zoom & Antialiasing", 1280, 720, 0.08f, 0.08f, 0.08f, 1.0f)) {
            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            // Upload Screenshot to Vulkan Texture with Automatic Mipmap Generation
            long texture = window.createTexture(pixels, imgW, imgH, true);
            if (texture == 0) {
                throw new RuntimeException("Failed to upload Vulkan texture!");
            }

            float zoom = 1.0f;
            float zoomSpeed = 0.005f;
            boolean zoomingIn = true;
            long lastFpsTime = System.currentTimeMillis();
            int frames = 0;

            while (window.pollEvents()) {
                FastDWM.waitForVSync();

                // Dynamic Smooth Zoom & Center Pan Animation
                if (zoomingIn) {
                    zoom += zoomSpeed;
                    if (zoom >= 2.5f) zoomingIn = false;
                } else {
                    zoom -= zoomSpeed;
                    if (zoom <= 0.4f) zoomingIn = true;
                }

                float winW = 1280.0f;
                float winH = 720.0f;

                float renderW = (winW * 0.8f) * zoom;
                float renderH = (winH * 0.8f) * zoom;
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
