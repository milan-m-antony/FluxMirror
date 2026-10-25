<p align="center">
  <img src="assets/icon.png" width="140" height="140" alt="FluxMirror Logo" />
</p>

# FluxMirror — Cross-Platform Native 60–120 FPS Android Screen Mirror & Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C11](https://img.shields.io/badge/C-11-blue.svg?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Windows | macOS | Linux](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-orange.svg)](#-cross-platform-architecture)
[![Direct3D11 | Metal | OpenGL](https://img.shields.io/badge/GPU-Direct3D11%20%7C%20Metal%20%7C%20OpenGL-0078D7.svg)](#-cross-platform-architecture)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/FluxMirror/pulls)

> **High-performance, cross-platform native desktop application for mirroring and controlling Android devices at maximum refresh rate (60–120 FPS), ultra-low latency (<15–25 ms), exact aspect-ratio preservation, persistent floating window mode, and embedded title bar camera screenshot controls.**

---

## 🌐 Cross-Platform Architecture

FluxMirror is built from the ground up for high-performance cross-platform operations on **Windows**, **macOS**, and **Linux**, utilizing native platform rendering APIs:

| OS Platform | GPU Pipeline & Hardware Acceleration | Windowing & Controls Architecture | 1-Click Launcher |
| :--- | :--- | :--- | :--- |
| **Windows 10 / 11** | **Direct3D11** + D3D11VA hardware video decoding | Native Win32 window procedure; Maximize button replaced with embedded Camera Notch Screenshot Button; Topmost floating mode | `start.bat` |
| **macOS (Apple Silicon & Intel)** | **Metal** + VideoToolbox hardware decoding | Native AppKit / Cocoa floating window with high-DPI Retina scaling and title bar integration | `start.sh` |
| **Linux (Ubuntu, Fedora, Arch)** | **OpenGL / Vulkan** + VA-API / DRM acceleration | Wayland / X11 floating mirror window with native input event injection | `start.sh` |

---

## 🌟 Key Capabilities

- **🚀 Complete Native Foundation (Zero Lag):**
  - Full native C architecture using the complete scrcpy core for device mirroring and control.
  - Direct hardware video decoding via **Direct3D11** (Windows), **Metal** (macOS), and **OpenGL/VA-API** (Linux) with zero frame buffering (`--video-buffer 0`).
  - True real-time input response for touch, mouse, and keyboard without browser overhead or latency.
- **⚡ 60–120 FPS High Refresh Rate:**
  - Streams smoothly at the display's maximum panel rate (e.g. 120 FPS on high-refresh panels like Samsung Galaxy S21 FE).
- **🪟 Persistent Floating Window (Always-On-Top):**
  - Window defaults to floating mode (`--always-on-top`), remaining pinned above code editors, terminals, and browsers while you type code.
  - Does not hide, lose focus, or minimize when interacting with background applications.
- **📷 Integrated Title Bar Camera Notch Screenshot Button:**
  - Built directly into the native window controls logic (replacing the Maximize button to maintain strict phone aspect ratio).
  - Sleek vector camera notch icon styled for dark title bars with subtle hover and pressed states.
  - Includes native tooltip: *"Take Screenshot (Copy to Clipboard)"*.
- **📋 Direct-to-Clipboard Multi-Format Capture & HUD Toast:**
  - One click on the Camera button captures a full-resolution screen snapshot silently via ADB without flashing command prompt windows.
  - Ingests image into the system clipboard simultaneously as **`CF_DIB`** (bottom-up standard DIB), **`CF_BITMAP`** (GDI bitmap), and **`PNG`**, enabling immediate pasting (`Ctrl + V` / `Cmd + V`) across all applications (Paint, Word, Discord, Telegram, Slack, browsers).
  - Displays a centered, auto-dismissing in-window HUD toast: **`✓ Copied to clipboard`** with audio chime.
- **📐 Aspect-Ratio Locking & Streamlined Controls:**
  - Strictly preserves mobile aspect ratio (e.g. 1080×2340 or 1080×2400) without stretching or letterboxing.
  - Clean **Minimize (`─`)** and **Close (`✕`)** buttons remain fully functional.

---

## 🚀 1-Click Launchers

### Windows (Dedicated CMD Outside IDE)
Double-click or run from terminal:
```cmd
start.bat
```
* Automatically spawns a **dedicated external Command Prompt window outside the IDE** on your desktop showing real-time streaming FPS and logs.
* To open a dedicated standalone Command Prompt in the project folder anytime: `terminal.bat`.
* To run inline in the current terminal: `start.bat --inline`.

### macOS & Linux (Terminal)
```bash
chmod +x start.sh
./start.sh
```

---

## 🛠️ Building Native Engine from Source

### Windows
Requires MSYS2 UCRT64 toolchain (gcc, ninja, meson, SDL3, FFmpeg):
```cmd
build.bat
```

### macOS & Linux
```bash
meson setup build -Dcompile_server=false -Dprebuilt_server=scrcpy-server --buildtype=release
ninja -C build
```

---

## 🏗️ Repository Structure

```
previewer/
├── start.bat                   # Windows dedicated external CMD launcher
├── terminal.bat                # 1-click standalone external terminal
├── start.sh                    # macOS & Linux Bash launcher
├── build.bat                   # Windows build script
├── meson.build                 # Root Meson build configuration
├── meson_options.txt           # Build options
├── scrcpy-server               # Prebuilt Android server jar
├── assets/
│   └── icon.png                # Official 3D application icon
├── app/                        # Complete C native cross-platform source code
│   ├── meson.build             # App build definition
│   ├── scrcpy-windows.rc       # Windows resources & branding
│   ├── data/
│   │   ├── icon.ico            # Windows executable & taskbar icon
│   │   ├── scrcpy.png          # Window icon
│   │   └── disconnected.png    # Disconnect screen texture
│   └── src/
│       ├── main.c              # Entry point
│       ├── scrcpy.c            # Scrcpy core coordinator
│       ├── screen.c            # Window & rendering manager
│       ├── sys/win/
│       │   ├── titlebar_camera.h # Native camera button header
│       │   └── titlebar_camera.c # Win32 window controls modification
│       └── ...
└── bin/                        # Compiled portable standalone binaries
    ├── FluxMirror.exe          # Compiled native executable
    ├── scrcpy-server           # Android server component
    ├── adb.exe                 # Android Debug Bridge
    └── *.dll                   # SDL3, FFmpeg, and USB runtime libraries
```

---

## 📜 License

MIT License. Free for personal, academic, and commercial use.
