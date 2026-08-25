# Relaunch the local Godot build of Kindled: The Necromancer's Keep, or run the evidence probes against it.
#   pwsh Scripts\godot-run.ps1                    # kill our previous run, launch the game windowed
#   pwsh Scripts\godot-run.ps1 -Pack              # repack sprites from roster.json first
#   pwsh Scripts\godot-run.ps1 -Probe battle,12   # run probe(s), print PROBE lines + png path, no relaunch
#   pwsh Scripts\godot-run.ps1 -Probe "battle,12,swap;siege,6"   # several, ; separated
#   pwsh Scripts\godot-run.ps1 -Adversary battle,60,7   # adversarial QA run: scene,secs,seed -> docs/qa/
# No export step: the engine binary runs the project folder directly, which is the same code the web
# export ships. Add a Windows export preset only if a standalone .exe is ever needed.
[CmdletBinding()]
param([string]$Probe, [string]$Adversary, [switch]$Pack)

$repo = Split-Path $PSScriptRoot -Parent
$proj = Join-Path $repo 'godot'
$pkg = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe'
$gui = Join-Path $pkg 'Godot_v4.7.2-stable_win64.exe'
$con = Join-Path $pkg 'Godot_v4.7.2-stable_win64_console.exe'
if (-not (Test-Path $con)) { Write-Host "Godot not found at $pkg (winget install GodotEngine.GodotEngine)" -ForegroundColor Red; exit 1 }

if ($Pack) {
    py (Join-Path $repo 'Scripts\art\godot_pack.py'); if (-not $?) { exit 1 }
    & $con --headless --path $proj --import | Out-Null   # runtime reads .godot/imported, not the png
}

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

if ($Adversary) {
    $udir = Join-Path $env:APPDATA 'Godot/app_userdata/Kindled- The Necromancer''s Keep'
    Remove-Item (Join-Path $udir "adversary_$($Adversary.Split(',')[2])_f*.png") -ErrorAction SilentlyContinue   # stale per-finding shots
    # 2>&1: Godot prints SCRIPT ERROR on stderr; ToString unwraps the ErrorRecord PS 5.1 wraps it in
    $out = @(& $con --path $proj -- "--adversary=$Adversary" 2>&1 | ForEach-Object { $_.ToString() })
    $out | Where-Object { $_ -match '^ADVERSARY|SCRIPT ERROR|Assertion failed' } | ForEach-Object { Write-Host $_ }
    if ($out -match 'Parse Error|Compile Error') { Write-Host 'adversary run hit a script parse/compile error: fix the game first' -ForegroundColor Red; exit 1 }
    $json = ($out | Select-String 'report=(.*\.json)').Matches[0].Groups[1].Value
    if (-not $json -or -not (Test-Path $json)) { Write-Host "adversary run produced no report (exit $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
    # engine-level errors are findings too: fold SCRIPT ERROR lines into the report
    $rep = Get-Content $json -Raw -Encoding utf8 | ConvertFrom-Json
    $errs = @(); $seen = @{}
    for ($i = 0; $i -lt $out.Count - 1; $i++) {
        if ($out[$i] -notmatch '^(SCRIPT ERROR|ERROR): ' -or $out[$i] -match 'BUG:|RID |leaked|Pages in use|still in use') { continue }
        $key = "$($out[$i])|$($out[$i+1].Trim())"
        if ($seen[$key]) { $seen[$key]++; continue }
        $seen[$key] = 1
        $errs += [pscustomobject]@{ message = $out[$i]; at = $out[$i+1].Trim(); count = 1 }
    }
    foreach ($e in $errs) { $e.count = $seen["$($e.message)|$($e.at)"] }
    $rep | Add-Member -NotePropertyName engine_errors -NotePropertyValue $errs -Force
    $dst = Join-Path $repo 'docs\qa'
    New-Item -ItemType Directory -Force $dst | Out-Null
    $stem = [IO.Path]::GetFileNameWithoutExtension($json)
    $rep | ConvertTo-Json -Depth 12 | Out-File (Join-Path $dst "$stem.json") -Encoding utf8
    Copy-Item ([IO.Path]::ChangeExtension($json, 'csv')) $dst -Force
    Copy-Item (Join-Path (Split-Path $json) "$stem*.png") $dst -Force    # end frame + one per finding
    Write-Host "report: $dst\$stem.json ($($rep.findings.Count) findings, $($errs.Count) engine errors)" -ForegroundColor Green
    exit 0
}

$p = Start-Process $gui -ArgumentList '--path', "`"$proj`"" -PassThru
Write-Host "launched Kindled pid $($p.Id)" -ForegroundColor Green
