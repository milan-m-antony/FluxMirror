# FluxMirror Native Build Guide

This document describes how to build the native C++20 and Qt 6 desktop application across Windows, macOS, and Linux using CMake, Ninja, and vcpkg.

---

## 1. Prerequisites

### Universal Requirements
- **CMake** (v3.22+)
- **Ninja** (v1.10+)
- **Android SDK Platform Tools (ADB)** (v34+)
- **Git**

### Platform-Specific Compilers & Toolchains
- **Windows**: Visual Studio 2022 (MSVC v143+) or Clang with C++20 support.
- **macOS**: Xcode 14+ / AppleClang with C++20 support.
- **Linux**: GCC 11+ or Clang 13+ with C++20 support.

---

## 2. Dependency Setup via vcpkg

Install `vcpkg` and set the `VCPKG_ROOT` environment variable:

```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat    # Windows
./vcpkg/bootstrap-vcpkg.sh     # macOS / Linux
export VCPKG_ROOT=/path/to/vcpkg
```

Dependencies declared in `vcpkg.json`:
- `qtbase[gui,widgets,network,concurrent]`
- `qtdeclarative`
- `ffmpeg[avcodec,avutil,swscale]`

---

## 3. Building with CMake Presets

### Windows (MSVC + Ninja)
```cmd
cmake --preset windows-msvc
cmake --build --preset windows-release
```
Output: `build/windows-msvc/MirrorApp.exe`

### Linux (GCC + Ninja)
```bash
cmake --preset linux-gcc
cmake --build --preset linux-release
```
Output: `build/linux-gcc/MirrorApp`

### macOS (AppleClang + Ninja)
```bash
cmake --preset macos-clang
cmake --build --preset macos-release
```
Output: `build/macos-clang/MirrorApp.app`

---

## 4. Running Unit Tests via CTest

```bash
ctest --preset windows-test   # Windows
ctest --preset linux-test     # Linux
ctest --preset macos-test     # macOS
```

---

## 5. Instant 1-Click Launchers (Pre-compiled / scrcpy Engine)

If build dependencies are not yet installed locally, the native launchers automatically discover your installed native scrcpy GPU engine:

- **Windows**: Double-click `start.bat`
- **Linux**: `./scripts/linux/start.sh`
- **macOS**: Double-click `scripts/macos/start.command`
