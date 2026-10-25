<#
.SYNOPSIS
    FluxMirror Floating Window Launcher
.DESCRIPTION
    Launches FluxMirror as a standalone floating desktop mirror with screenshot capture and navigation keys.
.EXAMPLE
    .\launch.ps1            # Launches floating app window (like scrcpy)
    .\launch.ps1 -Browser   # Opens in regular browser tab
    .\launch.ps1 -Build     # Builds production web bundle
#>

[CmdletBinding()]
param (
    [switch]$Browser,
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
Write-Host "   Floating Device Mirror, Screenshots & Navigation    " -ForegroundColor DarkGray
Write-Host "========================================================" -ForegroundColor DarkCyan
Write-Host ""

if ($Build) {
    Write-Host "[+] Building production web bundle..." -ForegroundColor Green
    Set-Location $previewerDir
    & npm.cmd run build
    Write-Host "[+] Production build ready in dist/" -ForegroundColor Green
    exit 0
}

Write-Host "[+] Starting FluxMirror on http://localhost:5173..." -ForegroundColor Green
Write-Host "[+] Press Ctrl+C in this terminal to stop." -ForegroundColor DarkGray
Write-Host ""

# Launch floating window by default (Chrome / Edge --app mode)
$chrome = "C:\Program Files\Google\Chrome\Application\chrome.exe"
$edge = "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

Start-Sleep -Milliseconds 600

if (-not $Browser) {
    if (Test-Path $chrome) {
        Start-Process $chrome -ArgumentList "--app=http://localhost:5173 --window-size=400,860"
    } elseif (Test-Path $edge) {
        Start-Process $edge -ArgumentList "--app=http://localhost:5173 --window-size=400,860"
    } else {
        Start-Process "http://localhost:5173"
    }
} else {
    Start-Process "http://localhost:5173"
}

Set-Location $previewerDir
& npm.cmd run dev
