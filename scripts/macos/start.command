#!/usr/bin/env bash
# ========================================================
#   FluxMirror — macOS Native Desktop Launcher
#   High-Performance 60–120 FPS Native Mirror Engine
# ========================================================

set -e
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "========================================================"
echo "               FLUXMIRROR MACOS LAUNCHER                "
echo "   Native C++20 / Qt 6 High-Performance Mirror Engine   "
echo "========================================================"
echo ""

# Check ADB
if ! command -v adb &> /dev/null; then
    echo "[!] 'adb' not found. Install via: brew install android-platform-tools"
    exit 1
fi

# Check native binary or scrcpy
NATIVE_BIN=""
if [ -d "$ROOT_DIR/build/macos-clang/MirrorApp.app" ]; then
    NATIVE_BIN="$ROOT_DIR/build/macos-clang/MirrorApp.app/Contents/MacOS/MirrorApp"
elif command -v scrcpy &> /dev/null; then
    NATIVE_BIN="$(command -v scrcpy)"
fi

if [ -z "$NATIVE_BIN" ]; then
    echo "[!] Neither built MirrorApp nor scrcpy found. Install via: brew install scrcpy"
    exit 1
fi

# Query devices
DEVICES=$(adb devices | grep -E '\s+device$' | awk '{print $1}')
if [ -z "$DEVICES" ]; then
    echo "[x] No authorized Android device detected. Connect via USB and enable USB Debugging."
    exit 1
fi

TARGET_SERIAL=$(echo "$DEVICES" | head -n 1)
MODEL=$(adb -s "$TARGET_SERIAL" shell getprop ro.product.model | tr -d '\r')

echo "[+] Connected Device: $MODEL ($TARGET_SERIAL)"
echo "[+] Starting native mirror with Metal / GPU acceleration..."

# Launch native mirror
"$NATIVE_BIN" -s "$TARGET_SERIAL" \
    --window-title "FluxMirror — $MODEL" \
    --max-fps 120 \
    --video-buffer 0 \
    --video-codec h264 \
    --video-bit-rate 16M \
    --always-on-top \
    --stay-awake \
    --print-fps "$@"
