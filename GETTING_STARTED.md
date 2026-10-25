# Getting Started with FluxMirror

**FluxMirror** is a high-performance, low-latency (60–120 FPS) native desktop application for mirroring and controlling Android devices. It comes completely self-contained—**no compilers, MSYS2, or Android Studio required**.

---

## ⚡ Quick Start (Windows)

### Option A: 1-Line Instant Launcher (PowerShell)
Open PowerShell and run:
```powershell
irm https://raw.githubusercontent.com/milan-m-antony/FluxMirror/main/run.ps1 | iex
```
* Automatically downloads the latest verified release from GitHub Releases.
* Verifies SHA-256 integrity checksum.
* Caches files in `%LOCALAPPDATA%\FluxMirror` for instant launches next time.

---

### Option B: Manual ZIP Download

1. Go to the [Releases](https://github.com/milan-m-antony/FluxMirror/releases) page.
2. Download the latest `FluxMirror-Windows-x64.zip`.
3. Right-click the downloaded ZIP and click **Extract All...**
4. Connect your Android phone to your PC via USB cable.
5. Double-click **`start.bat`**.

---

## 📱 Preparing Your Android Device

Before launching FluxMirror, enable **USB Debugging** on your phone (required only once):

1. **Enable Developer Options**:
   - Open your phone's **Settings** ➔ **About phone** ➔ **Software information**.
   - Tap **Build number** 7 times until you see *"You are now a developer!"*.
2. **Enable USB Debugging**:
   - Go back to **Settings** ➔ **Developer options**.
   - Turn on the toggle for **USB debugging**.
3. **Authorize Your Computer**:
   - Plug your phone into your computer via USB.
   - A prompt will appear on your phone: *"Allow USB debugging?"*.
   - Check the box **"Always allow from this computer"** and tap **Allow**.

---

## 🎮 Features & Controls

### Title Bar Controls
- **`[ 🔥 FluxMirror ▾ ]` Brand Menu (Top-Left)**:
  - **Active Device**: Displays the currently active phone model and lets you switch between multiple connected USB/Wi-Fi devices with 1 click.
  - **1-Click Wireless ADB (Port 5555)**: Queries your phone's local Wi-Fi IP and automatically connects wirelessly. You can then unplug your USB cable!
  - **Take Screenshot (`Ctrl + Shift + S`)**: Silently copies full-resolution screenshot to clipboard.
  - **Turn Device Screen Off**: Turns off your physical phone display to save battery while continuing to mirror on your computer.
- **`[ 📷 ]` Camera Notch Button**:
  - Located on the right side next to Minimize.
  - One click silently captures the phone screen directly to your clipboard in `PNG` and standard bitmap formats with an in-window toast notification.
- **`[ ─ ]` Minimize & `[ ✕ ]` Close**:
  - Clean native window controls.

---

## ⌨️ Helpful Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl` + `Shift` + `S` | Take screenshot and copy to clipboard |
| `Alt` + `O` | Turn device screen off (keep mirroring) |
| `Alt` + `P` | Power button (screen on/off) |
| `Alt` + `H` | Home button |
| `Alt` + `B` / Right-Click | Back button |
| `Alt` + `S` | App switcher (recent apps) |
| `Alt` + `Up` / `Down` | Volume Up / Volume Down |

---

## ❓ Troubleshooting

### 1. `Warning: Device connected but UNAUTHORIZED`
- Your phone detected your computer, but you haven't approved the connection prompt yet.
- Unlock your phone screen and tap **Allow** on the "Allow USB debugging?" dialog.

### 2. `Error: No Android device detected`
- Ensure your USB cable supports **data transfer** (some cables are charge-only).
- Try another USB port (preferably a direct motherboard USB port, not an unpowered USB hub).
- If using Samsung devices, install the official [Samsung Android USB Driver](https://developer.samsung.com/android-usb-driver) if Windows does not detect the device.

### 3. Wireless ADB Switcher Subnet Error
- Make sure your PC and your phone are connected to the **same Wi-Fi network** (and your router allows client-to-client communication).

---

## 📜 License & Attributions
FluxMirror is licensed under the **Apache License, Version 2.0**. It is a derivative work of [scrcpy](https://github.com/Genymobile/scrcpy) by Genymobile. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for complete details.
