<p align="center">
  <img src="assets/icon.png" width="140" height="140" alt="FluxMirror Logo" />
</p>

# FluxMirror — Native 60–120 FPS Android Screen Mirror & Controller

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C11](https://img.shields.io/badge/C-11-blue.svg?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Windows | macOS | Linux](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-orange.svg)](#-platform-support)
[![GPU: Direct3D11](https://img.shields.io/badge/GPU-Direct3D11%20Hardware%20Accel-0078D7.svg)](#-platform-support)
[![CI Build](https://github.com/milan-m-antony/FluxMirror/actions/workflows/ci.yml/badge.svg)](https://github.com/milan-m-antony/FluxMirror/actions/workflows/ci.yml)

> **High-performance native desktop application for mirroring and controlling Android devices at maximum refresh rate (60–120 FPS), ultra-low latency (<15–25 ms), exact aspect-ratio preservation, persistent floating window mode, unified Brand Control Center, and embedded title bar camera screenshot controls.**

---

## ⚡ Quick Start (No Compilers Required)

For detailed device setup instructions, see [**GETTING_STARTED.md**](GETTING_STARTED.md).

### Option 1: 1-Line PowerShell Launcher (Windows)
Open PowerShell in any folder and run:
```powershell
irm https://raw.githubusercontent.com/milan-m-antony/FluxMirror/main/run.ps1 | iex
```
* Queries the latest official release from GitHub.
* Cryptographically verifies package integrity using SHA-256.
* Caches files in `%LOCALAPPDATA%\FluxMirror` for instant launches.

### Option 2: Pre-Built Portable ZIP
1. Download **`FluxMirror-Windows-x64.zip`** from [**GitHub Releases**](https://github.com/milan-m-antony/FluxMirror/releases).
2. Extract the ZIP anywhere on your PC.
3. Connect your Android phone with **USB Debugging** enabled.
4. Double-click **`start.bat`**.

---

## 🌟 Key Capabilities

- **🚀 Complete Native Foundation (Zero Lag):**
  - Full native C architecture using the complete scrcpy core for device mirroring and input injection.
  - Direct hardware video decoding via **Direct3D11** (`d3d11va`) with zero frame buffering (`--video-buffer 0`).
  - True real-time input response for touch, mouse, and keyboard without browser overhead or latency.
- **⚡ 60–120 FPS High Refresh Rate:**
  - Streams smoothly at your device's maximum panel rate (e.g. 120 FPS on high-refresh panels like Samsung Galaxy S21 FE).
- **🪟 Persistent Floating Window (Always-On-Top):**
  - Window defaults to floating mode (`--always-on-top`), remaining pinned above code editors, terminals, and browsers while you type code.
  - Does not hide, lose focus, or minimize when interacting with background applications.
- **🔥 Unified FluxMirror Brand Control Menu:**
  - Integrated interactive brand pill (`[ 🔥 FluxMirror ▾ ]`) on the top-left of the title bar with drag pass-through support.
  - Left-clicking opens the native **Device Control Center**:
    - **Active Device Indicator & Switcher**: Submenu displaying the active model (e.g. `SM_G990B2`), all connected USB/Wi-Fi devices, and instant 1-click device switching.
    - **1-Click Wireless ADB Switcher**: Automatically initializes `adb tcpip 5555`, queries device Wi-Fi IP, connects over port 5555, and seamlessly switches to wireless mirroring.
    - **Take Screenshot**: Triggers silent snapshot directly to clipboard.
    - **Turn Device Screen Off**: Powers off physical phone display while keeping mirror stream running (`keyevent 223`).
    - **Refresh Device List**: Instant re-scan of connected ADB devices.
- **📷 Integrated Title Bar Camera Notch Screenshot Button:**
  - Built directly into native window controls (docked replacing Maximize to maintain strict phone aspect ratio).
  - Sleek vector camera notch icon styled for dark title bars with hover and pressed states.
  - Native tooltip: *"Take Screenshot (Copy to Clipboard)"*.
- **📋 Direct-to-Clipboard Multi-Format Capture & HUD Toast:**
  - One click on the Camera button captures a full-resolution screen snapshot silently via ADB without flashing command prompt windows.
  - Ingests image into the system clipboard simultaneously as **`CF_DIB`** (bottom-up standard DIB), **`CF_BITMAP`** (GDI bitmap), and **`PNG`**, enabling immediate pasting (`Ctrl + V`) across all applications (Paint, Word, Discord, Telegram, Slack, browsers).
  - Displays a centered, auto-dismissing in-window HUD toast: **`✓ Copied to clipboard`** with audio chime.
- **📐 Ultra-Clean Title Bar & Aspect-Ratio Preservation:**
  - Strictly preserves mobile aspect ratio (e.g. 1080×2340 or 1080×2400) without stretching or letterboxing.
  - Right side strictly hosts **only three streamlined controls**: **Minimize (`─`)**, **Camera Notch (`📷`)**, and **Close (`✕`)**.
  - Maximum clear caption area reserved for smooth native Windows window dragging, snapping, and Aero gestures.

---

## 🌐 Platform Support

| OS Platform | Status | Graphics & Windowing Pipeline | Launch Method |
| :--- | :--- | :--- | :--- |
| **Windows 10 / 11 (64-bit)** | **Pre-compiled Release** (Zero install) | Direct3D11 (`d3d11va`) + Win32 custom dark title bar controls | `start.bat` or `run.ps1` |
| **macOS (Apple Silicon & Intel)** | Build from source | Metal / VideoToolbox + SDL3 Cocoa window | `start.sh` |
| **Linux (Ubuntu, Fedora, Arch)** | Build from source | OpenGL / Vulkan + VA-API hardware acceleration | `start.sh` |

---

## 🛠️ Building Native Engine from Source

### Windows (MSYS2 UCRT64)
Requires [MSYS2](https://www.msys2.org/) with UCRT64 toolchain (gcc, ninja, meson, SDL3, FFmpeg, libusb):
```cmd
build.bat
```
* Automatically locates MSYS2 bash and dynamically resolves repository root paths.
* Compiles the native engine and copies binaries to `bin\`.
* To package a release zip: `powershell -File scripts\package-release.ps1`.

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
├── run.ps1                     # Safe 1-line public PowerShell launcher
├── start.sh                    # macOS & Linux Bash launcher
├── build.bat                   # Dynamic Windows build script (MSYS2 UCRT64)
├── meson.build                 # Root Meson build configuration
├── meson_options.txt           # Build options
├── scrcpy-server               # Prebuilt Android server component
├── GETTING_STARTED.md          # User onboarding guide
├── THIRD_PARTY_NOTICES.md      # Upstream licenses & third-party notices
├── LICENSE                     # Apache License, Version 2.0
├── assets/
│   └── icon.png                # Official application icon
├── scripts/
│   └── package-release.ps1     # Standalone distribution packaging script
├── app/                        # Complete C native source code
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
│       │   └── titlebar_camera.c # Win32 window controls & brand menu
│       └── ...
└── bin/                        # Staged portable standalone binaries
    ├── FluxMirror.exe          # Compiled native executable
    ├── scrcpy-server           # Android server component
    ├── adb.exe                 # Android Debug Bridge
    └── *.dll                   # SDL3, FFmpeg, and USB runtime libraries
```

---

## 📜 License & Attributions

FluxMirror is licensed under the **Apache License, Version 2.0**.

It is a derivative work based on **[scrcpy](https://github.com/Genymobile/scrcpy)**:
- Upstream scrcpy: Copyright (C) 2018 Genymobile, Copyright (C) 2018–2024 Romain Vimont (`rom@rom1v.com`), and scrcpy contributors.
- FluxMirror enhancements and modifications: Copyright (C) 2026 Milan M Antony.

For complete license text, upstream notices, and bundled library attributions (SDL3, FFmpeg, libusb, ADB), see [**LICENSE**](LICENSE) and [**THIRD_PARTY_NOTICES.md**](THIRD_PARTY_NOTICES.md).
