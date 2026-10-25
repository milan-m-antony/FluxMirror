<#
.SYNOPSIS
    FluxMirror Title Bar Camera Screenshot Button & Clipboard Toast
.DESCRIPTION
    1. Enforces single-instance execution via a system-wide Mutex (zero duplicate overlays).
    2. Binds lifetime directly to the FluxMirror/scrcpy process (-MirrorPid).
    3. Dynamically queries native Minimize button screen bounds via WM_GETTITLEBARINFOEX
       and places the camera screenshot button immediately to its left with zero overlap.
    4. Continuously enforces Maximize button removal and black Flux logo application.
    5. Hides overlay when mirror window is minimized/hidden, and exits when mirror closes.
    6. When clicked: captures high-res screenshot to clipboard and displays "Copied to clipboard" toast.
#>

[CmdletBinding()]
param (
    [switch]$Snap,
    [string]$DeviceId,
    [int]$MirrorPid
)

$rootDir = (Resolve-Path "$PSScriptRoot\..").Path
$adb = "C:\Android\Sdk\platform-tools\adb.exe"
if (-not (Test-Path $adb)) {
    $adb = (Get-Command adb -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    if (-not $adb) { $adb = "adb.exe" }
}

# --- DIAGNOSTIC LOGGING ---
$logDir = "$rootDir\logs"
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir -Force | Out-Null }
$logFile = "$logDir\dock.log"

function Log-Dock([string]$msg) {
    try {
        $timestamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss.fff")
        $line = "[$timestamp] [PID $PID] $msg"
        Add-Content -Path $logFile -Value $line -ErrorAction SilentlyContinue
    } catch {}
}

# --- 1. SINGLE-INSTANCE MUTEX ENFORCEMENT ---
$mutexName = "Local\FluxMirror_Screenshot_Dock_Mutex"
$createdNew = $false
$hasMutex = $false
$dockMutex = $null

try {
    $dockMutex = New-Object System.Threading.Mutex($true, $mutexName, [ref]$createdNew)
    if ($createdNew) {
        $hasMutex = $true
    } else {
        try {
            $hasMutex = $dockMutex.WaitOne(0, $false)
        } catch [System.Threading.AbandonedMutexException] {
            $hasMutex = $true
        }
    }
} catch {
    $hasMutex = $false
}

if (-not $Snap -and -not $hasMutex) {
    Log-Dock "[SingleInstance] Another dock instance is already active. Exiting duplicate process."
    exit 0
}

Log-Dock "Screenshot dock initialized. MirrorPid=$MirrorPid, SingleInstanceMutexAcquired=$hasMutex"

function Cleanup-And-Exit {
    Log-Dock "[Shutdown] Releasing mutex and terminating screenshot dock."
    if ($dockMutex -and $hasMutex) {
        try {
            $dockMutex.ReleaseMutex()
            $dockMutex.Dispose()
        } catch {}
    }
    try {
        [System.Windows.Application]::Current.Shutdown()
    } catch {}
    exit 0
}

function Get-TargetDevice {
    if ($DeviceId) { return $DeviceId }
    $devicesOutput = & $adb devices
    $deviceLines = @($devicesOutput | Where-Object { $_ -match "\s+device$" })
    if ($deviceLines.Count -gt 0) {
        return ($deviceLines[0] -split "\s+")[0].Trim()
    }
    return $null
}

Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Copy-MobileScreenshotToClipboard {
    $dev = Get-TargetDevice
    if (-not $dev) {
        Log-Dock "[Screenshot] Error: No target device found via ADB."
        return $false
    }

    $tempFile = [System.IO.Path]::GetTempFileName() + ".png"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $adb
    $psi.Arguments = "-s $dev exec-out screencap -p"
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.CreateNoWindow = $true

    try {
        $proc = [System.Diagnostics.Process]::Start($psi)
        $fs = [System.IO.File]::Create($tempFile)
        $proc.StandardOutput.BaseStream.CopyTo($fs)
        $fs.Close()
        $proc.WaitForExit()

        if ((Test-Path $tempFile) -and (Get-Item $tempFile).Length -gt 1000) {
            $bytes = [System.IO.File]::ReadAllBytes($tempFile)
            Remove-Item $tempFile -Force -ErrorAction SilentlyContinue

            $ms = New-Object System.IO.MemoryStream(,$bytes)
            $img = [System.Drawing.Image]::FromStream($ms)
            [System.Windows.Forms.Clipboard]::SetImage($img)
            $img.Dispose()
            $ms.Dispose()

            [System.Media.SystemSounds]::Asterisk.Play()
            Log-Dock "[Screenshot] Captured mobile screen from $dev and copied to clipboard successfully."
            return $true
        } else {
            Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
            Log-Dock "[Screenshot] Error: Captured file was empty or too small."
            return $false
        }
    } catch {
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
        Log-Dock "[Screenshot] Exception during capture: $($_.Exception.Message)"
        return $false
    }
}

