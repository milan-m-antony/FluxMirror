# FluxMirror — 60 FPS Device Mirror & Interactive Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![React 19](https://img.shields.io/badge/React-19-61dafb.svg?logo=react&logoColor=black)](https://react.dev)
[![TypeScript](https://img.shields.io/badge/TypeScript-6.0-blue.svg?logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![Vite](https://img.shields.io/badge/Vite-8.0-646CFF.svg?logo=vite&logoColor=white)](https://vitejs.dev)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/FluxMirror/pulls)

> **High-performance 60 FPS Android device mirror, interactive gesture controller, always-on-top floating window, and instant clipboard screenshot tool.**

---

## 🌟 Dual-Engine Modes

| Feature | ⚡ Mode 1: Native 60 FPS Mirror (Default) | 🌐 Mode 2: Web Studio Mode |
| :--- | :--- | :--- |
| **Engine** | Direct3D11 / GPU MediaCodec | React 19 HTML5 Canvas / Vite |
| **Frame Rate** | **60 FPS Buttery Smooth** | Telemetry Canvas Stream |
| **Latency** | **< 15 ms (Zero Lag)** | ~800–1200 ms (USB Screencap) |
| **Aspect Ratio** | Locked to exact phone ratio (1080×2340) | Locked responsive scaling (No stretching) |
| **Always-On-Top** | Native Windows `HWND_TOPMOST` (`--always-on-top`) | W3C Document Picture-in-Picture |
| **Screenshot Tool** | Floating docked `[Copy Screenshot]` pill button | On-screen `Copy Screenshot` button |
| **Navigation** | Right-click (Back), Middle-click (Home), Alt+S (Recents) | On-screen Back (`◀`), Home (`●`), Recents (`■`) |
| **Launch Command** | `.\launch.bat` | `.\launch.bat -Web` |

---

## 🚀 Quick Start Guide

### Prerequisites
1. **Android SDK Platform Tools (ADB)** in PATH or at `C:\Android\Sdk\platform-tools`.
2. Connected Android device with **USB Debugging** enabled.

---

### Option 1: 1-Click Launch (60 FPS Ultra-Fast, Zero Lag)

Double-click `launch.bat` in this folder:
```cmd
launch.bat
```
Or via PowerShell:
```powershell
.\launch.ps1
```
* **Left Click**: Tap & Drag / Swipe
* **Right Click**: Back
* **Middle Click**: Home
* **Alt + S**: Recents / App Switcher
* **Alt + R**: Rotate Screen
* **Docked Pill**: 1-click **Copy Screenshot** straight to clipboard for instant `Ctrl + V` pasting into Antigravity IDE, GitHub, or documentation!

---

### Option 2: Web Studio Mode (Browser & Picture-in-Picture)

```cmd
launch.bat -Web
```
Or via PowerShell:
```powershell
.\launch.ps1 -Web
```
Opens the browser interface on [http://localhost:5173](http://localhost:5173) with:
- HTML5 Canvas zero-broken-icon stream
- Exact aspect-ratio preservation (resizes without stretching)
- Always-On-Top Pin toggle
- On-screen navigation gestures (Back, Home, Recents)

---

## 🏗️ Project Architecture

```
previewer/
├── launch.bat               # 1-click launcher (60 FPS Native Mirror by default)
├── launch.ps1               # PowerShell runner with -Web switch
├── package.json             # Node.js project manifest & dependencies
├── vite.config.ts           # Vite configuration with embedded ADB Bridge plugin
├── tools/
│   └── screenshot_dock.ps1  # Floating Always-on-Top "Copy Screenshot" dock button
├── server/
│   └── adbBridge.ts         # Node.js Vite middleware for ADB process orchestration
├── src/
│   ├── App.tsx              # React 19 Canvas gesture engine, PiP mode & controls
│   ├── index.css            # Responsive exact-aspect-ratio design system
│   └── main.tsx             # React application entry point
├── docs/
│   ├── ARCHITECTURE.md      # In-depth architectural blueprint
│   ├── ROADMAP.md           # Feature roadmap
│   └── API_SPECIFICATION.md # REST endpoint documentation for the ADB bridge
└── dist/                    # Production-ready web assets
```

---

## 📜 License
MIT License. Free for open-source and commercial use.
