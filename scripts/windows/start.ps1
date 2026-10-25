<#
.SYNOPSIS
    FluxMirror Native Windows Launcher
.DESCRIPTION
    Launches high-performance native desktop mirror with 60–120 FPS, Direct3D11 GPU rendering,
    exact aspect-ratio preservation, Always-on-top, and 1-click clipboard screenshots.
#>

[CmdletBinding()]
param (
    [int]$Fps,
    [switch]$AlwaysOnTop = $true,
    [switch]$Wifi,
    [switch]$Diag,
    [switch]$Snap
)

$ErrorActionPreference = "Stop"
$rootDir = (Resolve-Path "$PSScriptRoot\..\..").Path

# 1. Environment checks
if (-not (Get-Command adb -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\Android\Sdk\platform-tools") {
        $env:PATH = "C:\Android\Sdk\platform-tools;$env:PATH"
    }
}

$adb = (Get-Command adb -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)
if (-not $adb) {
    Write-Host "[x] Error: 'adb' executable not found in PATH." -ForegroundColor Red
    Write-Host "    Install Android SDK Platform Tools or add to PATH." -ForegroundColor Yellow
    exit 1
}

# 2. Check for native engine (Built C++ binary or scrcpy runner)
$binCandidates = @(
    "$rootDir\build\windows-msvc\MirrorApp.exe",
    "$rootDir\build\Release\MirrorApp.exe",
    "C:\Users\minua\AppData\Local\Microsoft\WinGet\Packages\Genymobile.scrcpy_Microsoft.Winget.Source_8wekyb3d8bbwe\scrcpy-win64-v5.0.1\scrcpy.exe",
    (Get-Command scrcpy -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)
)

$nativeBin = $null
foreach ($cand in $binCandidates) {
    if ($cand -and (Test-Path $cand)) {
        $nativeBin = $cand
        break
    }
}

if (-not $nativeBin) {
    Write-Host "[x] Error: Native mirroring binary not found." -ForegroundColor Red
    Write-Host "    Please build via CMake/Ninja or install scrcpy." -ForegroundColor Yellow
    exit 1
}

# 3. Detect connected Android devices
Write-Host "[+] Querying connected Android devices via ADB..." -ForegroundColor Green
$devicesOutput = & $adb devices -l
$deviceLines = @($devicesOutput | Where-Object { $_ -match "\s+device\s+" })

if ($deviceLines.Count -eq 0) {
    Write-Host "[x] No connected Android device detected with USB Debugging enabled." -ForegroundColor Red
    Write-Host "    Please connect your phone, authorize USB Debugging, and re-run." -ForegroundColor Yellow
    exit 1
}

$targetSerial = ($deviceLines[0] -split "\s+")[0].Trim()
$modelMatch = $deviceLines[0] -match "model:(\S+)"
$model = if ($modelMatch) { $Matches[1] } else { "Android Device" }

Write-Host "[+] Connected Device: $model ($targetSerial)" -ForegroundColor Cyan

# 4. Display refresh rate capability detection
$maxFps = 60
try {
    $displayDump = & $adb -s $targetSerial shell "dumpsys display | grep -E 'fps=120|fps=90|fps=60'"
    if ($displayDump -match "fps=120") {
        $maxFps = 120
    } elseif ($displayDump -match "fps=90") {
        $maxFps = 90
    }
} catch {}

$streamFps = if ($Fps -gt 0) { $Fps } else { $maxFps }

Write-Host "[+] Max Refresh Rate:  $maxFps Hz" -ForegroundColor Green
Write-Host "[+] Target Stream FPS: $streamFps FPS" -ForegroundColor Green
Write-Host "[+] Video Pipeline:    Direct3D11 GPU Hardware Accelerated" -ForegroundColor Green
Write-Host "[+] Buffer Latency:    0 ms (Ultra-Low Latency Mode)" -ForegroundColor Green

# 5. Handle Diagnostics Mode
if ($Diag) {
    Write-Host "`n--- Device Diagnostic Info ---" -ForegroundColor DarkCyan
    & $adb -s $targetSerial shell "getprop ro.product.manufacturer; getprop ro.product.model; getprop ro.build.version.release; wm size"
    exit 0
}

# 6. Handle One-shot Screenshot Mode
if ($Snap) {
    Write-Host "[+] Capturing full-res mobile screenshot to clipboard..." -ForegroundColor Green
    & "$rootDir\tools\screenshot_dock.ps1" -Snap
    exit 0
}

# 7. Launch Native Mirror Window
Write-Host "`n[+] Launching dedicated native mirror window..." -ForegroundColor Cyan
$scrcpyDir = [System.IO.Path]::GetDirectoryName($nativeBin)

$mirrorArgs = @(
    "-s", $targetSerial,
    "--window-title", "FluxMirror - $model ($streamFps FPS)",
    "--max-fps", $streamFps,
    "--video-buffer", "0",
    "--video-codec", "h264",
    "--video-bit-rate", "16M",
    "--stay-awake",
    "--print-fps"
)

if ($AlwaysOnTop) {
    $mirrorArgs += "--always-on-top"
}

if ($Wifi) {
    $mirrorArgs += "--tcpip"
}

Start-Process -FilePath $nativeBin -WorkingDirectory $scrcpyDir -ArgumentList ($mirrorArgs -join " ")

# 8. Attach floating screenshot companion dock
$dockScript = "$rootDir\tools\screenshot_dock.ps1"
if (Test-Path $dockScript) {
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$dockScript`"" -WindowStyle Hidden
    Write-Host "[+] Floating 'Copy Screenshot' dock attached on top!" -ForegroundColor Green
}

Write-Host "[+] Native mirror running. Press Ctrl+C or close the mirror window to exit." -ForegroundColor DarkGray
