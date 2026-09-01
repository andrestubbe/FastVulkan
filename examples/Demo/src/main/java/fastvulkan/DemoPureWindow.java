package fastvulkan;

public class DemoPureWindow {
    public static void main(String[] args) {
        // Create 100% pure native Win32 window (no Swing, no JFrame) using FastVulkanWindow directly
        try (FastVulkanWindow window = new FastVulkanWindow("FastJava — Pure Native Win32 Window", 1024, 600, 0.1f, 0.1f, 0.1f, 1.0f)) {
            
            long lastFpsTime = System.nanoTime();
            int frames = 0;

            while (window.pollEvents()) {
                window.present();

                frames++;
                long now = System.nanoTime();
                if (now - lastFpsTime >= 1_000_000_000L) {
                    window.setTitle("FastJava Native Win32 - FPS: " + frames);
                    frames = 0;
                    lastFpsTime = now;
                }
            }
        }

        System.out.println("Native window closed successfully.");
    }
}
