<#
.SYNOPSIS
    FluxMirror Title Bar Camera Screenshot Button & Clipboard Toast
.DESCRIPTION
    1. Sets title bar logo to black Flux icon.
    2. Removes Maximize button (leaves only Minimize and Close).
    3. Embeds a native camera screenshot icon directly in the title bar next to the minimize button.
    4. When clicked: captures high-res screenshot to clipboard and displays "Copied to clipboard" toast.
#>

[CmdletBinding()]
param (
    [switch]$Snap,
    [string]$DeviceId
)

Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$rootDir = (Resolve-Path "$PSScriptRoot\..").Path
$adb = "C:\Android\Sdk\platform-tools\adb.exe"
if (-not (Test-Path $adb)) {
    $adb = (Get-Command adb -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    if (-not $adb) { $adb = "adb.exe" }
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

function Copy-MobileScreenshotToClipboard {
    $dev = Get-TargetDevice
    if (-not $dev) { return $false }

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
            return $true
        } else {
            Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
            return $false
        }
    } catch {
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
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

# --- WIN32 HELPER FOR WINDOW CUSTOMIZATION & CAPTION BUTTON EMBEDDING ---
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

  public class Win32TitleBarHelper {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder strText, int maxCount);

    [DllImport("user32.dll")]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder strClass, int maxCount);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hWnd);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLong(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

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
    public const uint SWP_NOACTIVATE = 0x0010;
    public const uint SWP_FRAMECHANGED = 0x0020;
    public const uint SWP_SHOWWINDOW = 0x0040;
    public const uint SWP_HIDEWINDOW = 0x0080;

    public const uint SC_MAXIMIZE = 0xF030;
    public const uint MF_BYCOMMAND = 0x00000000;
    public const uint WM_SETICON = 0x0080;
    public const int ICON_SMALL = 0;
    public const int ICON_BIG = 1;

    // Finds the active FluxMirror / scrcpy window
    public static IntPtr FindMirrorWindow() {
      IntPtr found = IntPtr.Zero;
      EnumWindows((hWnd, lParam) => {
        if (IsWindowVisible(hWnd)) {
          StringBuilder title = new StringBuilder(256);
          GetWindowText(hWnd, title, 256);
          StringBuilder cls = new StringBuilder(256);
          GetClassName(hWnd, cls, 256);
          string t = title.ToString();
          string c = cls.ToString();
          if (t.StartsWith("FluxMirror") || t.StartsWith("Flux Mirror") || t.StartsWith("Flux Mobile Screen") ||
              (c == "SDL_app" && (t.Contains("Flux") || t.Length == 0 || t == "scrcpy"))) {
            found = hWnd;
            return false;
          }
        }
        return true;
      }, IntPtr.Zero);
      return found;
    }

    // Removes Maximize button (keeps only Minimize and Close)
    public static void DisableMaximize(IntPtr hWnd) {
      if (hWnd == IntPtr.Zero) return;
      int style = GetWindowLong(hWnd, GWL_STYLE);
      style &= ~WS_MAXIMIZEBOX;
      SetWindowLong(hWnd, GWL_STYLE, style);
      SetWindowPos(hWnd, IntPtr.Zero, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
      IntPtr hMenu = GetSystemMenu(hWnd, false);
      if (hMenu != IntPtr.Zero) {
        DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
      }
    }

    // Sets window title bar icon to black icon
    public static void SetBlackIcon(IntPtr hWnd, string iconPath) {
      if (hWnd == IntPtr.Zero || string.IsNullOrEmpty(iconPath)) return;
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
  }
"@

# --- 1. TITLE BAR CAMERA ICON BUTTON WINDOW ---
# Starts off-screen (Left="-10000") to avoid flicker until snapped to title bar
[xml]$xamlCam = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="FluxCameraTitleBtn"
        Height="28" Width="36"
        WindowStyle="None"
        AllowsTransparency="True"
        Background="Transparent"
        Topmost="True"
        ShowInTaskbar="False"
        ResizeMode="NoResize"
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
                            <!-- Clean Camera Outline (Matching User Mockup) -->
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

# --- 2. "COPIED TO CLIPBOARD" TOAST NOTIFICATION WINDOW ---
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
    $winToast.Left = -10000
    $toastDismissTimer.Stop()
})

