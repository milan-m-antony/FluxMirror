@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ========================================================
echo                  FLUXMIRROR NATIVE ENGINE               
echo      Complete Scrcpy Native C Core + Camera Controls    
echo ========================================================
echo.

:: 1. Resolve ADB executable from multiple standard locations
set "ADB_BIN="
if exist "%~dp0bin\adb.exe" set "ADB_BIN=%~dp0bin\adb.exe"
if not defined ADB_BIN if exist "%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe" set "ADB_BIN=%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"
if not defined ADB_BIN if defined ANDROID_HOME if exist "%ANDROID_HOME%\platform-tools\adb.exe" set "ADB_BIN=%ANDROID_HOME%\platform-tools\adb.exe"
if not defined ADB_BIN if defined ANDROID_SDK_ROOT if exist "%ANDROID_SDK_ROOT%\platform-tools\adb.exe" set "ADB_BIN=%ANDROID_SDK_ROOT%\platform-tools\adb.exe"
if not defined ADB_BIN if exist "C:\Android\Sdk\platform-tools\adb.exe" set "ADB_BIN=C:\Android\Sdk\platform-tools\adb.exe"

if not defined ADB_BIN (
    for /f "delims=" %%A in ('where adb.exe 2^>nul') do (
        if not defined ADB_BIN set "ADB_BIN=%%A"
    )
)

if not defined ADB_BIN (
    set "ADB_BIN=adb.exe"
)

echo [+] Using ADB: !ADB_BIN!

:: 2. Find connected Android device
echo [+] Detecting connected Android device...
set "TARGET_SERIAL="
set "UNAUTH_FOUND=0"

for /f "tokens=1,2" %%A in ('"!ADB_BIN!" devices') do (
    if "%%B"=="device" (
        if not defined TARGET_SERIAL set "TARGET_SERIAL=%%A"
    ) else if "%%B"=="unauthorized" (
        set "UNAUTH_FOUND=1"
    )
)

if not defined TARGET_SERIAL (
    if "!UNAUTH_FOUND!"=="1" (
        echo [!] Warning: An Android device is connected but UNAUTHORIZED.
        echo     Please check your phone screen and tap "Allow USB debugging".
        echo     Check "Always allow from this computer" for convenience.
    ) else (
        echo [x] Error: No Android device detected via ADB.
        echo     1. Connect your device via USB cable or enable Wireless Debugging.
        echo     2. Ensure "USB Debugging" is enabled in Developer Options.
        echo     3. Run 'adb devices' to verify detection.
    )
    exit /b 1
)

echo [+] Target device: !TARGET_SERIAL!

:: 3. Resolve compiled FluxMirror executable
set "FLUX_BIN="
if exist "%~dp0bin\FluxMirror.exe" set "FLUX_BIN=%~dp0bin\FluxMirror.exe"
if not defined FLUX_BIN if exist "%~dp0bin\scrcpy.exe" set "FLUX_BIN=%~dp0bin\scrcpy.exe"
if not defined FLUX_BIN if exist "%~dp0build-win64\app\scrcpy.exe" set "FLUX_BIN=%~dp0build-win64\app\scrcpy.exe"

if not defined FLUX_BIN (
    echo [x] Error: Compiled FluxMirror binary not found.
    echo     Please build using build.bat first or download a release from GitHub.
    exit /b 1
)

:: 4. Process arguments
set "INLINE_MODE=0"
set "EXTRA_ARGS="

:args_loop
if "%~1"=="" goto args_done
if "%~1"=="--inline" (
    set "INLINE_MODE=1"
) else (
    set "EXTRA_ARGS=!EXTRA_ARGS! %1"
)
shift
goto args_loop
:args_done

if "!INLINE_MODE!"=="1" (
    echo [+] Launching FluxMirror inline...
    "!FLUX_BIN!" -s !TARGET_SERIAL! --window-title "FluxMirror" --always-on-top --max-fps 120 --video-buffer 0 --video-codec h264 --video-bit-rate 16M --stay-awake --print-fps !EXTRA_ARGS!
) else (
    echo [+] Spawning dedicated external Command Prompt window outside IDE...
    start "FluxMirror Engine" cmd.exe /c ""!FLUX_BIN!" -s !TARGET_SERIAL! --window-title "FluxMirror" --always-on-top --max-fps 120 --video-buffer 0 --video-codec h264 --video-bit-rate 16M --stay-awake --print-fps !EXTRA_ARGS! || pause"
    echo [+] FluxMirror running in dedicated window outside IDE.
)

endlocal
