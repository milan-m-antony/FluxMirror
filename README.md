# FluxMirror — Native 60–120 FPS Android Screen Mirror & Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C11](https://img.shields.io/badge/C-11-blue.svg?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Direct3D11](https://img.shields.io/badge/GPU-Direct3D11-0078D7.svg)](https://docs.microsoft.com/en-us/windows/win32/direct3d11/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/FluxMirror/pulls)

> **High-performance, cross-platform desktop application for mirroring and controlling Android devices at maximum refresh rate (60–120 FPS), ultra-low latency (<15–25 ms), exact aspect-ratio preservation, and embedded title bar camera screenshot controls.**

---

## 🌟 Key Capabilities

- **🚀 Complete Native Scrcpy Foundation (Zero Lag):**
  - Full native C architecture using the complete scrcpy core for device mirroring and control.
  - Direct hardware video decoding via **Direct3D11** with D3D11VA interop and zero frame buffering (`--video-buffer 0`).
  - True real-time input response for touch, mouse, and keyboard.
- **⚡ 60–120 FPS High Refresh Rate:**
  - Streams smoothly at high refresh rates (up to 120 FPS on supported panels like Samsung Galaxy S21 FE).
- **📷 Integrated Title Bar Camera Notch Screenshot Button:**
  - Built directly into the native Windows window controls logic (replacing the Maximize button).
  - No external helper tools, overlay processes, or separate window wrappers.
  - Sleek vector camera notch icon styled for dark title bars with subtle hover and pressed states.
  - Includes native tooltip: *"Take Screenshot (Copy to Clipboard)"*.
- **📋 Direct-to-Clipboard Capture & In-Window HUD Toast:**
  - Clicking the Camera button captures a full-resolution screen snapshot silently via ADB.
  - Ingests the image directly into the Windows system clipboard as a 32-bit DIB bitmap.
  - Shows an obsidian pill toast notification: **`✓ Copied to clipboard`** (featuring obsidian glass `#1E293B`, vibrant Flux orange border `#FF6B00`, and emerald checkmark badge `#10B981`) with audio chime.
- **🖤 Obsidian Black Flux Logo:**
  - Window title bar and taskbar display the custom **Black Flux Logo** (`icon.ico` / `scrcpy.png`).
- **🪟 Aspect-Ratio Locking & Streamlined Controls:**
  - Maximize button replaced with the screenshot camera button to preserve mobile phone aspect ratios.
  - Clean **Minimize (`─`)** and **Close (`✕`)** buttons remain fully functional.
  - Smooth edge resizing with locked aspect ratio.

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

* **Windows**: `build.bat` (or `ninja -C build-win64`)
* **macOS & Linux**: `meson setup build && ninja -C build`

---

## 🏗️ Repository Structure

```
previewer/
├── start.bat                   # Dedicated external CMD launcher
├── terminal.bat                # 1-click standalone external terminal
├── start.sh                    # macOS & Linux Bash launcher
├── build.bat                   # Windows build script
├── meson.build                 # Root Meson build configuration
├── meson_options.txt           # Build options
├── scrcpy-server               # Prebuilt Android server jar
├── app/                        # Complete C native source code
│   ├── meson.build             # App build definition
│   ├── scrcpy-windows.rc       # Windows resources & branding
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
