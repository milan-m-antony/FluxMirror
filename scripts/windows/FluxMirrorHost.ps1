<#
.SYNOPSIS
    FluxMirror Native Host Window with Custom Title Bar
.DESCRIPTION
    Replaces standard OS title bar with a customized Flux title bar:
    FluxMirror title | Camera | Minimize | Close
    Embeds borderless scrcpy (SDL3) via SetParent, preserving full Direct3D11 GPU
    rendering performance, mouse/touch input, keyboard shortcuts, and aspect ratio.
    Captures high-res Android screen to Windows clipboard on camera button click.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory = $true)]
    [string]$Serial,

    [string]$Model = "Android Device",
    [int]$Fps = 60,
    [string]$NativeBin,
    [string]$Adb = "adb.exe",
    [switch]$AlwaysOnTop,
    [switch]$Wifi
)

Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

  public class FluxNativeHost {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder strClass, int maxCount);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLong(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr SetFocus(IntPtr hWnd);

    public const int GWL_STYLE = -16;
    public const int WS_CHILD = 0x40000000;
    public const int WS_POPUP = unchecked((int)0x80000000);
    public const uint WM_CLOSE = 0x0010;

    public static IntPtr FindScrcpyWindow(uint targetPid) {
      IntPtr found = IntPtr.Zero;
      EnumWindows((hWnd, lParam) => {
        if (!IsWindowVisible(hWnd)) return true;
        uint pid;
        GetWindowThreadProcessId(hWnd, out pid);
        if (pid == targetPid) {
          StringBuilder cls = new StringBuilder(256);
          GetClassName(hWnd, cls, 256);
          if (cls.ToString() == "SDL_app") {
            found = hWnd;
            return false;
          }
        }
        return true;
      }, IntPtr.Zero);
      return found;
    }
  }
"@

$scrcpyDir = [System.IO.Path]::GetDirectoryName($NativeBin)

