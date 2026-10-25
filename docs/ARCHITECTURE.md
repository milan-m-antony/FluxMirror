# FluxMirror Native Architecture Specification

## Overview
**FluxMirror** is a native C++20 and Qt 6 desktop application engineered for ultra-low latency, high-refresh-rate (60–120 FPS) Android screen mirroring and remote control across Windows, Linux, and macOS.

It avoids browser-based engines, Electron, WebSockets, and WebCodecs entirely. The video pipeline interacts directly with the hardware `MediaCodec` encoder via ADB, decodes through FFmpeg/native hardware APIs, and renders on GPU surfaces via the Qt Rendering Hardware Interface (RHI).

---

## Module Breakdown

### 1. `DeviceManager` (`src/device/`)
- Interacts with ADB daemon to discover connected devices asynchronously.
- Evaluates display refresh capabilities via `dumpsys display` (detecting 60Hz, 90Hz, and 120Hz panels).
- Monitors device hotplugging, connect, and disconnect signals.

### 2. `ConnectionManager` (`src/connection/`)
- Manages USB ADB port-forwarding and reverse tunnels.
- Manages TCP/IP wireless pairing over port 5555.
- Provides clean socket shutdown and tunnel teardown on exit.

### 3. `MirrorEngine` (`src/mirroring/`)
- Orchestrates `scrcpy-server` lifecycle on the Android device.
- Communicates using the binary scrcpy protocol:
  - Video stream socket for H.264/H.265 NAL units.
  - Control socket for injecting touch, keyevents, text, and scroll packets.
- Employs zero frame buffering (`--video-buffer=0`).

### 4. `VideoDecoder` (`src/video/`)
- Decodes incoming video packets using FFmpeg (`libavcodec`).
- Probes hardware decoding acceleration:
  - Windows: Direct3D11 Video Acceleration (`D3D11VA`).
  - macOS: VideoToolbox (`Metal`).
  - Linux: VAAPI / CUDA / Vulkan.
- Configures `AV_CODEC_FLAG_LOW_DELAY` to prevent frame queue accumulation.

### 5. `VideoRenderer` (`src/rendering/`)
- `QQuickPaintedItem` / `QQuickRhiItem` implementing GPU texture blits.
- Strictly locks rendering to the device's physical aspect ratio ($1080 \times 2340$).
- Applies letterboxing (horizontal black bars) or pillarboxing (vertical black bars) dynamically when desktop windows are resized, guaranteeing zero screen distortion or stretching.

### 6. `InputController` (`src/input/`)
- Normalizes desktop mouse clicks, drags, and wheel events to exact Android device screen coordinates:
  $$\text{deviceX} = \text{clamp}\left(0, \text{phoneWidth}, (\text{mouseX} - \text{viewportX}) \times \frac{\text{phoneWidth}}{\text{viewportWidth}}\right)$$
  $$\text{deviceY} = \text{clamp}\left(0, \text{phoneHeight}, (\text{mouseY} - \text{viewportY}) \times \frac{\text{phoneHeight}}{\text{viewportHeight}}\right)$$
- Handles navigation shortcuts: Back (`◀`), Home (`●`), Recents (`■`), Power.

### 7. `ClipboardManager` (`src/clipboard/`)
- 1-click **Copy Screenshot** directly to the operating system's native clipboard (`QClipboard::setImage`).
- Save screenshot to file dialog.
- Bidirectional text synchronization.

### 8. `WindowManager` (`src/window/`)
- Always-On-Top toggle (`Qt::WindowStaysOnTopHint`).
- Fullscreen mode toggle (`F11`).
- Preserves exact aspect ratio when resized.

### 9. `PerformanceMonitor` (`src/diagnostics/`)
- Computes real rendered and decoded FPS over a 1-second sliding window without frame duplication.
- Tracks dropped frames and render latency.