if ($Snap) {
    $ok = Copy-MobileScreenshotToClipboard
    if ($ok) {
        Write-Host "[+] Screenshot copied to clipboard! Press Ctrl+V in chat." -ForegroundColor Green
        exit 0
    } else {
        Write-Host "[x] Failed to capture mobile screen." -ForegroundColor Red
        exit 1
    }
}

# --- WIN32 HELPER FOR WINDOW CUSTOMIZATION & DYNAMIC CAPTION BUTTON ANCHORING ---
Add-Type @"
  using System;
  using System.Runtime.InteropServices;
  using System.Text;

  public struct RECT {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct TITLEBARINFOEX {
    public int cbSize;
    public RECT rcTitleBar;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
    public int[] rgstate;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
    public RECT[] rgrect;
  }

  public class WindowSyncResult {
    public bool ShouldExit;
    public bool IsVisible;
    public double TargetX;
    public double TargetY;
    public string LogMessage;
  }

  public class Win32DockHelper {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder strText, int maxCount);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder strClass, int maxCount);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLong(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessage(IntPtr hWnd, int Msg, IntPtr wParam, ref TITLEBARINFOEX lParam);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr LoadImage(IntPtr hInst, string lpszName, uint uType, int cxDesired, int cyDesired, uint fuLoad);

    [DllImport("user32.dll")]
    public static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

    [DllImport("user32.dll")]
    public static extern bool DeleteMenu(IntPtr hMenu, uint uPosition, uint uFlags);

    public const int GWL_STYLE = -16;
    public const int GWL_EXSTYLE = -20;
    public const int WS_MAXIMIZEBOX = 0x00010000;
    public const int WS_EX_NOACTIVATE = 0x08000000;
    public const int WS_EX_TOOLWINDOW = 0x00000080;

    public const uint SWP_NOSIZE = 0x0001;
    public const uint SWP_NOMOVE = 0x0002;
    public const uint SWP_NOZORDER = 0x0004;
    public const uint SWP_FRAMECHANGED = 0x0020;

    public const uint SC_MAXIMIZE = 0xF030;
    public const uint MF_BYCOMMAND = 0x00000000;
    public const uint WM_SETICON = 0x0080;
    public const int WM_GETTITLEBARINFOEX = 0x033F;
    public const int ICON_SMALL = 0;
    public const int ICON_BIG = 1;

    // Persistent static state across timer ticks
    public static IntPtr TargetHwnd = IntPtr.Zero;
    public static bool HasDiscovered = false;
    public static int MissedCount = 0;
    public static int SearchTimeoutTicks = 0;
    public static bool LastVisibleState = false;
    public static bool MaximizeDisabled = false;
    public static bool IconApplied = false;

    // Reset state
    public static void Reset() {
      TargetHwnd = IntPtr.Zero;
      HasDiscovered = false;
      MissedCount = 0;
      SearchTimeoutTicks = 0;
      LastVisibleState = false;
      MaximizeDisabled = false;
      IconApplied = false;
    }

    // Finds the active mirror window
    public static IntPtr FindMirrorWindow(uint targetPid) {
      IntPtr candidate = IntPtr.Zero;
      EnumWindows((hWnd, lParam) => {
        if (!IsWindowVisible(hWnd)) return true;
        uint pid;
        GetWindowThreadProcessId(hWnd, out pid);
        if (targetPid > 0 && pid != targetPid) return true;

        StringBuilder title = new StringBuilder(256);
        GetWindowText(hWnd, title, 256);
        StringBuilder cls = new StringBuilder(256);
        GetClassName(hWnd, cls, 256);
        string t = title.ToString();
        string c = cls.ToString();

        // Ignore tooltips, IME, and helper windows
        if (c.Contains("Tooltip") || c.Contains("Indicator") || c.Contains("IME")) return true;

        // SDL_app is the scrcpy window, or window title matches FluxMirror / Flux
        if (c == "SDL_app" || t.StartsWith("FluxMirror") || t.StartsWith("Flux Mirror") || t.StartsWith("Flux Mobile Screen")) {
          RECT r;
          if (GetWindowRect(hWnd, out r)) {
            int w = r.Right - r.Left;
            int h = r.Bottom - r.Top;
            if (w >= 100 && h >= 100) {
              candidate = hWnd;
              return false; // Found
            }
          }
        }
        return true;
      }, IntPtr.Zero);
      return candidate;
    }

    // Removes maximize button
    public static bool DisableMaximize(IntPtr hWnd) {
      if (hWnd == IntPtr.Zero || !IsWindow(hWnd)) return false;
      int style = GetWindowLong(hWnd, GWL_STYLE);
      if ((style & WS_MAXIMIZEBOX) != 0) {
        style &= ~WS_MAXIMIZEBOX;
        SetWindowLong(hWnd, GWL_STYLE, style);
        SetWindowPos(hWnd, IntPtr.Zero, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        IntPtr hMenu = GetSystemMenu(hWnd, false);
        if (hMenu != IntPtr.Zero) {
          DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
        }
        return true;
      }
      return false;
    }

    // Queries native minimize button screen rect
    public static bool GetMinimizeRect(IntPtr hWnd, out RECT minRect) {
      minRect = new RECT();
      if (hWnd == IntPtr.Zero || !IsWindow(hWnd)) return false;
      try {
        TITLEBARINFOEX tbi = new TITLEBARINFOEX();
        tbi.cbSize = Marshal.SizeOf(typeof(TITLEBARINFOEX));
        tbi.rgrect = new RECT[6];
        tbi.rgstate = new int[6];
        SendMessage(hWnd, WM_GETTITLEBARINFOEX, IntPtr.Zero, ref tbi);
        if (tbi.rgrect != null && tbi.rgrect.Length > 2) {
          RECT r = tbi.rgrect[2];
          // Bit 0x00008000 is STATE_SYSTEM_INVISIBLE
          if ((tbi.rgstate[2] & 0x00008000) == 0 && r.Right > r.Left && r.Bottom > r.Top) {
            minRect = r;
            return true;
          }
        }
      } catch {}
      return false;
    }

    public static void SetBlackIcon(IntPtr hWnd, string iconPath) {
      if (hWnd == IntPtr.Zero || string.IsNullOrEmpty(iconPath) || !IsWindow(hWnd)) return;
      try {
        IntPtr hSmall = LoadImage(IntPtr.Zero, iconPath, 1, 16, 16, 0x0010);
        if (hSmall != IntPtr.Zero) {
          SendMessage(hWnd, WM_SETICON, (IntPtr)ICON_SMALL, hSmall);
        }
        IntPtr hBig = LoadImage(IntPtr.Zero, iconPath, 1, 32, 32, 0x0010);
        if (hBig != IntPtr.Zero) {
          SendMessage(hWnd, WM_SETICON, (IntPtr)ICON_BIG, hBig);
        }
      } catch {}
    }

    // Synchronizes and validates mirror window state, returning exact overlay positions
    public static WindowSyncResult SyncWindow(uint targetPid, double camWidth, double camHeight, string iconPath) {
      WindowSyncResult res = new WindowSyncResult();
      res.ShouldExit = false;
      res.IsVisible = false;
      res.TargetX = -10000;
      res.TargetY = -10000;
      res.LogMessage = null;

      // 1. Search for mirror window if needed
      if (TargetHwnd == IntPtr.Zero || !IsWindow(TargetHwnd)) {
        IntPtr found = FindMirrorWindow(targetPid);
        if (found != IntPtr.Zero) {
          TargetHwnd = found;
          HasDiscovered = true;
          MissedCount = 0;
          SearchTimeoutTicks = 0;
          res.LogMessage = string.Format("[WindowDetection] Found mirror window HWND {0} (PID {1})", TargetHwnd, targetPid);
        } else {
          if (HasDiscovered) {
            MissedCount++;
            if (MissedCount > 25) { // 250ms absent -> shut down
              res.ShouldExit = true;
              res.LogMessage = "[Cleanup] Mirror window closed or destroyed. Initiating dock shutdown.";
              return res;
            }
          } else {
            SearchTimeoutTicks++;
            if (SearchTimeoutTicks > 1500) { // 15 seconds absent -> shut down
              res.ShouldExit = true;
              res.LogMessage = "[Cleanup] Mirror window did not appear within 15 seconds. Shutting down dock.";
              return res;
            }
          }
          return res;
        }
      }

      // 2. Validate window existence, visibility, iconic state
      if (!IsWindow(TargetHwnd) || !IsWindowVisible(TargetHwnd) || IsIconic(TargetHwnd)) {
        if (LastVisibleState) {
          LastVisibleState = false;
          res.LogMessage = "[Visibility] Mirror window minimized or hidden. Overlay hidden.";
        }
        return res;
      }

      // 3. Validate window bounds
      RECT winRect;
      if (!GetWindowRect(TargetHwnd, out winRect)) {
        if (LastVisibleState) {
          LastVisibleState = false;
          res.LogMessage = "[Visibility] GetWindowRect failed. Overlay hidden.";
        }
        return res;
      }

      int w = winRect.Right - winRect.Left;
      int h = winRect.Bottom - winRect.Top;
      if (w < 100 || h < 100 || winRect.Left < -3000 || winRect.Top < -3000) {
        if (LastVisibleState) {
          LastVisibleState = false;
          res.LogMessage = string.Format("[Visibility] Invalid window bounds ({0}x{1}). Overlay hidden.", w, h);
        }
        return res;
      }

      // 4. Ensure Maximize button is disabled & icon set
      if (!MaximizeDisabled) {
        if (DisableMaximize(TargetHwnd)) {
          MaximizeDisabled = true;
          res.LogMessage = "[UI] Stripped WS_MAXIMIZEBOX and deleted SC_MAXIMIZE from system menu.";
        }
      }
      if (!IconApplied && !string.IsNullOrEmpty(iconPath)) {
        SetBlackIcon(TargetHwnd, iconPath);
        IconApplied = true;
      }

      // 5. Query Minimize button position
      RECT minRect;
      if (GetMinimizeRect(TargetHwnd, out minRect)) {
        res.TargetX = minRect.Left - camWidth - 2.0;
        res.TargetY = minRect.Top + ((minRect.Bottom - minRect.Top - camHeight) / 2.0);
      } else {
        // Fallback (47px Close + 47px Minimize = 94px from Right)
        res.TargetX = winRect.Right - 94.0 - camWidth - 2.0;
        res.TargetY = winRect.Top + 4.0;
      }

      res.IsVisible = true;
      if (!LastVisibleState) {
        LastVisibleState = true;
        res.LogMessage = string.Format("[Visibility] Mirror window active and visible. Camera overlay positioned at ({0:F0}, {1:F0}).", res.TargetX, res.TargetY);
      }

      return res;
    }
  }
"@

# --- 2. TITLE BAR CAMERA ICON BUTTON WINDOW ---
# Starts off-screen (Left="-10000") and transparent (Opacity="0") to prevent any phantom icons on desktop
[xml]$xamlCam = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="FluxCameraTitleBtn"
        Height="26" Width="34"
        WindowStyle="None"
        AllowsTransparency="True"
        Background="Transparent"
        Topmost="True"
        ShowInTaskbar="False"
        ResizeMode="NoResize"
        Opacity="0"
        Left="-10000" Top="-10000">
    <Button Name="BtnCamera"
            Background="Transparent"
            BorderThickness="0"
            Cursor="Hand"
            ToolTip="Take Screenshot &amp; Copy to Clipboard"
            HorizontalAlignment="Stretch"
            VerticalAlignment="Stretch">
        <Button.Template>
            <ControlTemplate TargetType="Button">
                <Border Name="bdr" Background="{TemplateBinding Background}" CornerRadius="4">
                    <Viewbox Width="17" Height="15" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Canvas Width="24" Height="24">
                            <!-- Clean Camera Outline -->
                            <Path Data="M 4,7 L 7,7 L 8.8,4.4 C 9.1,4.1 9.5,4 10,4 L 14,4 C 14.5,4 14.9,4.1 15.2,4.4 L 17,7 L 20,7 C 21.1,7 22,7.9 22,9 L 22,18 C 22,19.1 21.1,20 20,20 L 4,20 C 2.9,20 2,19.1 2,18 L 2,9 C 2,7.9 2.9,7 4,7 Z"
                                  Stroke="#FFFFFF"
                                  StrokeThickness="1.6"
                                  StrokeLineJoin="Round"
                                  StrokeStartLineCap="Round"
                                  StrokeEndLineCap="Round"
                                  Fill="Transparent"/>
                            <!-- Inner Lens Circle -->
                            <Ellipse Canvas.Left="8" Canvas.Top="9" Width="8" Height="8"
                                     Stroke="#FFFFFF" StrokeThickness="1.6" Fill="Transparent"/>
                            <!-- Flash Dot -->
                            <Ellipse Canvas.Left="17" Canvas.Top="8.5" Width="1.6" Height="1.6"
                                     Fill="#FFFFFF"/>
                        </Canvas>
                    </Viewbox>
                </Border>
                <ControlTemplate.Triggers>
                    <Trigger Property="IsMouseOver" Value="True">
                        <Setter TargetName="bdr" Property="Background" Value="#2BFFFFFF"/>
                    </Trigger>
                    <Trigger Property="IsPressed" Value="True">
                        <Setter TargetName="bdr" Property="Background" Value="#45FFFFFF"/>
                    </Trigger>
                </ControlTemplate.Triggers>
            </ControlTemplate>
        </Button.Template>
    </Button>
</Window>
"@

# --- 3. "COPIED TO CLIPBOARD" TOAST NOTIFICATION WINDOW ---
[xml]$xamlToast = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="FluxToast"
        Height="42" Width="205"
        WindowStyle="None"
        AllowsTransparency="True"
        Background="Transparent"
        Topmost="True"
        ShowInTaskbar="False"
        ResizeMode="NoResize"
        Opacity="0"
        Left="-10000" Top="-10000">
    <Border Background="#1E293B"
            BorderBrush="#FF6B00"
            BorderThickness="1.5"
            CornerRadius="21"
            Padding="14,0"
            Margin="2">
        <Border.Effect>
            <DropShadowEffect BlurRadius="12" Color="#000000" Opacity="0.6" ShadowDepth="3"/>
        </Border.Effect>
        <StackPanel Orientation="Horizontal" HorizontalAlignment="Center" VerticalAlignment="Center">
            <!-- Emerald Checkmark Badge -->
            <Border Width="18" Height="18" CornerRadius="9" Background="#10B981" Margin="0,0,8,0">
                <TextBlock Text="✓" Foreground="#FFFFFF" FontWeight="Bold" FontSize="11"
                           HorizontalAlignment="Center" VerticalAlignment="Center" Margin="0,-1,0,0"/>
            </Border>
            <!-- Toast Text -->
            <TextBlock Text="Copied to clipboard" Foreground="#FFFFFF"
                       FontFamily="Segoe UI" FontWeight="SemiBold" FontSize="12"
                       VerticalAlignment="Center"/>
        </StackPanel>
    </Border>
</Window>
"@

$winCam = [System.Windows.Markup.XamlReader]::Load((New-Object System.Xml.XmlNodeReader $xamlCam))
$winToast = [System.Windows.Markup.XamlReader]::Load((New-Object System.Xml.XmlNodeReader $xamlToast))

$btnCam = $winCam.FindName("BtnCamera")

# Toast dismiss timer
$toastDismissTimer = New-Object System.Windows.Threading.DispatcherTimer
$toastDismissTimer.Interval = [TimeSpan]::FromSeconds(1.8)
$toastDismissTimer.Add_Tick({
    $winToast.Opacity = 0.0
    $winToast.Left = -10000
    $toastDismissTimer.Stop()
})

$script:dpiScaleX = 1.0
$script:dpiScaleY = 1.0

# Make both windows non-activating so clicking doesn't steal focus from mirror window
$winCam.Add_SourceInitialized({
    $helper = New-Object System.Windows.Interop.WindowInteropHelper($winCam)
    $h = $helper.Handle
    $ex = [Win32DockHelper]::GetWindowLong($h, [Win32DockHelper]::GWL_EXSTYLE)
    [Win32DockHelper]::SetWindowLong($h, [Win32DockHelper]::GWL_EXSTYLE, $ex -bor [Win32DockHelper]::WS_EX_NOACTIVATE -bor [Win32DockHelper]::WS_EX_TOOLWINDOW)

    $source = [System.Windows.PresentationSource]::FromVisual($winCam)
    if ($source -and $source.CompositionTarget) {
        $script:dpiScaleX = $source.CompositionTarget.TransformToDevice.M11
        $script:dpiScaleY = $source.CompositionTarget.TransformToDevice.M22
    }
})

$winToast.Add_SourceInitialized({
    $helper = New-Object System.Windows.Interop.WindowInteropHelper($winToast)
    $h = $helper.Handle
    $ex = [Win32DockHelper]::GetWindowLong($h, [Win32DockHelper]::GWL_EXSTYLE)
    [Win32DockHelper]::SetWindowLong($h, [Win32DockHelper]::GWL_EXSTYLE, $ex -bor [Win32DockHelper]::WS_EX_NOACTIVATE -bor [Win32DockHelper]::WS_EX_TOOLWINDOW)
})

# Click Event on Title Bar Camera Button: Snap Screenshot & Show Toast
$btnCam.Add_Click({
    $ok = Copy-MobileScreenshotToClipboard
    if ($ok) {
        $rect = New-Object RECT
        if ([Win32DockHelper]::TargetHwnd -ne [IntPtr]::Zero -and [Win32DockHelper]::GetWindowRect([Win32DockHelper]::TargetHwnd, [ref]$rect)) {
            $mirrorWidthPhysical = $rect.Right - $rect.Left
            $mirrorWidthDip = $mirrorWidthPhysical / $script:dpiScaleX
            $mirrorLeftDip = $rect.Left / $script:dpiScaleX
            $mirrorTopDip = $rect.Top / $script:dpiScaleY
            $winToast.Left = $mirrorLeftDip + (($mirrorWidthDip - $winToast.Width) / 2)
            $winToast.Top = $mirrorTopDip + 38
        } else {
            $winToast.Left = $winCam.Left - (($winToast.Width - $winCam.Width) / 2)
            $winToast.Top = $winCam.Top + 36
        }
        $winToast.Opacity = 1.0
        $toastDismissTimer.Stop()
        $toastDismissTimer.Start()
    }
})

# --- 4. SYNCHRONIZE CAMERA BUTTON DYNAMICALLY TO MINIMIZE BUTTON AT 100 FPS ---
$iconPath = "$rootDir\resources\icons\flux_black.ico"

$syncTimer = New-Object System.Windows.Threading.DispatcherTimer
$syncTimer.Interval = [TimeSpan]::FromMilliseconds(10) # 100 FPS position sync
$syncTimer.Add_Tick({
    # 0. Check if target parent process has exited
    if ($MirrorPid -gt 0) {
        $parentProc = Get-Process -Id $MirrorPid -ErrorAction SilentlyContinue
        if (-not $parentProc -or $parentProc.HasExited) {
            Log-Dock "[Shutdown] Mirror process (PID $MirrorPid) has exited. Cleaning up dock."
            Cleanup-And-Exit
            return
        }
    }

    # 1. Update mirror window state and compute exact target coordinates
    $camPhysicalWidth = $winCam.Width * $script:dpiScaleX
    $camPhysicalHeight = $winCam.Height * $script:dpiScaleY
    $res = [Win32DockHelper]::SyncWindow([uint32]$MirrorPid, $camPhysicalWidth, $camPhysicalHeight, $iconPath)

    if ($res.LogMessage) {
        Log-Dock $res.LogMessage
    }

    if ($res.ShouldExit) {
        Cleanup-And-Exit
        return
    }

    # 2. Position or hide overlay based on validated window state
    if ($res.IsVisible) {
        $targetDipLeft = $res.TargetX / $script:dpiScaleX
        $targetDipTop = $res.TargetY / $script:dpiScaleY

        if ($winCam.Opacity -lt 1.0) {
            $winCam.Opacity = 1.0
        }

        if ([Math]::Abs($winCam.Left - $targetDipLeft) -gt 0.5 -or [Math]::Abs($winCam.Top - $targetDipTop) -gt 0.5) {
            $winCam.Left = $targetDipLeft
            $winCam.Top = $targetDipTop
        }
    } else {
        # Hide offscreen and transparent when minimized, hidden, or invalid
        if ($winCam.Left -gt -5000) {
            $winCam.Left = -10000
            $winCam.Top = -10000
            $winCam.Opacity = 0.0
            $winToast.Left = -10000
            $winToast.Top = -10000
            $winToast.Opacity = 0.0
        }
    }
})

$winCam.Add_Loaded({
    $syncTimer.Start()
    $winToast.Show()
})

$winCam.Add_Closed({
    Cleanup-And-Exit
})

$winCam.ShowDialog() | Out-Null
