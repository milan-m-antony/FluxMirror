<#
.SYNOPSIS
    Universal Automation Launcher for FluxMirror
.DESCRIPTION
    Forwards directly to the native 60–120 FPS start.ps1 launcher script.
#>

[CmdletBinding()]
param (
    [int]$Fps,
    [switch]$AlwaysOnTop = $true,
    [switch]$Wifi,
    [switch]$Diag,
    [switch]$Snap
)

& "$PSScriptRoot\scripts\windows\start.ps1" @PSBoundParameters
