<#
.SYNOPSIS
  Push the Kindled web build to itch.io via butler.

.EXAMPLE
  powershell -File Scripts\itch-push.ps1
.EXAMPLE
  powershell -File Scripts\itch-push.ps1 -DryRun
#>
param(
    [string]$ItchUser = 'hawaiianspawn',
    [string]$Channel = 'html5',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($env:ITCH_API_KEY)) {
    Write-Host "ITCH_API_KEY is not set. Get an API key at https://itch.io/user/settings/api-keys and set it: `$env:ITCH_API_KEY = '...'"
    exit 1
}

$repoRoot = Split-Path $PSScriptRoot -Parent
$buildWeb = Join-Path $repoRoot 'build\web'
$indexHtml = Join-Path $buildWeb 'index.html'

if (-not (Test-Path $indexHtml)) {
    Write-Host "No web build found at $indexHtml. Run Scripts\release.ps1 to produce one first."
    exit 1
}

$gameGd = Join-Path $repoRoot 'godot\scripts\Game.gd'
$ver = '0.1.0'
if (Test-Path $gameGd) {
    $match = Select-String -Path $gameGd -Pattern 'const\s+VERSION\s*:=\s*"([^"]+)"' | Select-Object -First 1
    if ($match) {
        $ver = $match.Matches[0].Groups[1].Value
    } else {
        Write-Warning "No VERSION constant found in $gameGd; using $ver"
    }
} else {
    Write-Warning "Game.gd not found at $gameGd; using $ver"
}

$butlerCmd = Get-Command butler -ErrorAction SilentlyContinue
if ($butlerCmd) {
    $butler = $butlerCmd.Source
} else {
    $butler = Join-Path $env:LOCALAPPDATA 'butler\butler.exe'
    if (-not (Test-Path $butler)) {
        Write-Host "butler.exe not found on PATH or at $butler. Install it first."
        exit 1
    }
}

$env:BUTLER_API_KEY = $env:ITCH_API_KEY

$target = "$ItchUser/kindled:$Channel"
$butlerArgs = @('push', $buildWeb, $target, '--userversion', $ver)
if ($DryRun) {
    $butlerArgs += '--dry-run'
}

Write-Host "$butler $($butlerArgs -join ' ')"
& $butler @butlerArgs
exit $LASTEXITCODE
