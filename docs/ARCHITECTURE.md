# Architecture Blueprint: Android Web Studio

## 1. Executive Summary

Android Web Studio is a lightweight, cross-platform web client and Node.js bridge that renders a connected Android device's screen directly into an HTML5 Canvas element inside a modern web browser. It provides low-latency gesture interaction, hardware button simulation, live telemetry streaming, and in-memory screenshot captures.

---

## 2. High-Level System Architecture

```
┌────────────────────────────────────────────────────────┐
│                   Web Browser Client                  │
│                                                        │
│  ┌──────────────────┐           ┌──────────────────┐  │
│  │ HTML5 Canvas     │           │ Navigation &     │  │
│  │ Pointer Engine   │           │ Hardware Dock    │  │
│  └────────┬─────────┘           └────────┬─────────┘  │
│           │  Coordinates & Gestures      │ KeyCodes   │
└───────────┼──────────────────────────────┼────────────┘
            │ HTTP POST /api/action        │
            ▼                              ▼
┌────────────────────────────────────────────────────────┐
│              Vite Middleware Server (Node.js)          │
│              (server/adbBridge.ts)                     │
│                                                        │
│  • Frame Poller (/api/frame -> screencap -p stream)   │
│  • Action Router (/api/action -> input tap/swipe/key) │
│  • Device Info (/api/device-info -> getprop / dumpsys) │
└───────────────────────────┬────────────────────────────┘
                            │ Child Process Pipes / Exec
                            ▼
┌────────────────────────────────────────────────────────┐
│              Host OS Android Debug Bridge              │
│                      (adb.exe / adb)                   │
└───────────────────────────┬────────────────────────────┘
                            │ USB / Wi-Fi (Port 5555)
                            ▼
┌────────────────────────────────────────────────────────┐
│                 Target Android Device                  │
│               (e.g., Samsung SM-G990B2)                │
└────────────────────────────────────────────────────────┘
```

---

## 3. Core Subsystems

### 3.1. Canvas Gesture & Coordinate Engine (`src/App.tsx`)

The screen is rendered on an `<canvas>` element whose internal coordinate buffer matches the device's native resolution (e.g. $1080 \times 2400$).

1. **Bounding Box Normalization:**
   ```typescript
   const rect = canvas.getBoundingClientRect();
   const scaleX = canvas.width / rect.width;
   const scaleY = canvas.height / rect.height;

   const deviceX = (e.clientX - rect.left) * scaleX;
   const deviceY = (e.clientY - rect.top) * scaleY;
   ```
2. **Gesture Classification:**
   - On `pointerdown`, timestamp $t_0$ and starting $(x_0, y_0)$ are recorded.
   - On `pointerup`, the displacement distance $d = \sqrt{(x_1 - x_0)^2 + (y_1 - y_0)^2}$ is computed:
     - If $d < 15\text{ px}$: Sent as **Tap** (`adb shell input tap x y`).
     - If $d \ge 15\text{ px}$: Sent as **Swipe** (`adb shell input swipe x0 y0 x1 y1 duration`).
3. **Mouse Wheel Interception:**
   - Wheel delta `e.deltaY` is translated into a vertical swipe starting from screen center:
     $$\Delta y = \text{deltaY} > 0 ? -400 : 400$$

---

### 3.2. In-Memory Clipboard Engine (`handleCopyScreenshot`)

Rather than saving files to the filesystem and requiring manual file dialog picking, the canvas writes directly to an in-memory image blob and injects it into the system clipboard:

```typescript
canvas.toBlob(async (blob) => {
  if (!blob) return;
  await navigator.clipboard.write([
    new ClipboardItem({ 'image/png': blob })
  ]);
}, 'image/png');
```

**Benefits:**
- Zero disk I/O or temporary file litter.
- Native multi-platform support (Windows Clipboard, macOS Pasteboard, X11/Wayland clipboard).
- Immediate `Ctrl + V` pasting into Slack, Discord, GitHub issues, or AI coding assistants.

---

### 3.3. ADB Bridge Middleware (`server/adbBridge.ts`)

Embedded directly into the Vite development and production server using Vite's `configureServer` middleware hook:

1. **Path Resolution:** Dynamically inspects `C:\Android\Sdk\platform-tools\adb.exe`, `%LOCALAPPDATA%\Android\Sdk\...`, and system `$PATH`.
2. **Direct Process Streaming:**
   - `/api/frame`: Spawns `adb exec-out screencap -p` and pipes the binary `stdout` stream directly to the HTTP response (`image/png`), avoiding intermediate file generation on the host OS.
3. **Hardware Keystroke Dispatches:**
   Maps button clicks to Android Linux keycodes:
   - Back: `4`
   - Home: `3`
   - App Switch / Recents: `187`
   - Power / Lock: `26`
   - Wake Up: `224`
   - Volume Up: `24`
   - Volume Down: `25`
   - Rotate Display: `130`

---

## 4. Performance & Latency Metrics

| Component | Target Latency | Implementation Detail |
|---|---|---|
| Screen Capture Stream | 80ms – 130ms | `adb exec-out screencap -p` binary piping |
| Input Tap / Gesture | < 30ms | `adb shell input tap` non-blocking execution |
| Screenshot Copy | < 15ms | In-memory `canvas.toBlob` + `navigator.clipboard` |
| FPS (Polled) | 8 – 12 FPS | Polled frame-loop (Upgrade path: scrcpy WebSocket) |
