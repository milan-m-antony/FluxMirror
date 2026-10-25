@echo off
setlocal
cd /d "%~dp0"

echo ========================================================
echo               BUILDING FLUXMIRROR FROM SOURCE           
echo ========================================================

set "MSYS_BASH=C:\msys64\usr\bin\bash.exe"
if not exist "%MSYS_BASH%" (
    echo [x] Error: MSYS2 bash not found at %MSYS_BASH%
    exit /b 1
)

echo [+] Running Meson and Ninja build in UCRT64...
"%MSYS_BASH%" -lc "export PATH=/ucrt64/bin:$PATH; cd /c/project/previewer && if [ ! -d build-win64 ]; then meson setup build-win64 -Dcompile_server=false -Dprebuilt_server=/c/project/previewer/scrcpy-server -Dportable=true --buildtype=release; fi && ninja -C build-win64"

if %ERRORLEVEL% neq 0 (
    echo [x] Build failed!
    exit /b %ERRORLEVEL%
)

echo [+] Copying compiled binary to bin\FluxMirror.exe...
copy /y build-win64\app\scrcpy.exe bin\FluxMirror.exe >nul
copy /y build-win64\app\scrcpy.exe bin\scrcpy.exe >nul

echo [+] Build completed successfully!
endlocal
