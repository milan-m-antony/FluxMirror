<#
.SYNOPSIS
    Standalone Android Web Studio Launcher
.DESCRIPTION
    Launches the cross-platform Web Studio interface, starts the local ADB bridge server on port 5173, and opens Chrome/Edge.
.EXAMPLE
    .\launch.ps1            # Opens in default browser
    .\launch.ps1 -Float     # Opens as floating standalone window (like scrcpy)
    .\launch.ps1 -Build     # Builds production web bundle
#>

[CmdletBinding()]
param (
    [switch]$Float,
    [switch]$Build
)

$ErrorActionPreference = "Stop"
$previewerDir = $PSScriptRoot

# Auto-configure Node.js and ADB in session PATH
if (-not (Get-Command node -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\Program Files\nodejs") {
        $env:PATH = "C:\Program Files\nodejs;$env:PATH"
    }
}

if (-not (Get-Command adb -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\Android\Sdk\platform-tools") {
        $env:PATH = "C:\Android\Sdk\platform-tools;$env:PATH"
    }
}

Write-Host ""
Write-Host "========================================================" -ForegroundColor DarkCyan
Write-Host "                     FLUXMIRROR                         " -ForegroundColor Cyan
Write-Host "   Cross-Platform Device Mirror & Interactive Controller" -ForegroundColor DarkGray
Write-Host "========================================================" -ForegroundColor DarkCyan
Write-Host ""

if ($Build) {
    Write-Host "[+] Building production web bundle..." -ForegroundColor Green
    Set-Location $previewerDir
    & npm.cmd run build
    Write-Host "[+] Production build ready in dist/" -ForegroundColor Green
    exit 0
}

Write-Host "[+] Starting Android Web Studio on http://localhost:5173..." -ForegroundColor Green
if ($Float) {
    Write-Host "[+] Floating Window Mode (Frameless app window enabled)" -ForegroundColor Yellow
}
Write-Host "[+] Press Ctrl+C in this terminal to stop the server." -ForegroundColor DarkGray
Write-Host ""

# Launch browser / floating window
$chrome = "C:\Program Files\Google\Chrome\Application\chrome.exe"
$edge = "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

Start-Sleep -Milliseconds 600

if ($Float) {
    if (Test-Path $chrome) {
        Start-Process $chrome -ArgumentList "--app=http://localhost:5173 --window-size=440,940"
    } elseif (Test-Path $edge) {
        Start-Process $edge -ArgumentList "--app=http://localhost:5173 --window-size=440,940"
    } else {
        Start-Process "http://localhost:5173"
    }
} else {
    Start-Process "http://localhost:5173"
}

Set-Location $previewerDir
& npm.cmd run dev
