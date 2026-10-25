# Android Web Studio — Device Mirror & Interactive Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![React 19](https://img.shields.io/badge/React-19-61dafb.svg?logo=react&logoColor=black)](https://react.dev)
[![TypeScript](https://img.shields.io/badge/TypeScript-6.0-blue.svg?logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![Vite](https://img.shields.io/badge/Vite-8.0-646CFF.svg?logo=vite&logoColor=white)](https://vitejs.dev)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/milan-m-antony/Android-Web-Studio/pulls)

> **Cross-platform browser-based Android screen mirror, interactive gesture controller, floating Picture-in-Picture window, and instant clipboard screenshot tool.**

---

## 🌟 Key Features

- **🌐 Cross-Platform:** Works anywhere a modern web browser runs (Windows, macOS, Linux).
- **🪟 Floats Like `scrcpy` (2 Ways):**
  - **Always-On-Top PiP Mode:** Click **`Float Screen`** in the header to pop the phone screen into a native desktop Picture-in-Picture window that stays on top of all other IDE and code windows!
  - **Frameless App Mode:** Launch with `launch.bat -Float` to run Chrome/Edge without tabs or address bars as a standalone floating window.
- **⚡ Instant Screenshot to Clipboard:** 1-click **`Copy Screenshot`** button captures the full native resolution screen frame directly into your system clipboard (`navigator.clipboard.write`) for instant `Ctrl + V` pasting into GitHub, chat, or documentation. Zero junk files left on disk!
- **👆 Interactive Touch & Gestures:**
  - **Tap:** Click anywhere on the virtual screen to send an exact ADB tap event (`adb shell input tap x y`) with visual touch ripple animation.
  - **Swipe / Drag:** Click and drag to perform fluid swipe gestures (`adb shell input swipe x1 y1 x2 y2 duration`).
  - **Scroll:** Mouse wheel automatically translates into native vertical swipes for smooth scrolling.
- **⌨️ Remote Text Typing:** Type or paste text/URLs into the web interface and click Send to type it straight onto the phone without using the on-screen keyboard.
- **🧭 Integrated Android Navigation Dock:**
  - **Back** (`◀` — KeyCode 4)
  - **Home** (`●` — KeyCode 3)
  - **Recents / Multitasking** (`■` — KeyCode 187)
- **🎛️ Hardware & Power Controls:** Dedicated buttons for **Power / Lock** (26), **Wake Screen** (224), **Volume Up** (24), **Volume Down** (25), and **Rotate View** (130).
- **📶 Wireless ADB (Wi-Fi Debugging):**
  - Detects the phone's Wi-Fi IP address automatically.
  - 1-click **`Enable TCP/IP 5555`** to unplug USB and mirror wirelessly.
- **📊 Real-Time Stream Telemetry:** Live FPS counter, stream latency tracker (ms), device resolution detection ($1080 \times 2400$), manufacturer/model identification, and battery level & charging state indicator.
- **⚡ Performance Modes:** Switch between **Eco Mode** (12 FPS, low CPU) and **Turbo Mode** (60 FPS, high responsiveness).

---

## 🚀 Quick Start Guide

### Prerequisites
1. **Node.js 18+**: Verify with `node -v`.
2. **Android SDK Platform Tools (ADB)**: Verify with `adb devices`.
3. An Android phone or tablet connected via USB or Wi-Fi (`adb connect <ip>:5555`) with **USB Debugging** enabled in Developer Options.

---

### Option 1: 1-Click Launch (Windows)

#### Standard Browser Mode
Double-click `launch.bat` in this folder:
```cmd
launch.bat
```

#### Floating Window Mode (Frameless like scrcpy)
```cmd
launch.bat -Float
```
Or via PowerShell:
```powershell
.\launch.ps1 -Float
```

---

### Option 2: Standard NPM Workflow (Cross-Platform)

```bash
# 1. Install dependencies (already installed)
npm install

# 2. Start development server with ADB bridge
npm run dev

# 3. Build optimized production web bundle
npm run build
```

Then visit [http://localhost:5173](http://localhost:5173).

---

## 🏗️ Project Architecture

```
previewer/
├── launch.bat               # 1-click standalone Windows batch launcher
├── launch.ps1               # Standalone PowerShell runner with -Float support
├── package.json             # Node.js project manifest & dependencies
├── vite.config.ts           # Vite configuration with embedded ADB Bridge plugin
├── server/
│   └── adbBridge.ts         # Node.js Vite middleware for ADB process orchestration
├── src/
│   ├── App.tsx              # React 19 Canvas gesture engine, PiP floating mode & controls
│   ├── index.css            # Vanilla CSS obsidian & orange design system
│   └── main.tsx             # React application entry point
├── docs/
│   ├── ARCHITECTURE.md      # In-depth architectural blueprint and event engine
│   ├── ROADMAP.md           # Feature roadmap for next development phases
│   └── API_SPECIFICATION.md # REST endpoint documentation for the ADB bridge
└── dist/                    # Production-ready web assets
```

---

## 📄 Documentation

- [Architecture Blueprint](file:///./docs/ARCHITECTURE.md)
- [Feature Roadmap](file:///./docs/ROADMAP.md)
- [API Specification](file:///./docs/API_SPECIFICATION.md)

---

## 📜 License
MIT License. Free for open-source and commercial use.
