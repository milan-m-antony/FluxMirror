@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ========================================================
echo                  FLUXMIRROR NATIVE ENGINE               
echo      Complete Scrcpy Native C Core + Camera Controls    
echo ========================================================
echo.

:: 1. Resolve ADB
set "ADB_BIN=%~dp0bin\adb.exe"
if not exist "!ADB_BIN!" (
    if exist "C:\Android\Sdk\platform-tools\adb.exe" (
        set "ADB_BIN=C:\Android\Sdk\platform-tools\adb.exe"
    ) else (
        set "ADB_BIN=adb.exe"
    )
)

:: 2. Find connected Android device
echo [+] Detecting connected Android device...
for /f "tokens=1,2" %%A in ('"!ADB_BIN!" devices') do (
    if "%%B"=="device" (
        if not defined TARGET_SERIAL set "TARGET_SERIAL=%%A"
    )
)

if not defined TARGET_SERIAL (
    echo [x] Error: No Android device detected via ADB.
    echo     Please connect your device via USB or Wi-Fi debugging.
    exit /b 1
)

echo [+] Target device: !TARGET_SERIAL!

:: 3. Launch native compiled FluxMirror
set "FLUX_BIN=%~dp0bin\FluxMirror.exe"
if not exist "!FLUX_BIN!" (
    set "FLUX_BIN=%~dp0bin\scrcpy.exe"
)

if not exist "!FLUX_BIN!" (
    echo [x] Error: Compiled FluxMirror binary not found.
    echo     Please build using ninja in build-win64.
    exit /b 1
)

echo [+] Launching FluxMirror native window...
"!FLUX_BIN!" -s !TARGET_SERIAL! --window-title "FluxMirror" --max-fps 120 --video-buffer 0 --video-codec h264 --video-bit-rate 16M --stay-awake --print-fps %*

endlocal
