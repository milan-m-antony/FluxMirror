<#
.SYNOPSIS
    FluxMirror Floating Screenshot Tool
.DESCRIPTION
    Floating Always-on-Top "Copy Screenshot" pill button that docks directly above the FluxMirror window.
    Captures full resolution phone screenshots directly to the Windows clipboard with a single click.
#>

[CmdletBinding()]
param (
    [switch]$Snap,
    [string]$DeviceId
)

Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

Add-Type @"
  using System;
  using System.Runtime.InteropServices;
  public struct RECT {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }
  public class Win32Helper {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
  }
"@

[xml]$xaml = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="FluxMirror Dock"
        Height="36" Width="180"
        WindowStyle="None"
        AllowsTransparency="True"
        Background="Transparent"
        Topmost="True"
        ShowInTaskbar="False"
        ResizeMode="NoResize">
    <Border Background="#F00F172A"
            BorderBrush="#FF6B00"
            BorderThickness="1.5"
            CornerRadius="18"
            Padding="3,2,3,2">
        <Border.Effect>
            <DropShadowEffect BlurRadius="12" Color="#000000" Opacity="0.7" ShadowDepth="2"/>
        </Border.Effect>
        <Grid VerticalAlignment="Center">
            <Grid.ColumnDefinitions>
                <ColumnDefinition Width="*"/>
                <ColumnDefinition Width="24"/>
            </Grid.ColumnDefinitions>

            <Button Name="BtnSnap" Grid.Column="0"
                    Background="#FF6B00"
                    Foreground="#FFFFFF"
                    FontFamily="Segoe UI"
                    FontSize="11.5"
                    FontWeight="Bold"
                    BorderThickness="0"
                    Height="26"
                    Margin="2,0,3,0"
                    Cursor="Hand">
                <Button.Template>
                    <ControlTemplate TargetType="Button">
                        <Border Name="bdr" Background="{TemplateBinding Background}" CornerRadius="13">
                            <ContentPresenter HorizontalAlignment="Center" VerticalAlignment="Center"/>
                        </Border>
                        <ControlTemplate.Triggers>
                            <Trigger Property="IsMouseOver" Value="True">
                                <Setter TargetName="bdr" Property="Background" Value="#FF8533"/>
                            </Trigger>
                            <Trigger Property="IsPressed" Value="True">
                                <Setter TargetName="bdr" Property="Background" Value="#E05500"/>
                            </Trigger>
                        </ControlTemplate.Triggers>
                    </ControlTemplate>
                </Button.Template>
                <TextBlock Name="TxtSnapLabel" Text="Copy Screenshot" VerticalAlignment="Center"/>
            </Button>

            <Button Name="BtnClose" Grid.Column="1"
                    Background="Transparent"
                    Foreground="#94A3B8"
                    FontFamily="Segoe UI"
                    FontSize="11"
                    FontWeight="Bold"
                    BorderThickness="0"
                    Height="24"
                    Width="22"
                    Cursor="Hand">
                <Button.Template>
                    <ControlTemplate TargetType="Button">
                        <Border Name="bdrClose" Background="{TemplateBinding Background}" CornerRadius="11">
                            <TextBlock Text="X" HorizontalAlignment="Center" VerticalAlignment="Center" Foreground="#94A3B8"/>
                        </Border>
                        <ControlTemplate.Triggers>
                            <Trigger Property="IsMouseOver" Value="True">
                                <Setter TargetName="bdrClose" Property="Background" Value="#334155"/>
                            </Trigger>
                        </ControlTemplate.Triggers>
                    </ControlTemplate>
                </Button.Template>
            </Button>
        </Grid>
    </Border>
</Window>
"@

$reader = (New-Object System.Xml.XmlNodeReader $xaml)
$window = [System.Windows.Markup.XamlReader]::Load($reader)

$btnSnap = $window.FindName("BtnSnap")
$txtLabel = $window.FindName("TxtSnapLabel")
$btnClose = $window.FindName("BtnClose")

$window.Add_MouseLeftButtonDown({ $window.DragMove() })
$btnClose.Add_Click({ $window.Close() })

$btnSnap.Add_Click({
    $txtLabel.Text = "Capturing..."
    $window.Dispatcher.Invoke([Action]{}, [System.Windows.Threading.DispatcherPriority]::Render)

    $ok = Copy-MobileScreenshotToClipboard

    if ($ok) {
        $txtLabel.Text = "Copied!"
        $btnSnap.Background = [System.Windows.Media.BrushConverter]::new().ConvertFromString("#10B981")
        
        $timer = New-Object System.Windows.Threading.DispatcherTimer
        $timer.Interval = [TimeSpan]::FromSeconds(1.5)
        $timer.Add_Tick({
            $txtLabel.Text = "Copy Screenshot"
            $btnSnap.Background = [System.Windows.Media.BrushConverter]::new().ConvertFromString("#FF6B00")
            $timer.Stop()
        })
        $timer.Start()
    } else {
        $txtLabel.Text = "Failed"
        Start-Sleep -Milliseconds 600
        $txtLabel.Text = "Copy Screenshot"
    }
})

$window.Add_Loaded({
    $hwnd = [Win32Helper]::FindWindow($null, "Flux Mirror")
    if ($hwnd -eq [IntPtr]::Zero) {
        $hwnd = [Win32Helper]::FindWindow($null, "FluxMirror")
    }
    if ($hwnd -eq [IntPtr]::Zero) {
        $hwnd = [Win32Helper]::FindWindow($null, "Flux Mobile Screen")
    }

    if ($hwnd -ne [IntPtr]::Zero) {
        $rect = New-Object RECT
        if ([Win32Helper]::GetWindowRect($hwnd, [ref]$rect)) {
            $window.Left = [Math]::Max(10, $rect.Left + 15)
            $window.Top = [Math]::Max(10, $rect.Top - 40)
        }
    } else {
        $window.Left = ([System.Windows.SystemParameters]::PrimaryScreenWidth - $window.Width) / 2
        $window.Top = 30
    }
})

$window.ShowDialog() | Out-Null
