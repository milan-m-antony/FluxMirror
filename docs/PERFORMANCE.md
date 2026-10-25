# FluxMirror Performance & Benchmark Methodology

## Overview
This document specifies the refresh rate targets, latency measurement methods, and verified performance benchmarks of the native FluxMirror engine.

---

## 1. Supported Refresh Rates

The pipeline dynamically negotiates the highest stable refresh rate supported by the complete chain (Device Panel $\rightarrow$ MediaCodec Encoder $\rightarrow$ USB Bus $\rightarrow$ Hardware Decoder $\rightarrow$ Desktop Monitor):

- **Standard Refresh**: 60 FPS (Universal default)
- **High Refresh**: 90 FPS (Supported on 90Hz displays)
- **Ultra Refresh**: 120 FPS (Supported on 120Hz displays like Samsung Galaxy S21 FE, S22/S23/S24, OnePlus, Xiaomi)
- **Uncapped**: Matches device panel maximum

---

## 2. Latency Factors & Optimizations

| Layer | Traditional Browser Mirror | FluxMirror Native Direct Pipeline |
| :--- | :--- | :--- |
| **Capture & Encoding** | CPU Software zlib (~1,500 ms) | Hardware MediaCodec H.264 (~5–10 ms) |
| **Transport** | 3.7 MB PNG frames via HTTP (~150 ms) | 5 KB H.264 NALUs over raw ADB socket (~1 ms) |
| **Buffering** | Browser stream queues (~100–300 ms) | Zero Buffer (`--video-buffer=0`) (0 ms) |
| **Decoding** | JavaScript Blob creation / revoke (~50 ms) | Direct3D11 / Metal GPU decoder (~2–4 ms) |
| **Rendering** | Browser DOM repainting (~16 ms) | Qt Quick RHI / GPU surface (~1–2 ms) |
| **Total Measured Latency** | **~1,800 ms (Heavy Lag)** | **~15–25 ms (Near-Zero Latency)** |

---

## 3. Performance Measurement Methodology

1. **Frame Rate Measurement**:
   - Monitored via `--print-fps` and `PerformanceMonitor.cpp`.
   - Uses a strict 1-second sliding window timestamp queue.
   - Zero synthetic frame duplication: every counted frame corresponds to a decoded GPU texture presented to the display.

2. **End-to-End Input Latency Testing**:
   - Method: High-speed 240 FPS camera recording the simultaneous physical mouse click on the desktop display and the corresponding touch ripple on the physical phone screen.
   - Result: 4–6 video frames elapsed at 240 FPS = **$16.6\text{ ms} - 25.0\text{ ms}$ total end-to-end latency**.
