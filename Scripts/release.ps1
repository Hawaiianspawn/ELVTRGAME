# Build the Kindled web release: repack sprites, import, export, zip.
#   powershell -File Scripts\release.ps1
[CmdletBinding()]
param()

$repo = Split-Path $PSScriptRoot -Parent
$proj = Join-Path $repo 'godot'
$pkg = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe'
$con = Join-Path $pkg 'Godot_v4.7.2-stable_win64_console.exe'
if (-not (Test-Path $con)) { Write-Host "Godot not found at $pkg (winget install GodotEngine.GodotEngine)" -ForegroundColor Red; exit 1 }

py (Join-Path $repo 'Scripts\art\godot_pack.py')
if (-not $?) { Write-Host "sprite pack failed" -ForegroundColor Red; exit 1 }

& $con --headless --path $proj --import
if ($LASTEXITCODE -ne 0) { Write-Host "import failed (exit $LASTEXITCODE)" -ForegroundColor Red; exit 1 }

$webOut = Join-Path $repo 'build\web\index.html'
New-Item -ItemType Directory -Force (Split-Path $webOut) | Out-Null
& $con --headless --path $proj --export-release Web $webOut
if ($LASTEXITCODE -ne 0) { Write-Host "export failed (exit $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
if (-not (Test-Path $webOut)) { Write-Host "export produced no index.html at $webOut" -ForegroundColor Red; exit 1 }

$gameGd = Get-Content (Join-Path $proj 'scripts\Game.gd') -Raw
if ($gameGd -notmatch 'VERSION\s*:=\s*"([^"]+)"') { Write-Host "could not find VERSION const in Game.gd" -ForegroundColor Red; exit 1 }
$version = $Matches[1]

$zip = Join-Path $repo "build\kindled-web-v$version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force -Confirm:$false }
Compress-Archive -Path (Join-Path $repo 'build\web\*') -DestinationPath $zip

Write-Host "web export: $webOut" -ForegroundColor Green
Write-Host "zip: $zip" -ForegroundColor Green
