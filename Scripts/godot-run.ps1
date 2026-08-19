# Relaunch the local Godot build of Kindled: The Green Dot, or run the evidence probes against it.
#   pwsh Scripts\godot-run.ps1                    # kill our previous run, launch the game windowed
#   pwsh Scripts\godot-run.ps1 -Pack              # repack sprites from roster.json first
#   pwsh Scripts\godot-run.ps1 -Probe battle,12   # run probe(s), print PROBE lines + png path, no relaunch
#   pwsh Scripts\godot-run.ps1 -Probe "battle,12,swap;cavalry,8"   # several, ; separated
# No export step: the engine binary runs the project folder directly, which is the same code the web
# export ships. Add a Windows export preset only if a standalone .exe is ever needed.
[CmdletBinding()]
param([string]$Probe, [switch]$Pack)

$repo = Split-Path $PSScriptRoot -Parent
$proj = Join-Path $repo 'godot'
$pkg = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe'
$gui = Join-Path $pkg 'Godot_v4.7.2-stable_win64.exe'
$con = Join-Path $pkg 'Godot_v4.7.2-stable_win64_console.exe'
if (-not (Test-Path $con)) { Write-Host "Godot not found at $pkg (winget install GodotEngine.GodotEngine)" -ForegroundColor Red; exit 1 }

if ($Pack) { py (Join-Path $repo 'Scripts\art\godot_pack.py'); if (-not $?) { exit 1 } }

# Only stop game runs we launched (--path <our project>, not an editor session: no -e/--editor).
Get-CimInstance Win32_Process -Filter "Name LIKE 'Godot_v4.7%'" |
    Where-Object { $_.CommandLine -like "*--path*$proj*" -and $_.CommandLine -notmatch ' -e | --editor' } |
    ForEach-Object { Write-Host "stopping previous run pid $($_.ProcessId)"; Stop-Process -Id $_.ProcessId -Force }

if ($Probe) {
    $fail = 0
    foreach ($spec in $Probe.Split(";")) {
        Write-Host "== probe $spec" -ForegroundColor Cyan
        $out = & $con --path $proj -- "--probe=$spec"
        $out | Where-Object { $_ -match '^PROBE|SCRIPT ERROR|Assertion failed' } | ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0 -or -not ($out -match '^PROBE .* fps=')) { Write-Host "probe $spec FAILED (exit $LASTEXITCODE)" -ForegroundColor Red; $fail = 1 }
    }
    exit $fail
}

$p = Start-Process $gui -ArgumentList '--path', "`"$proj`"" -PassThru
Write-Host "launched Kindled pid $($p.Id)" -ForegroundColor Green
