# FluxMirror — Native 60–120 FPS Android Screen Mirror & Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6.4+-41CD52.svg?logo=qt)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.22+-064F8C.svg?logo=cmake)](https://cmake.org/)
[![Ninja](https://img.shields.io/badge/Ninja-Fast-black.svg)](https://ninja-build.org/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/FluxMirror/pulls)

> **High-performance, native-first desktop application for mirroring and controlling Android devices with maximum refresh rate (60–120 FPS) and ultra-low latency across Windows, Linux, and macOS.**

---

## 🌟 Key Capabilities

- **🚀 Native-First Performance (Zero Browser Overhead):**
  - Built with **C++20** and **Qt 6 / QML**.
  - Direct GPU rendering via **Direct3D11** (Windows), **Metal** (macOS), and **OpenGL/Vulkan** (Linux).
  - Eliminates all WebSockets, WebCodecs, and browser overhead.
- **⚡ Maximum Refresh Rate (60–120+ FPS):**
  - Auto-detects device panel capabilities (e.g. 120Hz display on Samsung Galaxy S21 FE / modern flagships).
  - Supports configurable stream targets: **60 FPS**, **90 FPS**, **120 FPS**, or uncapped.
- **⏱️ Ultra-Low End-to-End Latency (~15–25 ms):**
  - Zero-buffer pipeline (`--video-buffer=0`).
  - Hardware H.264 / H.265 video encoding via device `MediaCodec`.
  - Non-blocking asynchronous input event processing.
- **📐 Exact Screen Aspect-Ratio Preservation:**
  - Locks strictly to your device's physical aspect ratio ($1080 \times 2340$).
  - Smooth dynamic window resizing with pillarboxing / letterboxing — **never stretches, squishes, or distorts**.
  - Automatic adaptation on device rotation (portrait $\leftrightarrow$ landscape).
- **📌 Desktop Window Management:**
  - **Always-On-Top** mode: stays pinned above IDEs and code editors.
  - **Fullscreen** mode (`F11`).
- **📷 Instant Clipboard Screenshots:**
  - 1-click **`Copy Screenshot`** button copies the full-resolution frame directly to your system clipboard (`Ctrl + V` / `Cmd + V`).
  - Save screenshot dialog to export PNG directly to disk.
- **🎮 Responsive Input Controls:**
  - Left-Click: Tap / Drag / Swipe.
  - Right-Click: **Back** (`◀`).
  - Middle-Click: **Home** (`●`).
  - Alt + S: **Recents / Multitasking** (`■`).
  - Alt + R: **Rotate Screen**.
  - Bidirectional text clipboard synchronization (`Ctrl + C` / `Ctrl + V`).

---

## 🚀 Quick Start (1-Click Launchers)

### 1. Windows
Double-click `start.bat` in the repository root:
```cmd
start.bat
```
*(Or via PowerShell: `.\scripts\windows\start.ps1 -Fps 120`)*

### 2. Linux
```bash
chmod +x ./scripts/linux/start.sh
./scripts/linux/start.sh
```

### 3. macOS
Double-click `scripts/macos/start.command` or run:
```bash
chmod +x ./scripts/macos/start.command
./scripts/macos/start.command
```

---

## 🏗️ Repository Structure

```
previewer/
├── CMakeLists.txt              # Root C++20 / Qt 6 CMake build configuration
├── CMakePresets.json           # Native presets for MSVC, GCC, and AppleClang
├── vcpkg.json                  # C++ dependency manifest (Qt 6, FFmpeg)
├── start.bat                   # 1-click Windows native launcher
├── src/
│   ├── app/
│   │   ├── main.cpp            # Application entrypoint & CLI argument parser
│   │   └── AppController.cpp   # Coordinates modules into QML context
│   ├── device/
│   │   └── DeviceManager.cpp   # ADB discovery, refresh rate (120Hz) detection
│   ├── connection/
│   │   └── ConnectionManager.cpp # USB & Wi-Fi TCP/IP tunnels
│   ├── mirroring/
│   │   ├── MirrorEngine.cpp    # Scrcpy process & protocol orchestration
│   │   └── ScrcpyProtocol.hpp  # Binary packet & control message definitions
│   ├── video/
│   │   └── VideoDecoder.cpp    # FFmpeg / hardware decoding pipeline
│   ├── rendering/
│   │   └── VideoRenderer.cpp   # Qt Quick GPU surface with aspect ratio lock
│   ├── input/
│   │   └── InputController.cpp # Normalized coordinate mapping & navigation
│   ├── clipboard/
│   │   └── ClipboardManager.cpp# Native clipboard screenshots & text sync
│   ├── window/
│   │   └── WindowManager.cpp   # Always-on-top, fullscreen & resize management
│   ├── diagnostics/
│   │   └── PerformanceMonitor.cpp # Real FPS counter, dropped frames, latency
│   └── ui/
│       ├── Main.qml            # Core QML window & keyboard shortcuts
│       ├── MirrorViewport.qml  # Video canvas & bottom navigation dock
│       ├── DeviceToolbar.qml   # Top status bar, screenshot copy & pin toggle
│       └── PerformanceOverlay.qml # Live FPS telemetry overlay
├── scripts/
│   ├── windows/start.bat / start.ps1
│   ├── linux/start.sh
│   └── macos/start.command
├── tests/
│   ├── CMakeLists.txt          # CTest suite
│   ├── test_aspect_ratio.cpp   # Validates exact aspect-ratio math
│   └── test_coordinate_mapping.cpp # Validates pixel-perfect touch mapping
└── docs/
    ├── ARCHITECTURE.md         # Detailed architectural breakdown
    ├── BUILDING.md             # Compilation guide for Windows, Linux, macOS
    └── PERFORMANCE.md          # Benchmark methodology & latency report
```

---

## 🛠️ Building from Source

See [docs/BUILDING.md](file:///./docs/BUILDING.md) for complete build instructions using CMake Presets:

```bash
# Windows
cmake --preset windows-msvc
cmake --build --preset windows-release

# Linux
cmake --preset linux-gcc
cmake --build --preset linux-release

# macOS
cmake --preset macos-clang
cmake --build --preset macos-release
```

---

## 📜 License
MIT License. Free for personal, academic, and commercial use.