# Launch borderless scrcpy
$scrcpyArgs = @(
    "-s $Serial",
    "--window-borderless",
    "--window-title `"FluxMirrorInternal`"",
    "--max-fps $Fps",
    "--video-buffer 0",
    "--video-codec h264",
    "--video-bit-rate 16M",
    "--stay-awake",
    "--print-fps"
)

if ($Wifi) { $scrcpyArgs += "--tcpip" }

Write-Host "[+] Launching hardware-accelerated mirroring engine..." -ForegroundColor Cyan
$scrcpyProc = Start-Process -FilePath $NativeBin -WorkingDirectory $scrcpyDir -ArgumentList ($scrcpyArgs -join " ") -PassThru

# Wait for SDL window to be created
$sdlHwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 40; $i++) {
    Start-Sleep -Milliseconds 100
    if ($scrcpyProc.HasExited) {
        Write-Host "[x] Error: Mirroring process terminated unexpectedly." -ForegroundColor Red
        exit 1
    }
    $sdlHwnd = [FluxNativeHost]::FindScrcpyWindow($scrcpyProc.Id)
    if ($sdlHwnd -ne [IntPtr]::Zero) { break }
}

if ($sdlHwnd -eq [IntPtr]::Zero) {
    Write-Host "[x] Error: Could not locate scrcpy display window." -ForegroundColor Red
    if ($scrcpyProc -and -not $scrcpyProc.HasExited) { Stop-Process -Id $scrcpyProc.Id -Force }
    exit 1
}

# Query device display size to calculate ideal window aspect ratio
$idealWidth = 360
$idealHeight = 780
try {
    $wmSize = & $Adb -s $Serial shell "wm size"
    if ($wmSize -match "Physical size:\s*(\d+)x(\d+)") {
        $devW = [int]$Matches[1]
        $devH = [int]$Matches[2]
        if ($devW -gt 0 -and $devH -gt 0) {
            $ratio = $devW / $devH
            $idealHeight = 760
            $idealWidth = [int]($idealHeight * $ratio)
            if ($idealWidth -lt 300) { $idealWidth = 300 }
        }
    }
} catch {}

# Title bar text
$fpsBadge = "$Fps FPS"
$titleString = "FluxMirror - $Model ($fpsBadge)"
$subTitle = " - $Model ($fpsBadge)"

# Construct host window with custom title bar & WindowChrome
[xml]$xaml = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        xmlns:shell="clr-namespace:System.Windows.Shell;assembly=PresentationFramework"
        Title="$titleString"
        Height="$($idealHeight + 34)" Width="$idealWidth"
        MinWidth="280" MinHeight="400"
        Background="#0F172A"
        WindowStartupLocation="CenterScreen"
        Topmost="$($AlwaysOnTop.IsPresent)">
    <shell:WindowChrome.WindowChrome>
        <shell:WindowChrome CaptionHeight="34"
                            ResizeBorderThickness="6"
                            CornerRadius="0"
                            GlassFrameThickness="1"
                            UseAeroCaptionButtons="False"/>
    </shell:WindowChrome.WindowChrome>
    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition Height="34"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>
        
        <!-- Custom Flux Title Bar -->
        <Border Grid.Row="0" Name="TitleBarBorder" Background="#0F172A" BorderBrush="#1E293B" BorderThickness="0,0,0,1">
            <Grid>
                <!-- Title & Branding on Left -->
                <StackPanel Orientation="Horizontal" HorizontalAlignment="Left" VerticalAlignment="Center" Margin="10,0,0,0">
                    <TextBlock Text="● " Foreground="#FF6B00" FontWeight="Bold" FontSize="14"/>
                    <TextBlock Text="FluxMirror" Foreground="#F8FAFC" FontWeight="Bold" FontSize="12"/>
                    <TextBlock Text="$subTitle" Foreground="#94A3B8" FontSize="11" Margin="4,1,0,0"/>
                </StackPanel>

                <!-- Caption Controls on Right: Camera | Minimize | Close -->
                <StackPanel Orientation="Horizontal" HorizontalAlignment="Right" VerticalAlignment="Center">
                    <!-- 1. Camera Screenshot Button (Replaces Maximize) -->
                    <Button Name="BtnCam" Width="38" Height="34" Background="Transparent" BorderThickness="0" Cursor="Hand"
                            shell:WindowChrome.IsHitTestVisibleInChrome="True" ToolTip="Take Screenshot &amp; Copy to Clipboard">
                        <Button.Template>
                            <ControlTemplate TargetType="Button">
                                <Border Name="bdr" Background="{TemplateBinding Background}">
                                    <Viewbox Width="17" Height="15" HorizontalAlignment="Center" VerticalAlignment="Center">
                                        <Canvas Width="24" Height="24">
                                            <Path Data="M 4,7 L 7,7 L 8.8,4.4 C 9.1,4.1 9.5,4 10,4 L 14,4 C 14.5,4 14.9,4.1 15.2,4.4 L 17,7 L 20,7 C 21.1,7 22,7.9 22,9 L 22,18 C 22,19.1 21.1,20 20,20 L 4,20 C 2.9,20 2,19.1 2,18 L 2,9 C 2,7.9 2.9,7 4,7 Z"
                                                  Stroke="#FFFFFF" StrokeThickness="1.6" StrokeLineJoin="Round"
                                                  StrokeStartLineCap="Round" StrokeEndLineCap="Round" Fill="Transparent"/>
                                            <Ellipse Canvas.Left="8" Canvas.Top="9" Width="8" Height="8"
                                                     Stroke="#FFFFFF" StrokeThickness="1.6" Fill="Transparent"/>
                                            <Ellipse Canvas.Left="17" Canvas.Top="8.5" Width="1.6" Height="1.6" Fill="#FFFFFF"/>
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

                    <!-- 2. Minimize Button -->
                    <Button Name="BtnMin" Width="38" Height="34" Background="Transparent" BorderThickness="0" Cursor="Hand"
                            shell:WindowChrome.IsHitTestVisibleInChrome="True" ToolTip="Minimize">
                        <Button.Template>
                            <ControlTemplate TargetType="Button">
                                <Border Name="bdr" Background="{TemplateBinding Background}">
                                    <TextBlock Text="─" Foreground="#94A3B8" FontSize="13" HorizontalAlignment="Center" VerticalAlignment="Center"/>
                                </Border>
                                <ControlTemplate.Triggers>
                                    <Trigger Property="IsMouseOver" Value="True">
                                        <Setter TargetName="bdr" Property="Background" Value="#2BFFFFFF"/>
                                    </Trigger>
                                </ControlTemplate.Triggers>
                            </ControlTemplate>
                        </Button.Template>
                    </Button>

                    <!-- 3. Close Button -->
                    <Button Name="BtnClose" Width="38" Height="34" Background="Transparent" BorderThickness="0" Cursor="Hand"
                            shell:WindowChrome.IsHitTestVisibleInChrome="True" ToolTip="Close">
                        <Button.Template>
                            <ControlTemplate TargetType="Button">
                                <Border Name="bdr" Background="{TemplateBinding Background}">
                                    <TextBlock Name="txt" Text="✕" Foreground="#94A3B8" FontSize="11" HorizontalAlignment="Center" VerticalAlignment="Center"/>
                                </Border>
                                <ControlTemplate.Triggers>
                                    <Trigger Property="IsMouseOver" Value="True">
                                        <Setter TargetName="bdr" Property="Background" Value="#E81123"/>
                                        <Setter TargetName="txt" Property="Foreground" Value="#FFFFFF"/>
                                    </Trigger>
                                </ControlTemplate.Triggers>
                            </ControlTemplate>
                        </Button.Template>
                    </Button>
                </StackPanel>
            </Grid>
        </Border>
        
        <!-- Video Viewport Container Area -->
        <Border Grid.Row="1" Name="ViewportBorder" Background="#000000"/>
        
        <!-- Toast Notification Popup (Overlay within Window) -->
        <Border Grid.Row="1" Name="ToastBorder"
                HorizontalAlignment="Center" VerticalAlignment="Top"
                Margin="0,12,0,0"
                Background="#1E293B" BorderBrush="#FF6B00" BorderThickness="1.5"
                CornerRadius="18" Padding="14,6"
                Opacity="0" IsHitTestVisible="False">
            <Border.Effect>
                <DropShadowEffect BlurRadius="12" Color="#000000" Opacity="0.6" ShadowDepth="3"/>
            </Border.Effect>
            <StackPanel Orientation="Horizontal" VerticalAlignment="Center">
                <Border Width="16" Height="16" CornerRadius="8" Background="#10B981" Margin="0,0,8,0">
                    <TextBlock Text="✓" Foreground="#FFFFFF" FontWeight="Bold" FontSize="10"
                               HorizontalAlignment="Center" VerticalAlignment="Center" Margin="0,-1,0,0"/>
                </Border>
                <TextBlock Text="Copied to clipboard" Foreground="#FFFFFF"
                           FontFamily="Segoe UI" FontWeight="SemiBold" FontSize="12"
                           VerticalAlignment="Center"/>
            </StackPanel>
        </Border>
    </Grid>
</Window>
"@

$win = [System.Windows.Markup.XamlReader]::Load((New-Object System.Xml.XmlNodeReader $xaml))
$btnCam = $win.FindName("BtnCam")
$btnMin = $win.FindName("BtnMin")
$btnClose = $win.FindName("BtnClose")
$toastBorder = $win.FindName("ToastBorder")

# Toast Timer
$toastTimer = New-Object System.Windows.Threading.DispatcherTimer
$toastTimer.Interval = [TimeSpan]::FromSeconds(2.0)
$toastTimer.Add_Tick({
    $toastBorder.Opacity = 0.0
    $toastTimer.Stop()
})

$btnMin.Add_Click({
    $win.WindowState = [System.Windows.WindowState]::Minimized
})

$btnClose.Add_Click({
    $win.Close()
})

# Fullscreen Toggle on F11
$win.Add_KeyDown({
    param($s, $e)
    if ($e.Key -eq [System.Windows.Input.Key]::F11) {
        if ($win.WindowState -eq [System.Windows.WindowState]::Maximized) {
            $win.WindowState = [System.Windows.WindowState]::Normal
        } else {
            $win.WindowState = [System.Windows.WindowState]::Maximized
        }
    }
})

# Concurrency & Debounce protected screenshot capture
$script:isCapturing = $false
$btnCam.Add_Click({
    if ($script:isCapturing) { return }
    $script:isCapturing = $true

    try {
        $tempFile = [System.IO.Path]::GetTempFileName() + ".png"
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $Adb
        $psi.Arguments = "-s $Serial exec-out screencap -p"
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.CreateNoWindow = $true

        $capProc = [System.Diagnostics.Process]::Start($psi)
        $fs = [System.IO.File]::Create($tempFile)
        $capProc.StandardOutput.BaseStream.CopyTo($fs)
        $fs.Close()
        $capProc.WaitForExit()

        if ((Test-Path $tempFile) -and (Get-Item $tempFile).Length -gt 1000) {
            $bytes = [System.IO.File]::ReadAllBytes($tempFile)
            Remove-Item $tempFile -Force -ErrorAction SilentlyContinue

            $ms = New-Object System.IO.MemoryStream(,$bytes)
            $img = [System.Drawing.Image]::FromStream($ms)
            [System.Windows.Forms.Clipboard]::SetImage($img)
            $img.Dispose()
            $ms.Dispose()

            [System.Media.SystemSounds]::Asterisk.Play()
            $toastBorder.Opacity = 1.0
            $toastTimer.Stop()
            $toastTimer.Start()
            Write-Host "[+] Mobile screenshot ($Serial) copied to Windows clipboard successfully." -ForegroundColor Green
        } else {
            Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
            Write-Host "[!] Screenshot capture failed: Device may be locked or disconnected." -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[!] Screenshot exception: $($_.Exception.Message)" -ForegroundColor Red
    } finally {
        $script:isCapturing = $false
    }
})

# Resizing synchronization function
function Update-ChildWindowSize {
    if ($sdlHwnd -ne [IntPtr]::Zero -and [FluxNativeHost]::IsWindow($sdlHwnd)) {
        $source = [System.Windows.PresentationSource]::FromVisual($win)
        $dpiX = 1.0
        $dpiY = 1.0
        if ($source -and $source.CompositionTarget) {
            $dpiX = $source.CompositionTarget.TransformToDevice.M11
            $dpiY = $source.CompositionTarget.TransformToDevice.M22
        }

        $w = [int](($win.ActualWidth) * $dpiX)
        $h = [int](($win.ActualHeight - 34) * $dpiY)
        $top = [int](34 * $dpiY)

        if ($w -gt 50 -and $h -gt 50) {
            [FluxNativeHost]::MoveWindow($sdlHwnd, 0, $top, $w, $h, $true) | Out-Null
        }
    }
}

$script:hostHwnd = [IntPtr]::Zero
$win.Add_Loaded({
    $script:hostHwnd = (New-Object System.Windows.Interop.WindowInteropHelper($win)).Handle

    # Reparent SDL window to host container
    $oldStyle = [FluxNativeHost]::GetWindowLong($sdlHwnd, [FluxNativeHost]::GWL_STYLE)
    $newStyle = ($oldStyle -band (-bnot [FluxNativeHost]::WS_POPUP)) -bor [FluxNativeHost]::WS_CHILD
    [FluxNativeHost]::SetWindowLong($sdlHwnd, [FluxNativeHost]::GWL_STYLE, $newStyle) | Out-Null
    [FluxNativeHost]::SetParent($sdlHwnd, $script:hostHwnd) | Out-Null

    Update-ChildWindowSize

    # Focus scrcpy for immediate input reception
    [FluxNativeHost]::SetForegroundWindow($script:hostHwnd) | Out-Null
    [FluxNativeHost]::SetFocus($sdlHwnd) | Out-Null
})

$win.Add_SizeChanged({
    Update-ChildWindowSize
})

$win.Add_Closed({
    if ($sdlHwnd -ne [IntPtr]::Zero) {
        [FluxNativeHost]::SendMessage($sdlHwnd, [FluxNativeHost]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    }
    if ($scrcpyProc -and -not $scrcpyProc.HasExited) {
        Stop-Process -Id $scrcpyProc.Id -Force -ErrorAction SilentlyContinue
    }
})

# Background monitor to close host window if scrcpy terminates
$pollTimer = New-Object System.Windows.Threading.DispatcherTimer
$pollTimer.Interval = [TimeSpan]::FromMilliseconds(500)
$pollTimer.Add_Tick({
    if (-not $scrcpyProc -or $scrcpyProc.HasExited) {
        $pollTimer.Stop()
        $win.Close()
    }
})
$pollTimer.Start()

Write-Host "[+] FluxMirror running with custom camera title bar. Press Close to exit." -ForegroundColor Green
$win.ShowDialog() | Out-Null
