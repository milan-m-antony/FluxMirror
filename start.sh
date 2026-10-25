#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================================"
echo "                 FLUXMIRROR NATIVE ENGINE               "
echo "      Cross-Platform Android Screen Mirror & Control    "
echo "========================================================"
echo ""

# 1. Resolve ADB
if command -v adb >/dev/null 2>&1; then
    ADB_BIN="adb"
elif [ -f "$DIR/bin/adb" ]; then
    ADB_BIN="$DIR/bin/adb"
elif [ -f "$HOME/Library/Android/sdk/platform-tools/adb" ]; then
    ADB_BIN="$HOME/Library/Android/sdk/platform-tools/adb"
elif [ -f "$HOME/Android/Sdk/platform-tools/adb" ]; then
    ADB_BIN="$HOME/Android/Sdk/platform-tools/adb"
else
    echo "[x] Error: adb not found in PATH or Android SDK."
    echo "    Please install Android platform-tools."
    exit 1
fi

# 2. Find connected Android device
echo "[+] Detecting connected Android device..."
TARGET_SERIAL=$("$ADB_BIN" devices | awk 'NR>1 && $2=="device" {print $1; exit}')

if [ -z "$TARGET_SERIAL" ]; then
    echo "[x] Error: No Android device detected via ADB."
    echo "    Please connect your device via USB or Wi-Fi debugging."
    exit 1
fi

echo "[+] Target device: $TARGET_SERIAL"

# 3. Resolve compiled binary
if [ -f "$DIR/bin/FluxMirror" ]; then
    FLUX_BIN="$DIR/bin/FluxMirror"
elif [ -f "$DIR/build/app/scrcpy" ]; then
    FLUX_BIN="$DIR/build/app/scrcpy"
elif command -v FluxMirror >/dev/null 2>&1; then
    FLUX_BIN="FluxMirror"
else
    echo "[x] Error: Compiled FluxMirror binary not found."
    echo "    Please build using: meson setup build && ninja -C build"
    exit 1
fi

echo "[+] Launching FluxMirror native window..."
exec "$FLUX_BIN" -s "$TARGET_SERIAL" --window-title "FluxMirror" --always-on-top --max-fps 120 --video-buffer 0 --stay-awake --print-fps "$@"
