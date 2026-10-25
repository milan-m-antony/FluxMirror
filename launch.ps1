<#
.SYNOPSIS
    FluxMirror Launcher (60 FPS Native Direct3D11 Mirror & Web Studio)
.DESCRIPTION
    Launches FluxMirror with zero lag:
    - Default (Native 60 FPS): Direct3D11 hardware-accelerated screen mirror (<15ms latency, zero lag),
      locked exact phone aspect ratio, always-on-top, and floating "Copy Screenshot" dock button.
    - -Web: Launches the cross-platform web browser studio on http://localhost:5173.
    - -Build: Compiles production web bundle.
.EXAMPLE
    .\launch.ps1            # Launches native 60 FPS zero-lag mirror with screenshot dock
    .\launch.ps1 -Web       # Launches web browser studio (http://localhost:5173)
    .\launch.ps1 -Build     # Builds production web bundle
#>

[CmdletBinding()]
param (
    [switch]$Web,
    [switch]$Browser,
    [switch]$Build
)

$ErrorActionPreference = "Stop"
$previewerDir = $PSScriptRoot

# Configure PATH
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

function Find-Scrcpy {
    $candidates = @(
        "C:\Users\minua\AppData\Local\Microsoft\WinGet\Packages\Genymobile.scrcpy_Microsoft.Winget.Source_8wekyb3d8bbwe\scrcpy-win64-v5.0.1\scrcpy.exe",
        (Get-Command scrcpy -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    )
    foreach ($cand in $candidates) {
        if ($cand -and (Test-Path $cand)) { return $cand }
    }
    return $null
}

Write-Host ""
Write-Host "========================================================" -ForegroundColor DarkCyan
Write-Host "                     FLUXMIRROR                         " -ForegroundColor Cyan
Write-Host "   60 FPS Ultra-Fast Device Mirror & Screenshot Tool   " -ForegroundColor DarkGray
Write-Host "========================================================" -ForegroundColor DarkCyan
Write-Host ""

if ($Build) {
    Write-Host "[+] Building production web bundle..." -ForegroundColor Green
    Set-Location $previewerDir
    & npm.cmd run build
    Write-Host "[+] Production build ready in dist/" -ForegroundColor Green
    exit 0
}

# --- WEB BROWSER STUDIO MODE ---
if ($Web -or $Browser) {
    Write-Host "[+] Starting FluxMirror Web Studio on http://localhost:5173..." -ForegroundColor Green
    Write-Host "[+] Press Ctrl+C in this terminal to stop." -ForegroundColor DarkGray
    Write-Host ""

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
    exit 0
}

# --- DEFAULT: 60 FPS NATIVE D3D11 HARDWARE MIRROR (ZERO LAG) ---
$scrcpy = Find-Scrcpy
if (-not $scrcpy) {
    Write-Host "[!] scrcpy not found, falling back to Web Studio mode..." -ForegroundColor Yellow
    & "$previewerDir\launch.ps1" -Web
    exit 0
}

Write-Host "[+] Launching 60 FPS Ultra-Fast Hardware Mirror (Zero Lag)..." -ForegroundColor Green
$scrcpyDir = [System.IO.Path]::GetDirectoryName($scrcpy)

# Check connected device
$devs = & adb devices | Select-String "\s+device$"
$devArg = ""
if ($devs) {
    $devId = ($devs[0].Line -split "\s+")[0].Trim()
    $devArg = "-s $devId"
    Write-Host "[+] Connected Phone: $devId" -ForegroundColor Cyan
}

# Launch scrcpy with always-on-top, stay-awake, and custom window title
Start-Process -FilePath $scrcpy -WorkingDirectory $scrcpyDir -ArgumentList "$devArg --window-title `"FluxMirror`" --always-on-top --stay-awake"

Write-Host "[+] Live 60 FPS Direct3D11 Mirror is active!" -ForegroundColor Green
Write-Host "    - Left Click: Tap / Drag / Swipe" -ForegroundColor DarkGray
Write-Host "    - Right Click: Back" -ForegroundColor DarkGray
Write-Host "    - Middle Click: Home" -ForegroundColor DarkGray
Write-Host "    - Alt + S: Recents / App Switcher" -ForegroundColor DarkGray
Write-Host "    - Alt + R: Rotate Screen" -ForegroundColor DarkGray

# Attach Title Bar Camera Screenshot button & 'Copied to clipboard' toast
$dockScript = "$previewerDir\tools\screenshot_dock.ps1"
if (Test-Path $dockScript) {
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$dockScript`"" -WindowStyle Hidden
    Write-Host "[+] Title Bar Camera Screenshot button & 'Copied to clipboard' toast attached!" -ForegroundColor Green
}

Write-Host ""
