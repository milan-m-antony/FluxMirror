@echo off
setlocal
cd /d "%~dp0\..\.."

echo ========================================================
echo                  FLUXMIRROR NATIVE LAUNCHER             
echo    Native C++20 / Qt 6 High-Performance Mirror Engine   
echo ========================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0start.ps1" %*
endlocal
