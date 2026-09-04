package fastvulkan;

import fasttheme.FastTheme;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.awt.geom.RoundRectangle2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.io.File;

/**
 * GenerateDifferenceImage — Generates a pixel-by-pixel differential image between
 * Java2D (Marlin) and FastVulkan, saving it to disk for calibration analysis.
 */
public class GenerateDifferenceImage {

    private static final int WIN_W = 600;
    private static final int WIN_H = 440;

    private static BufferedImage createTestImage(int size) {
        BufferedImage img = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(new Color(40, 140, 240));
        g.fillOval(2, 2, size - 4, size - 4);
        g.setColor(new Color(255, 200, 50));
        g.fillOval(16, 16, size - 32, size - 32);
        g.setColor(new Color(240, 60, 60));
        g.fillOval(30, 30, size - 60, size - 60);
        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 14));
        g.drawString("IMG", size / 2 - 14, size / 2 + 5);
        g.dispose();
        return img;
    }

    private static BufferedImage renderJava2D(BufferedImage testImg) {
        BufferedImage img = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setColor(new Color(20, 20, 20));
        g.fillRect(0, 0, WIN_W, WIN_H);

        final float S = 100.0f;
        final float Y1 = 40.0f;
        final float Y2 = 170.0f;
        final float Y3 = 300.0f;

        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        // Row 1
        g.setColor(new Color(220, 50, 50));
        g.fill(new Rectangle2D.Float(40, Y1, S, S));

        g.setColor(new Color(50, 200, 120));
        g.fill(new RoundRectangle2D.Float(170, Y1, S, S, 35, 35));

        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(300, Y1, S, S));

        g.drawImage(testImg, 430, (int) Y1, null);

        // Row 2
        g.setStroke(new BasicStroke(1.0f));
        g.setColor(new Color(220, 50, 50));
        g.draw(new Rectangle2D.Float(40, Y2, S, S));

        g.setColor(new Color(50, 200, 120));
        g.draw(new RoundRectangle2D.Float(170, Y2, S, S, 35, 35));

        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(300, Y2, S, S));

        // Row 3
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
        g.setColor(new Color(255, 170, 0));
        g.fill(new Ellipse2D.Float(40, Y3, S, S));

        g.setColor(new Color(180, 100, 255));
        g.draw(new Ellipse2D.Float(170, Y3, S, S));

        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HRGB);
        g.setColor(Color.WHITE);
        g.setFont(new Font("Segoe UI", Font.BOLD, 18));
        g.drawString("FastJava Typography 1:1", 300, Y3 + 30);
        g.setFont(new Font("Segoe UI", Font.PLAIN, 13));
        g.setColor(new Color(180, 180, 180));
        g.drawString("Subpixel Anti-Aliasing (RGB ClearType)", 300, Y3 + 55);
        g.drawString("100% Hardware Accelerated Glyph Pipeline", 300, Y3 + 75);

        g.dispose();
        return img;
    }

    public static void main(String[] args) throws Exception {
        BufferedImage testImg = createTestImage(100);

        // 1. Render Reference Java2D image
        BufferedImage j2d = renderJava2D(testImg);

        // 2. Open Vulkan Window, render Vulkan Faces scene, and capture via Windows GDI Screen/Window Capture
        try (FastVulkanWindow window = new FastVulkanWindow(
                "Vulkan Faces Snapshot", WIN_W, WIN_H,
                0.0784f, 0.0784f, 0.0784f, 1.0f)) {

            long hwnd = window.getHWND();
            if (hwnd != 0) {
                FastTheme.setTitleBarDarkMode(hwnd, true);
                FastTheme.setCornerStyle(hwnd, 2);
            }

            FastVulkanGraphics vkg = new FastVulkanGraphics(window);

            // Render several frames to ensure swapchain is fully presented
            for (int i = 0; i < 20; i++) {
                window.pollEvents();
                final float S = 100.0f;
                final float Y1 = 40.0f;
                final float Y2 = 170.0f;
                final float Y3 = 300.0f;

                vkg.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                vkg.setColor(new Color(220, 50, 50));
                vkg.fill(new Rectangle2D.Float(40, Y1, S, S));

                vkg.setColor(new Color(50, 200, 120));
                vkg.fill(new RoundRectangle2D.Float(170, Y1, S, S, 35, 35));

                vkg.setColor(new Color(255, 170, 0));
                vkg.fill(new Ellipse2D.Float(300, Y1, S, S));

                vkg.setColor(Color.WHITE);
                vkg.drawImage(testImg, 430, Y1);

                vkg.setStrokeWidth(1.0f);
                vkg.setColor(new Color(220, 50, 50));
                vkg.draw(new Rectangle2D.Float(40, Y2, S, S));

                vkg.setColor(new Color(50, 200, 120));
                vkg.draw(new RoundRectangle2D.Float(170, Y2, S, S, 35, 35));

                vkg.setColor(new Color(180, 100, 255));
                vkg.draw(new Ellipse2D.Float(300, Y2, S, S));

                vkg.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
                vkg.setColor(new Color(255, 170, 0));
                vkg.fill(new Ellipse2D.Float(40, Y3, S, S));

                vkg.setColor(new Color(180, 100, 255));
                vkg.draw(new Ellipse2D.Float(170, Y3, S, S));

                vkg.setColor(Color.WHITE);
                vkg.setFont(new Font("Segoe UI", Font.BOLD, 18));
                vkg.drawString("FastJava Typography 1:1", 300, Y3 + 30);
                vkg.setFont(new Font("Segoe UI", Font.PLAIN, 13));
                vkg.setColor(new Color(180, 180, 180));
                vkg.drawString("Subpixel Anti-Aliasing (RGB ClearType)", 300, Y3 + 55);
                vkg.drawString("100% Hardware Accelerated Glyph Pipeline", 300, Y3 + 75);

                window.present();
                Thread.sleep(16);
            }

            // Capture exact client area via Windows API POINT + Robot
            // Win32 ClientToScreen: client (0,0) mapped to screen
            // Standard Win10/11: Left/Right border ~8px, Titlebar ~31px
            int wx = window.getX();
            int wy = window.getY();
            System.out.printf("Window location: x=%d, y=%d%n", wx, wy);
            int clientX = wx + 8;
            int clientY = wy + 31;
            Robot robot = new Robot();
            Rectangle clientRect = new Rectangle(clientX, clientY, WIN_W, WIN_H);
            BufferedImage captured = robot.createScreenCapture(clientRect);

            // Compute Differential Image (Heatmap & Amplified Difference)
            BufferedImage diff = new BufferedImage(WIN_W, WIN_H, BufferedImage.TYPE_INT_ARGB);
            long totalDiff = 0;
            int maxDiff = 0;

            for (int y = 0; y < WIN_H; y++) {
                for (int x = 0; x < WIN_W; x++) {
                    int p1 = j2d.getRGB(x, y);
                    int p2 = captured.getRGB(x, y);

                    int r1 = (p1 >> 16) & 0xFF, g1 = (p1 >> 8) & 0xFF, b1 = p1 & 0xFF;
                    int r2 = (p2 >> 16) & 0xFF, g2 = (p2 >> 8) & 0xFF, b2 = p2 & 0xFF;

                    int dr = Math.abs(r1 - r2);
                    int dg = Math.abs(g1 - g2);
                    int db = Math.abs(b1 - b2);
                    int dTotal = (dr + dg + db) / 3;

                    totalDiff += dTotal;
                    if (dTotal > maxDiff) maxDiff = dTotal;

                    if (dTotal == 0) {
                        diff.setRGB(x, y, 0xFF000000); // Identical: pure black
                    } else {
                        // Amplify difference by 5x with neon coloration for instant visual inspection
                        int ampR = Math.min(255, dr * 5);
                        int ampG = Math.min(255, dg * 5);
                        int ampB = Math.min(255, db * 5);
                        diff.setRGB(x, y, 0xFF000000 | (ampR << 16) | (ampG << 8) | ampB);
                    }
                }
            }

            File outDir = new File("docs/diff");
            outDir.mkdirs();
            File fJ2d = new File(outDir, "1_java2d_marlin.png");
            File fVulkan = new File(outDir, "2_vulkan_faces.png");
            File fDiff = new File(outDir, "3_differential_amplified.png");

            ImageIO.write(j2d, "png", fJ2d);
            ImageIO.write(captured, "png", fVulkan);
            ImageIO.write(diff, "png", fDiff);

            double avgDiff = (double) totalDiff / (WIN_W * WIN_H);
            System.out.printf("Differential Analysis Complete:%n");
            System.out.printf("  Average Delta: %.2f / 255 (%.2f%%)%n", avgDiff, (avgDiff / 255.0) * 100);
            System.out.printf("  Max Delta:     %d / 255%n", maxDiff);
            System.out.printf("  Saved: %s%n", fDiff.getAbsolutePath());
        }
    }
}