# Make both windows non-activating so clicking doesn't unfocus scrcpy
$winCam.Add_SourceInitialized({
    $helper = New-Object System.Windows.Interop.WindowInteropHelper($winCam)
    $h = $helper.Handle
    $ex = [Win32TitleBarHelper]::GetWindowLong($h, [Win32TitleBarHelper]::GWL_EXSTYLE)
    [Win32TitleBarHelper]::SetWindowLong($h, [Win32TitleBarHelper]::GWL_EXSTYLE, $ex -bor [Win32TitleBarHelper]::WS_EX_NOACTIVATE -bor [Win32TitleBarHelper]::WS_EX_TOOLWINDOW)
})

$winToast.Add_SourceInitialized({
    $helper = New-Object System.Windows.Interop.WindowInteropHelper($winToast)
    $h = $helper.Handle
    $ex = [Win32TitleBarHelper]::GetWindowLong($h, [Win32TitleBarHelper]::GWL_EXSTYLE)
    [Win32TitleBarHelper]::SetWindowLong($h, [Win32TitleBarHelper]::GWL_EXSTYLE, $ex -bor [Win32TitleBarHelper]::WS_EX_NOACTIVATE -bor [Win32TitleBarHelper]::WS_EX_TOOLWINDOW)
})

# Click Event on Title Bar Camera Button: Snap Screenshot & Show Toast
$btnCam.Add_Click({
    $ok = Copy-MobileScreenshotToClipboard
    if ($ok) {
        $rect = New-Object RECT
        if ($targetHwnd -ne [IntPtr]::Zero -and [Win32TitleBarHelper]::GetWindowRect($targetHwnd, [ref]$rect)) {
            $mirrorWidth = $rect.Right - $rect.Left
            $winToast.Left = $rect.Left + (($mirrorWidth - $winToast.Width) / 2)
            $winToast.Top = $rect.Top + 38
        } else {
            $winToast.Left = $winCam.Left - (($winToast.Width - $winCam.Width) / 2)
            $winToast.Top = $winCam.Top + 36
        }
        
        $toastDismissTimer.Stop()
        $toastDismissTimer.Start()
    }
})

# --- SYNCHRONIZE CAMERA BUTTON IN TITLE BAR AT 60 FPS ---
$targetHwnd = [IntPtr]::Zero
$iconPath = "$rootDir\resources\icons\flux_black.ico"
$isDecorated = $false
$hasDiscoveredWindow = $false
$missedCount = 0

$syncTimer = New-Object System.Windows.Threading.DispatcherTimer
$syncTimer.Interval = [TimeSpan]::FromMilliseconds(12) # ~80 FPS position sync
$syncTimer.Add_Tick({
    # 1. Search for window if not yet found
    if ($targetHwnd -eq [IntPtr]::Zero -or -not [Win32TitleBarHelper]::IsWindow($targetHwnd)) {
        $targetHwnd = [Win32TitleBarHelper]::FindMirrorWindow()
        if ($targetHwnd -ne [IntPtr]::Zero) {
            $hasDiscoveredWindow = $true
            $isDecorated = $false
            $missedCount = 0
        } else {
            if ($hasDiscoveredWindow) {
                $missedCount++
                if ($missedCount -gt 25) { # Close only if window was really closed for > 300ms
                    $winToast.Close()
                    $winCam.Close()
                }
            }
            return
        }
    }

    # 2. Check if mirror window is minimized
    if ([Win32TitleBarHelper]::IsIconic($targetHwnd) -or -not [Win32TitleBarHelper]::IsWindowVisible($targetHwnd)) {
        if ($winCam.Left -gt -5000) {
            $winCam.Left = -10000
            $winToast.Left = -10000
        }
        return
    }

    # 3. Apply Title Bar Customizations once: Remove Maximize box & Set Black Logo
    if (-not $isDecorated) {
        [Win32TitleBarHelper]::DisableMaximize($targetHwnd)
        if (Test-Path $iconPath) {
            [Win32TitleBarHelper]::SetBlackIcon($targetHwnd, $iconPath)
        }
        $isDecorated = $true
    }

    # 4. Position Camera Button Directly Inside the Title Bar (Left of Minimize Button)
    $rect = New-Object RECT
    if ([Win32TitleBarHelper]::GetWindowRect($targetHwnd, [ref]$rect)) {
        # Anchor camera button directly to the left of the minimize button
        $newLeft = $rect.Right - 176
        $newTop = $rect.Top + 2

        if ([Math]::Abs($winCam.Left - $newLeft) -gt 0.5 -or [Math]::Abs($winCam.Top - $newTop) -gt 0.5) {
            $winCam.Left = $newLeft
            $winCam.Top = $newTop
        }
    }
})

$winCam.Add_Loaded({
    $syncTimer.Start()
    $winToast.Show()
})

$winCam.ShowDialog() | Out-Null
