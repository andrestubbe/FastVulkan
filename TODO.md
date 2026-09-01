# FastVulkan Roadmap & Open TODOs

## 🐛 Known Issues / Fixes
- [x] **Titelleisten-Text ("F"-Bug)**: Win32 UTF-16 / ANSI-Zeichenkodierung bei `SetWindowText` korrigieren, damit der vollständige Titel angezeigt wird.
- [x] **Initialer Frame-Flash**: Letzte verbleibende 1-Frame-Hintergrundartefakte vor dem ersten Vulkan-Present vollständig beseitigen.
- [ ] **Zero-Load Background Pause**: Bei `SIZE_MINIMIZED` und virtuellem Desktop-Wechsel (DWM Cloaked) den Loop komplett ohne CPU/GPU-Last pausieren (`0 FPS` on demand).

## 🪟 Windowing & FastWindow Extraction
- [ ] **FastWindow Evolution**: Native Win32-Fenstererzeugung (`FastWindow.create(...)`) in FastWindow integrieren (`FastWindow.attach(JFrame)` als Legacy-Modus erhalten).
- [ ] **FastTheme Harmonization**: Titelleistenfarben, Dark Mode, Borders und Fenster-Titel zentral über FastTheme ansteuern.

## 🎨 Perfekte Graphics2D-Emulation (Vulkan 2D Engine)
- [ ] **Isolierte Entwicklung in FastVulkan**: Entwicklung eines 1:1 pixelgetreuen Graphics2D-Duplikats rein in FastVulkan, bevor es später nach FastGraphics portiert wird.
- [ ] **Core Shapes & Batching**: `fillRect`, `drawRect`, `fillRoundRect`, `drawRoundRect`, `fillOval`, `drawOval`, `drawLine`, `drawPolygon`, `fillPolygon`.
- [ ] **SDF Anti-Aliasing**: Shader-basierte kanten- und flackerfreie Kantenglättung für Rundungen und Kurven.
- [ ] **Clipping & Scissor**: Hardware-beschleunigtes Clipping via Vulkan Scissors.
- [ ] **Transform Pipeline**: Matrix-Stack für `translate`, `scale`, `rotate` direkt im Push-Constant-Shader.
- [ ] **Text & Font Pipeline**: Hardware-gerendertes Font-Atlas- und Glyph-Batching für `drawString`.
- [ ] **Alpha & Blending**: Volle ARGB-Unterstützung und Composite-Modi.

## 📦 Demos & Benchmark Validation
- [ ] **Window Demo** (`examples/Demo/Window`): Perfektes Live-Resize und Event-Handling.
- [ ] **Image Demo** (`examples/Demo/Image`): Aspect-Fill-Zoom mit FastTween und Mipmap-Antialiasing.
- [ ] **Shape Demo** (`examples/Demo/Shape`): 1:1 Graphics2D-Shape-Vergleich und TV-Test-Pattern.
- [ ] **Benchmark Suite** (`examples/Demo/Benchmark`): JMH-Throughput-Vergleich gegen Standard-Java2D und FastGraphics DirectX.
