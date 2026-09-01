package fastvulkan;

import fasttheme.FastTheme;
import javax.swing.JFrame;
import java.awt.Dimension;

public class DemoPureWindow {
    public static void main(String[] args) {
        // Auto-hide console if started from batch
        long consoleHwnd = FastTheme.getConsoleWindowHandle();
        if (consoleHwnd != 0) {
            FastTheme.setWindowTransparency(consoleHwnd, 0);
        }

        JFrame frame = new JFrame("FastJava — Pure Native Window Test (Long Title Bar)");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setPreferredSize(new Dimension(1024, 600));
        frame.pack();
        frame.setLocationRelativeTo(null);

        // Apply dark mode and titlebar styling via FastTheme
        long hwnd = FastTheme.getWindowHandle(frame);
        if (hwnd != 0) {
            FastTheme.setTitleBarDarkMode(hwnd, true);
            FastTheme.setTitleBarColor(hwnd, 20, 20, 20);
            FastTheme.setTitleBarTextColor(hwnd, 240, 240, 240);
            FastTheme.setCornerStyle(hwnd, 2);
        }

        frame.setVisible(true);

        // Dynamically update title with FPS/timer test
        new Thread(() -> {
            int counter = 0;
            while (frame.isVisible()) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException ignored) {}
                counter++;
                final String updatedTitle = "FastJava — Pure Native Window Test (Long Title Bar) - Seconds: " + counter;
                javax.swing.SwingUtilities.invokeLater(() -> frame.setTitle(updatedTitle));
            }
        }, "Title-Updater").start();
    }
}
