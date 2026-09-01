package fastvulkan;

import fastdwm.FastDWM;
import fasttheme.FastTheme;

public class DemoWindow {
    private static java.awt.image.BufferedImage createRoundIcon() {
        java.awt.image.BufferedImage icon = new java.awt.image.BufferedImage(64, 64, java.awt.image.BufferedImage.TYPE_INT_ARGB);
        java.awt.Graphics2D g = icon.createGraphics();
        g.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(java.awt.Color.WHITE);
        g.fillOval(4, 4, 56, 56);
        g.dispose();
        return icon;
    }

    public static void main(String[] args) {
        // Automatically hide the terminal console window if started from .bat
        long consoleHwnd = FastTheme.getConsoleWindowHandle();
        if (consoleHwnd != 0) {
            FastTheme.setWindowTransparency(consoleHwnd, 0);
        }

        try (FastVulkanWindow window = new FastVulkanWindow("ABCDEFGHIJKLMN", 1024, 600)) {
            // Set native round window icon from FastAnimation demo
            window.setIconImage(createRoundIcon());

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                // Apply Dark Mode and Black Titlebar styling via FastTheme
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
                FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
                FastTheme.setWindowBackgroundColor(hwnd, 20, 20, 20);
                FastTheme.setCornerStyle(hwnd, 2); // Windows 11 Rounded corners
            }

            // Set background to noticeable Red
            window.setClearColor(0.88f, 0.12f, 0.12f, 1.0f);

            long lastFpsTime = System.nanoTime();
            int frames = 0;

            while (window.pollEvents()) {
                window.present();

                frames++;
                long now = System.nanoTime();
                if (now - lastFpsTime >= 1_000_000_000L) {
                    window.setTitle("ABCDEFGHIJKLMN - FPS: " + frames);
                    frames = 0;
                    lastFpsTime = now;
                }
            }
        }

        System.out.println("Window closed successfully.");
    }
}
