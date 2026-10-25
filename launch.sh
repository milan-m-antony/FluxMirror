#!/usr/bin/env bash
# ========================================================
#   FluxMirror — Universal macOS & Linux Native Launcher
#   High-Performance 60–120 FPS Zero-Lag Android Mirror
# ========================================================

set -e

# Detect OS
OS_TYPE="$(uname -s)"

# Color codes
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo ""
echo -e "${CYAN}========================================================${NC}"
echo -e "${CYAN}                     FLUXMIRROR                         ${NC}"
echo -e "   Universal 60–120 FPS Native Mirror (macOS & Linux)   "
echo -e "${CYAN}========================================================${NC}"
echo ""

# Find ADB
if ! command -v adb &> /dev/null; then
    echo -e "${YELLOW}[!] 'adb' not found in PATH.${NC}"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo -e "${CYAN}[+] Run: brew install android-platform-tools${NC}"
    else
        echo -e "${CYAN}[+] Run: sudo apt install adb (or your distro package manager)${NC}"
    fi
    exit 1
fi

# Find SCRCPY
if ! command -v scrcpy &> /dev/null; then
    echo -e "${YELLOW}[!] 'scrcpy' not found in PATH.${NC}"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo -e "${CYAN}[+] Run: brew install scrcpy${NC}"
    else
        echo -e "${CYAN}[+] Run: sudo apt install scrcpy (or snap install scrcpy)${NC}"
    fi
    exit 1
fi

# Parse CLI arguments
TARGET_FPS="120"
ALWAYS_ON_TOP="--always-on-top"
TCP_MODE=""
SNAP_MODE=false
DIAG_MODE=false

for arg in "$@"; do
    case $arg in
        --fps=*)
            TARGET_FPS="${arg#*=}"
            ;;
        --no-always-on-top)
            ALWAYS_ON_TOP=""
            ;;
        --wifi|--tcpip)
            TCP_MODE="--tcpip"
            ;;
        --snap|--screenshot)
            SNAP_MODE=true
            ;;
        --diag|--diagnostics)
            DIAG_MODE=true
            ;;
    esac
done

# Diagnostics Mode
if [ "$DIAG_MODE" = true ]; then
    echo -e "${GREEN}[+] Running Pipeline Diagnostics...${NC}"
    adb devices -l
    echo ""
    echo -e "${GREEN}[+] Querying Display Capabilities...${NC}"
    adb shell "dumpsys display | grep -E 'fps|refreshRate|mSupportedModes'" | head -n 10
    exit 0
fi

# One-shot Screenshot Mode
if [ "$SNAP_MODE" = true ]; then
    echo -e "${GREEN}[+] Capturing mobile screen directly to clipboard...${NC}"
    TEMP_FILE="/tmp/flux_screen_$(date +%s).png"
    adb exec-out screencap -p > "$TEMP_FILE"
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        osascript -e "set the clipboard to (read (POSIX file \"$TEMP_FILE\") as «class PNGf»)"
    else
        if command -v xclip &> /dev/null; then
            xclip -selection clipboard -t image/png -i "$TEMP_FILE"
        elif command -v wl-copy &> /dev/null; then
            wl-copy -t image/png < "$TEMP_FILE"
        fi
    fi
    rm -f "$TEMP_FILE"
    echo -e "${GREEN}[+] Screenshot copied to system clipboard! Press Cmd+V / Ctrl+V to paste.${NC}"
    exit 0
fi

# Verify connected device
DEVICES=$(adb devices | grep -E '\s+device$' | awk '{print $1}')
if [ -z "$DEVICES" ]; then
    echo -e "${RED}[x] No authorized Android device detected.${NC}"
    echo -e "${YELLOW}[!] Connect your phone via USB and enable USB Debugging.${NC}"
    exit 1
fi

DEVICE_ID=$(echo "$DEVICES" | head -n 1)
MODEL=$(adb -s "$DEVICE_ID" shell getprop ro.product.model | tr -d '\r')

echo -e "${GREEN}[+] Connected Device:${NC} $MODEL ($DEVICE_ID)"
echo -e "${GREEN}[+] Target Refresh:${NC}  $TARGET_FPS FPS"
echo -e "${GREEN}[+] Latency Mode:${NC}    Ultra-Low Latency (Buffer = 0 ms)"
echo -e "${GREEN}[+] Rendering:${NC}       Direct GPU Pipeline"
echo ""
echo -e "${CYAN}[+] Interactive Controls:${NC}"
echo "    - Left Click: Tap / Drag / Swipe"
echo "    - Right Click: Back"
echo "    - Middle Click: Home"
echo "    - Alt/Option + S: Recents / Multitasking"
echo "    - Alt/Option + R: Rotate Screen"
echo "    - Double-click borders: Remove black bars"
echo ""

# Launch scrcpy with zero buffer, locked aspect ratio, live FPS counter, and always-on-top
scrcpy -s "$DEVICE_ID" \
    --window-title "FluxMirror — $MODEL ($TARGET_FPS FPS)" \
    --max-fps "$TARGET_FPS" \
    --video-buffer 0 \
    --video-codec h264 \
    --video-bit-rate 16M \
    $ALWAYS_ON_TOP \
    --stay-awake \
    --print-fps \
    $TCP_MODE
