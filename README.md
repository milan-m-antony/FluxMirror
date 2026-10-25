# FluxMirror — Native 60–120 FPS Android Screen Mirror & Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6.4+-41CD52.svg?logo=qt)](https://www.qt.io/)
[![Direct3D11](https://img.shields.io/badge/GPU-Direct3D11-0078D7.svg)](https://docs.microsoft.com/en-us/windows/win32/direct3d11/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/FluxMirror/pulls)

> **High-performance, native desktop application for mirroring and controlling Android devices with maximum refresh rate (60–120 FPS), ultra-low latency (<15–25 ms), aspect-ratio preservation, and embedded title bar screenshot capture.**

---

## 🌟 Key Capabilities

- **🚀 Native Direct3D11 GPU Pipeline (Zero Lag):**
  - Direct hardware video decoding via **Direct3D11** (Windows), **Metal** (macOS), and **OpenGL/Vulkan** (Linux).
  - Eliminates all browser, WebCodecs, and WebSocket overhead.
  - Zero-latency frame buffer (`--video-buffer=0`) delivering true real-time response.
- **⚡ 60–120 FPS Ultra-High Refresh Rate:**
  - Automatically queries connected panel capabilities via ADB (detects 60Hz, 90Hz, and 120Hz high-refresh displays).
  - Matches the native refresh rate of modern devices (e.g., Samsung Galaxy S21 FE 120Hz).
- **📷 Embedded Title Bar Camera Capture Button:**
  - Sleek camera icon button (`[ 📷 ]`) embedded directly inside the top window title bar, adjacent to the **Minimize** button.
  - Styled to match Windows 11 title bar caption buttons with subtle translucent hover highlights.
  - Configured with `WS_EX_NOACTIVATE` so clicking the camera button never unfocuses the mirror or interrupts your phone interaction.
- **📋 "Copied to clipboard" HUD Toast Notification:**
  - One click on the title bar camera icon immediately captures the full-resolution screenshot from the device via ADB.
  - Automatically copies the image to the Windows Clipboard (`Ctrl + V`).
  - Displays a centered, floating HUD toast: **`✓ Copied to clipboard`** (featuring obsidian glass `#1E293B`, vibrant Flux orange border `#FF6B00`, and emerald checkmark badge `#10B981`) with audio chime.
- **🖤 Screen Top Black Flux Logo:**
  - Window title bar displays the obsidian **Black Flux Logo** (`flux_black.png` / `flux_black.ico`) instead of the generic Android robot.
- **🪟 Streamlined Window Controls (Minimize & Close Only):**
  - Removed Maximize box (`WS_MAXIMIZEBOX` stripped) to maintain strict aspect-ratio integrity.
  - Leaves clean **Minimize (`_`)** and **Close (`✕`)** buttons.
- **📐 Exact Screen Aspect-Ratio Preservation:**
  - Strictly preserves your phone's physical aspect ratio (e.g., $1080 \times 2340$).
  - Resizing dynamically adapts without stretching, squishing, or distorting UI elements.
- **🎮 Responsive Input & Navigation Controls:**
  - **Left-Click:** Tap / Drag / Swipe
  - **Right-Click:** Back (`◀`)
  - **Middle-Click:** Home (`●`)
  - **Alt + S:** Recents / App Switcher (`■`)
  - **Alt + R:** Rotate Screen
  - **Ctrl + C / Ctrl + V:** Bidirectional clipboard synchronization

---

## 🚀 1-Click Launchers

### Windows

Run the universal launcher directly from the repository root:

```cmd
automate.bat
```

*(Or via PowerShell: `.\automate.ps1`)*

#### Useful Command-Line Options:

| Command | Action |
| :--- | :--- |
| `automate.bat` | Launches 60–120 FPS native mirror with title bar camera capture |
| `automate.bat -Snap` | Instantly grabs mobile screenshot directly to clipboard and exits |
| `automate.bat -Diag` | Displays connected device specifications and refresh rate diagnostics |
| `automate.bat -Fps 120` | Forces target stream rate to 120 FPS |
| `automate.bat -Wifi` | Connects wirelessly to device over Wi-Fi (ADB TCP/IP) |

### Linux
```bash
chmod +x ./scripts/linux/start.sh
./scripts/linux/start.sh
```

### macOS
```bash
chmod +x ./scripts/macos/start.command
./scripts/macos/start.command
```

---

## 🏗️ Architecture & Module Breakdown

```
previewer/
├── automate.bat                # 1-click Windows universal launcher
├── automate.ps1                # Universal PowerShell launcher script
├── start.bat                   # Windows batch forwarder
├── CMakeLists.txt              # Root C++20 / Qt 6 CMake build configuration
├── CMakePresets.json           # Native build presets (MSVC, GCC, AppleClang)
├── vcpkg.json                  # C++ dependency manifest (Qt 6, FFmpeg)
├── tools/
│   └── screenshot_dock.ps1     # Native title bar camera decorator & HUD toast
├── resources/
│   └── icons/
│       ├── flux_black.ico      # Black Flux title bar icon (multi-resolution)
│       ├── flux_black.png      # 256x256 high-resolution black Flux icon
│       └── scrcpy.png          # Replaced native SDL window icon
├── scripts/
│   ├── windows/start.ps1       # Windows hardware pipeline launcher
│   ├── linux/start.sh          # Linux X11/Wayland launcher
│   └── macos/start.command     # macOS launcher
├── src/
│   ├── app/                    # Entrypoint & CLI parameter handling
│   ├── device/                 # ADB discovery & 120Hz panel detection
│   ├── connection/             # USB tunnel & Wi-Fi TCP/IP pairing
│   ├── mirroring/              # scrcpy protocol & stream lifecycle
│   ├── video/                  # Hardware video decoding (D3D11VA, Metal, VAAPI)
│   ├── rendering/              # GPU viewport surface & aspect ratio lock
│   ├── input/                  # Normalized touch coordinate injection
│   ├── clipboard/              # System clipboard screenshot ingestion
│   └── window/                 # Always-on-top & caption management
└── docs/
    ├── ARCHITECTURE.md         # Deep-dive architecture specification
    ├── BUILDING.md             # Native compilation instructions
    └── PERFORMANCE.md          # Latency benchmarks & FPS methodology
```

---

## 🛠️ Building Native Engine from Source

```bash
# Windows (MSVC + Ninja)
cmake --preset windows-msvc
cmake --build --preset windows-release

# Linux (GCC + Ninja)
cmake --preset linux-gcc
cmake --build --preset linux-release

# macOS (AppleClang + Ninja)
cmake --preset macos-clang
cmake --build --preset macos-release
```

---

## 📜 License

MIT License. Free for personal, academic, and commercial use.
