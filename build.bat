@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

echo ========================================================
echo               BUILDING FLUXMIRROR FROM SOURCE           
echo ========================================================

:: 1. Locate MSYS2 bash
set "MSYS_BASH="
if exist "C:\msys64\usr\bin\bash.exe" set "MSYS_BASH=C:\msys64\usr\bin\bash.exe"
if not defined MSYS_BASH if defined MSYS2_ROOT if exist "%MSYS2_ROOT%\usr\bin\bash.exe" set "MSYS_BASH=%MSYS2_ROOT%\usr\bin\bash.exe"
if not defined MSYS_BASH if exist "C:\tools\msys64\usr\bin\bash.exe" set "MSYS_BASH=C:\tools\msys64\usr\bin\bash.exe"
if not defined MSYS_BASH if exist "D:\msys64\usr\bin\bash.exe" set "MSYS_BASH=D:\msys64\usr\bin\bash.exe"

if not defined MSYS_BASH (
    for /f "delims=" %%B in ('where bash.exe 2^>nul') do (
        if exist "%%~dpB..\usr\bin\bash.exe" set "MSYS_BASH=%%~dpB..\usr\bin\bash.exe"
    )
)

if not defined MSYS_BASH (
    echo [x] Error: MSYS2 bash not found.
    echo     Please install MSYS2 from https://www.msys2.org/ or set MSYS2_ROOT.
    echo     Or download pre-built portable releases from GitHub Releases.
    exit /b 1
)

echo [+] Using MSYS2 Bash: !MSYS_BASH!
echo [+] Repository Root: !REPO_ROOT!

:: 2. Execute Meson and Ninja build in UCRT64 environment
echo [+] Running Meson and Ninja build in UCRT64...
"!MSYS_BASH!" -lc "REPO_DIR=$(cygpath -u '!REPO_ROOT!'); export PATH=/ucrt64/bin:$PATH; cd \"$REPO_DIR\" && if [ ! -d build-win64 ]; then meson setup build-win64 -Dcompile_server=false -Dprebuilt_server=\"$REPO_DIR/scrcpy-server\" -Dportable=true --buildtype=release; fi && ninja -C build-win64"

if %ERRORLEVEL% neq 0 (
    echo [x] Build failed! Check the compiler errors above.
    exit /b %ERRORLEVEL%
)

:: 3. Stage binaries to bin\
if not exist "bin" mkdir "bin"

echo [+] Copying compiled binary to bin\FluxMirror.exe...
copy /y build-win64\app\scrcpy.exe bin\FluxMirror.exe >nul
copy /y build-win64\app\scrcpy.exe bin\scrcpy.exe >nul

:: 4. Copy runtime DLLs from UCRT64 if missing in bin\
"!MSYS_BASH!" -lc "REPO_DIR=$(cygpath -u '!REPO_ROOT!'); if [ -d /ucrt64/bin ]; then for f in SDL3.dll avcodec-*.dll avformat-*.dll avutil-*.dll swresample-*.dll libusb-1.0.dll; do cp -u /ucrt64/bin/$f \"$REPO_DIR/bin/\" 2>/dev/null || true; done; fi"

:: 5. Copy scrcpy-server to bin\ if missing
if exist "scrcpy-server" (
    copy /y "scrcpy-server" "bin\scrcpy-server" >nul
)

echo [+] Build completed successfully!
endlocal
