<#
.SYNOPSIS
    Packages the complete standalone portable FluxMirror Windows x64 distribution.
#>
[CmdletBinding()]
param(
    [string]$OutputDir = "$PSScriptRoot\..\dist",
    [string]$Version = "5.0.1"
)

Add-Type -AssemblyName System.IO.Compression.FileSystem
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path "$PSScriptRoot\..").Path
$OutputDir = Join-Path $RepoRoot "dist"
$DistDir = Join-Path $OutputDir "FluxMirror-Windows-x64"
$ZipFile = Join-Path $OutputDir "FluxMirror-Windows-x64.zip"
$ShaFile = Join-Path $OutputDir "SHA256SUMS.txt"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "         PACKAGING FLUXMIRROR PORTABLE RELEASE          " -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "[+] Repository Root: $RepoRoot"
Write-Host "[+] Destination:     $DistDir"

# 1. Clean previous build artifacts in dist
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
if (Test-Path $ZipFile) {
    Remove-Item -Force $ZipFile
}
if (Test-Path $ShaFile) {
    Remove-Item -Force $ShaFile
}
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

# 2. Locate compiled executable
$ExeSource = ""
if (Test-Path "$RepoRoot\build-win64\app\scrcpy.exe") {
    $ExeSource = "$RepoRoot\build-win64\app\scrcpy.exe"
} elseif (Test-Path "$RepoRoot\build\app\scrcpy.exe") {
    $ExeSource = "$RepoRoot\build\app\scrcpy.exe"
} elseif (Test-Path "$RepoRoot\bin\FluxMirror.exe") {
    $ExeSource = "$RepoRoot\bin\FluxMirror.exe"
}

if (-not $ExeSource) {
    throw "Compiled scrcpy.exe / FluxMirror.exe not found! Please run build.bat first."
}

Write-Host "[+] Copying executables from $ExeSource..."
Copy-Item $ExeSource "$DistDir\FluxMirror.exe" -Force
Copy-Item $ExeSource "$DistDir\scrcpy.exe" -Force

# 3. Copy scrcpy-server
if (-not (Test-Path "$RepoRoot\scrcpy-server")) {
    throw "scrcpy-server not found in repository root!"
}
Copy-Item "$RepoRoot\scrcpy-server" "$DistDir\scrcpy-server" -Force
Write-Host "[+] Copied scrcpy-server"

# 4. Copy scripts and documentation
$RequiredDocs = @(
    "start.bat",
    "terminal.bat",
    "LICENSE",
    "THIRD_PARTY_NOTICES.md"
)

foreach ($doc in $RequiredDocs) {
    if (Test-Path "$RepoRoot\$doc") {
        Copy-Item "$RepoRoot\$doc" "$DistDir\$doc" -Force
        Write-Host "[+] Copied $doc"
    } else {
        Write-Warning "Missing $doc"
    }
}

if (Test-Path "$RepoRoot\GETTING_STARTED.md") {
    Copy-Item "$RepoRoot\GETTING_STARTED.md" "$DistDir\GETTING_STARTED.md" -Force
    Write-Host "[+] Copied GETTING_STARTED.md"
}

# Icon asset
if (Test-Path "$RepoRoot\assets\icon.png") {
    Copy-Item "$RepoRoot\assets\icon.png" "$DistDir\scrcpy.png" -Force
} elseif (Test-Path "$RepoRoot\bin\scrcpy.png") {
    Copy-Item "$RepoRoot\bin\scrcpy.png" "$DistDir\scrcpy.png" -Force
}

# 5. Resolve UCRT64 runtime DLLs
$UcrtBin = ""
if (Test-Path "C:\msys64\ucrt64\bin") {
    $UcrtBin = "C:\msys64\ucrt64\bin"
} elseif (Test-Path "$env:MSYS2_ROOT\ucrt64\bin") {
    $UcrtBin = "$env:MSYS2_ROOT\ucrt64\bin"
}

$DllPatterns = @(
    "SDL3.dll",
    "avcodec-*.dll",
    "avformat-*.dll",
    "avutil-*.dll",
    "swresample-*.dll",
    "libusb-1.0.dll"
)

