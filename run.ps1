# ==============================================================================
# FluxMirror — Instant Public Launcher
# Downloads, cryptographically verifies, caches, and launches FluxMirror.
# Usage:
#   irm https://raw.githubusercontent.com/milan-m-antony/FluxMirror/main/run.ps1 | iex
# ==============================================================================

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
$ErrorActionPreference = "Stop"

$Repo = "milan-m-antony/FluxMirror"
$InstallDir = "$env:LOCALAPPDATA\FluxMirror"
$TargetExe = "$InstallDir\FluxMirror.exe"
$StartBat = "$InstallDir\start.bat"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "                 FLUXMIRROR PUBLIC LAUNCHER             " -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. Query GitHub Releases for the latest version
$ReleaseInfo = $null
try {
    $apiUrl = "https://api.github.com/repos/$Repo/releases/latest"
    $headers = @{ "User-Agent" = "FluxMirror-Launcher" }
    $ReleaseInfo = Invoke-RestMethod -Uri $apiUrl -Headers $headers -Method Get
} catch {
    Write-Host "[!] Could not fetch latest release info from GitHub (offline or release pending)." -ForegroundColor Yellow
}

$NeedDownload = $true
$VersionTag = "latest"

if ($ReleaseInfo -and $ReleaseInfo.tag_name) {
    $VersionTag = $ReleaseInfo.tag_name
    $VersionFile = "$InstallDir\version.txt"
    if ((Test-Path $TargetExe) -and (Test-Path $VersionFile)) {
        $InstalledVersion = (Get-Content $VersionFile -Raw).Trim()
        if ($InstalledVersion -eq $VersionTag) {
            $NeedDownload = $false
            Write-Host "[+] FluxMirror $VersionTag is up to date in cache." -ForegroundColor Green
        }
    }
} elseif (Test-Path $TargetExe) {
    $NeedDownload = $false
    Write-Host "[+] Using locally cached FluxMirror installation." -ForegroundColor Green
}

# 2. Download and verify if needed
if ($NeedDownload) {
    if (-not $ReleaseInfo -or -not $ReleaseInfo.assets) {
        if (Test-Path $TargetExe) {
            Write-Host "[!] No online release assets found, running existing local installation." -ForegroundColor Yellow
        } else {
            Write-Error "No published release found on GitHub yet. Please download from: https://github.com/$Repo/releases"
            return
        }
    } else {
        $zipAsset = $ReleaseInfo.assets | Where-Object { $_.name -like "*Windows*.zip" } | Select-Object -First 1
        $shaAsset = $ReleaseInfo.assets | Where-Object { $_.name -eq "SHA256SUMS.txt" } | Select-Object -First 1

        if (-not $zipAsset) {
            Write-Error "Could not find a Windows release asset in release $VersionTag."
            return
        }

        $tempDir = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), "FluxMirror_Download_" + [System.Guid]::NewGuid().ToString("N"))
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
        $tempZip = Join-Path $tempDir $zipAsset.name

        try {
            Write-Host "[+] Downloading FluxMirror $VersionTag ($([Math]::Round($zipAsset.size / 1MB, 1)) MB)..." -ForegroundColor Cyan
            (New-Object System.Net.WebClient).DownloadFile($zipAsset.browser_download_url, $tempZip)

            # Cryptographic verification
            if ($shaAsset) {
                Write-Host "[+] Verifying SHA-256 cryptographic checksum..." -ForegroundColor Cyan
                $shaContent = (New-Object System.Net.WebClient).DownloadString($shaAsset.browser_download_url)
                $computedHash = (Get-FileHash -Path $tempZip -Algorithm SHA256).Hash.ToLower()

                $matchFound = $false
                foreach ($line in ($shaContent -split "`n")) {
                    $parts = $line.Trim() -split "\s+"
                    if ($parts.Count -ge 2 -and $parts[0].ToLower() -eq $computedHash) {
                        $matchFound = $true
                        break
                    }
                }

                if (-not $matchFound) {
                    Write-Error "SECURITY ALERT: Checksum verification failed for $($zipAsset.name)! Aborting."
                    Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
                    return
                }
                Write-Host "✓ Checksum verification passed ($computedHash)" -ForegroundColor Green
            } else {
                Write-Host "[!] Notice: SHA256SUMS.txt not present in release assets, skipping verification." -ForegroundColor Yellow
            }

            Write-Host "[+] Installing to $InstallDir..." -ForegroundColor Cyan
            if (Test-Path $InstallDir) {
                Remove-Item -Recurse -Force $InstallDir
            }
            New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null

            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [System.IO.Compression.ZipFile]::ExtractToDirectory($tempZip, $InstallDir)

            Set-Content -Path "$InstallDir\version.txt" -Value $VersionTag -Force
            Write-Host "✓ Installation completed successfully!" -ForegroundColor Green
        } finally {
            Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
        }
    }
}

# 3. Launch FluxMirror
if (Test-Path $StartBat) {
    Write-Host "[+] Starting FluxMirror..." -ForegroundColor Green
    & cmd.exe /c "call `"$StartBat`"" $args
} elseif (Test-Path $TargetExe) {
    Write-Host "[+] Starting FluxMirror executable..." -ForegroundColor Green
    Start-Process -FilePath $TargetExe -ArgumentList "--window-title `"FluxMirror`" --always-on-top --max-fps 120 --video-buffer 0 --stay-awake --print-fps"
} else {
    Write-Error "Could not find runnable FluxMirror in $InstallDir."
}
