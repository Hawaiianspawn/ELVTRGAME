param([int]$ProcId, [string]$Out, [int]$DelaySec = 0, [string]$Title = "")
# Desktop-capture a process's main window (works for D3D windows where PrintWindow returns stale frames).
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class WinCap {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string cls, string title);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
'@
$h = if ($Title) { [WinCap]::FindWindow($null, $Title) } else { (Get-Process -Id $ProcId).MainWindowHandle }
[WinCap]::SetForegroundWindow($h) | Out-Null
if ($DelaySec -gt 0) { Start-Sleep -Seconds $DelaySec } else { Start-Sleep -Milliseconds 700 }
$r = New-Object WinCap+RECT
[WinCap]::GetWindowRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$Out