foreach ($pattern in $DllPatterns) {
    $found = $false
    if ($UcrtBin) {
        $matches = Get-ChildItem -Path $UcrtBin -Filter $pattern -ErrorAction SilentlyContinue
        if ($matches) {
            foreach ($m in $matches) {
                Copy-Item $m.FullName "$DistDir\" -Force
                Write-Host "[+] Copied DLL: $($m.Name)"
            }
            $found = $true
        }
    }
    
    if (-not $found -and (Test-Path "$RepoRoot\bin")) {
        $matches = Get-ChildItem -Path "$RepoRoot\bin" -Filter $pattern -ErrorAction SilentlyContinue
        if ($matches) {
            foreach ($m in $matches) {
                Copy-Item $m.FullName "$DistDir\" -Force
                Write-Host "[+] Copied DLL (from bin): $($m.Name)"
            }
            $found = $true
        }
    }

    if (-not $found) {
        Write-Warning "Could not locate DLL matching: $pattern"
    }
}

# 6. Resolve ADB components
$AdbFiles = @("adb.exe", "AdbWinApi.dll", "AdbWinUsbApi.dll")
$AdbFound = $true

foreach ($f in $AdbFiles) {
    if (Test-Path "$RepoRoot\bin\$f") {
        Copy-Item "$RepoRoot\bin\$f" "$DistDir\$f" -Force
        Write-Host "[+] Copied ADB component: $f"
    } elseif ($env:ANDROID_HOME -and (Test-Path "$env:ANDROID_HOME\platform-tools\$f")) {
        Copy-Item "$env:ANDROID_HOME\platform-tools\$f" "$DistDir\$f" -Force
        Write-Host "[+] Copied ADB component (from ANDROID_HOME): $f"
    } elseif ($env:LOCALAPPDATA -and (Test-Path "$env:LOCALAPPDATA\Android\Sdk\platform-tools\$f")) {
        Copy-Item "$env:LOCALAPPDATA\Android\Sdk\platform-tools\$f" "$DistDir\$f" -Force
        Write-Host "[+] Copied ADB component (from Android SDK): $f"
    } else {
        $AdbFound = $false
    }
}

if (-not $AdbFound) {
    Write-Host "[*] Downloading official Android Platform-Tools from Google..." -ForegroundColor Yellow
    $zipTemp = "$env:TEMP\platform-tools.zip"
    $url = "https://dl.google.com/android/repository/platform-tools_r35.0.1-windows.zip"
    (New-Object System.Net.WebClient).DownloadFile($url, $zipTemp)
    
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($zipTemp)
    foreach ($entry in $zip.Entries) {
        foreach ($af in $AdbFiles) {
            if ($entry.FullName -eq "platform-tools/$af") {
                $dest = "$DistDir\$af"
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dest, $true)
                Write-Host "[+] Extracted from official zip: $af"
            }
        }
    }
    $zip.Dispose()
    Remove-Item $zipTemp -Force -ErrorAction SilentlyContinue
}

# 7. Compress into standalone ZIP package
Write-Host "[+] Creating ZIP archive: $ZipFile..."
if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force }
[System.IO.Compression.ZipFile]::CreateFromDirectory($DistDir, $ZipFile)

# 8. Compute SHA-256 Checksum
$hash = (Get-FileHash -Path $ZipFile -Algorithm SHA256).Hash.ToLower()
$zipName = [System.IO.Path]::GetFileName($ZipFile)
$shaContent = "$hash *$zipName"
Set-Content -Path $ShaFile -Value $shaContent -Encoding ASCII
Write-Host "[+] Generated SHA256: $hash" -ForegroundColor Green

Write-Host "========================================================" -ForegroundColor Green
Write-Host "    PACKAGE CREATED SUCCESSFULLY!                      " -ForegroundColor Green
Write-Host "    Zip:      $ZipFile"
Write-Host "    Checksum: $ShaFile ($hash)"
Write-Host "========================================================" -ForegroundColor Green
