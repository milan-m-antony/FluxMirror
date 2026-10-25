# FluxMirror — Feature Roadmap & Next Steps

This roadmap outlines planned enhancements for developing FluxMirror as a standalone project in a dedicated window or repository.

---

## 🗺️ Release Phases

### Phase 1: Standalone Core & UX (Completed ✅)
- [x] React 19 + TypeScript + Vite project scaffolding.
- [x] Realistic mobile phone chassis with camera notch.
- [x] In-memory one-click screenshot capture to clipboard (`navigator.clipboard.write`).
- [x] Native coordinate mapping for touch taps, drags/swipes, and wheel scrolling.
- [x] Android navigation dock (Back, Home, Recents).
- [x] Hardware controls (Power, Wake, Volume, Rotation).
- [x] Device telemetry (Manufacturer, Model, Android version, resolution, battery level).
- [x] Standalone 1-click launchers (`launch.bat`, `launch.ps1`).

---

### Phase 2: Ultra-Low Latency Video Streaming (60 FPS) 🚀
**Goal:** Transition from polled `screencap` images to full 60 FPS H.264 video streaming.

- [ ] **Scrcpy Server Integration:** Push `scrcpy-server.jar` to `/data/local/tmp/` on the device and stream the raw H.264 byte stream over a local WebSocket.
- [ ] **WebCodecs API Client:** Use the browser's native hardware-accelerated `VideoDecoder` API to render 60 FPS video directly to `<canvas>` with sub-20ms latency.
- [ ] **Dynamic Bitrate Scaling:** Automatically scale resolution and bitrate based on connection type (USB 3.0 vs 2.4GHz Wi-Fi).

---

### Phase 3: Multi-Device Management & Wireless Pairing 📶
- [ ] **Device Selector Dropdown:** Header dropdown to switch between multiple connected phones/emulators instantly.
- [ ] **Wireless ADB GUI:**
  - One-click `adb tcpip 5555` switch for wireless connection.
  - QR Code generator & pairing dialog for Android 11+ wireless debugging (`adb pair`).
- [ ] **Battery Health Monitor:** Real-time charging rate (mA), temperature, and health indicators.

---

### Phase 4: Developer Tools & Session Capture 🎥
- [ ] **Screen Video Recording:** Record interactions directly in-browser using `MediaRecorder` API and export as `.webm` / `.mp4`.
- [ ] **Logcat Live Viewer:** Collapsible terminal drawer streaming `adb logcat` with regex filters and tag search.
- [ ] **APK Drag-and-Drop Installer:** Drop any `.apk` file directly onto the phone chassis to execute `adb install -r`.
- [ ] **Text Input Forwarding:** Type directly on the physical keyboard to send ASCII text into focused Android input fields (`adb shell input text`).

---

### Phase 5: Browser-Native WebUSB (Zero-Install Mode) 🌐
- [ ] Use the WebUSB API (`navigator.usb`) to talk directly to the Android device from Chromium-based browsers (Chrome, Edge, Brave, Opera) without requiring ADB or Node.js installed on the host machine.
- [ ] Deployable as a static Progressive Web App (PWA) on GitHub Pages.
