package fastvulkan;

import fasttheme.FastTheme;

public class DemoWindow {
    public static void main(String[] args) {
        // Automatically hide the terminal console window if started from .bat
        long consoleHwnd = FastTheme.getConsoleWindowHandle();
        if (consoleHwnd != 0) {
            FastTheme.setWindowTransparency(consoleHwnd, 0);
        }

        try (FastVulkanWindow window = new FastVulkanWindow("FastVulkan - Zero-Jitter Live Resize", 1024, 600)) {
            
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

            long lastFpsTime = System.currentTimeMillis();
            int frames = 0;

            while (window.pollEvents()) {
                window.present();

                frames++;
                long now = System.currentTimeMillis();
                if (now - lastFpsTime >= 1000) {
                    window.setTitle("FastVulkan — " + frames + " FPS | Zero-Lag Live Resize");
                    frames = 0;
                    lastFpsTime = now;
                }

                try {
                    Thread.sleep(1);
                } catch (InterruptedException ignored) {}
            }
        }

        System.out.println("Window closed successfully.");
    }
}
