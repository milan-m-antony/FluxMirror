<#
.SYNOPSIS
    FluxMirror Launcher (60–120 FPS Native Direct3D11 Hardware Mirror)
.DESCRIPTION
    Launches high-performance native desktop mirror with 60–120 FPS, Direct3D11 GPU rendering,
    exact aspect-ratio preservation, Always-on-top, and native title bar camera screenshot capture.
.EXAMPLE
    .\launch.ps1            # Launches native 60–120 FPS mirror
    .\launch.ps1 -Snap      # Captures full-res screenshot directly to clipboard
    .\launch.ps1 -Diag      # Displays device specs & refresh rate diagnostics
    .\launch.ps1 -Fps 120   # Forces 120 FPS stream rate
    .\launch.ps1 -Wifi      # Connects wirelessly over Wi-Fi (ADB TCP/IP)
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
$previewerDir = $PSScriptRoot

& "$previewerDir\scripts\windows\start.ps1" @PSBoundParameters
