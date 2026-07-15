# Smart iteration dispatcher for ELVTR C++ changes.
#
#   pwsh Scripts\ue-iterate.ps1            # auto-decide from working-tree changes
#   pwsh Scripts\ue-iterate.ps1 -DryRun    # just print the decision + reasons
#   pwsh Scripts\ue-iterate.ps1 -Force LiveCoding   # force hot-patch
#   pwsh Scripts\ue-iterate.ps1 -Force Relaunch     # force full rebuild+restart
#
# Decision: uses `git status`/`git diff` on ELVTR/Source (your uncommitted work).
#   FULL RELAUNCH when any change is a .Build.cs/.Target.cs, an added/removed/renamed
#   file, or a reflection change (UCLASS/USTRUCT/UENUM/UINTERFACE/UFUNCTION/UPROPERTY/
#   GENERATED_BODY) — e.g. a new Mass fragment or processor.
#   LIVE CODING otherwise (function-body edits, tuning constants, non-reflected header
#   tweaks) — patched into the running editor with no restart.
[CmdletBinding()]
param(
    [ValidateSet("Auto", "LiveCoding", "Relaunch")]
    [string]$Force = "Auto",
    [switch]$DryRun
)

. "$PSScriptRoot\ue-mcp.ps1"

$ReflectionRegex = 'UCLASS|USTRUCT|UENUM|UINTERFACE|UFUNCTION|UPROPERTY|GENERATED_BODY|GENERATED_UCLASS_BODY|IMPLEMENT_.*MODULE'
$SrcPathspec = "ELVTR/Source"

function Get-ChangedCppState {
    $lines = Invoke-GitLines status --porcelain -- $SrcPathspec
    $result = @()
    foreach ($ln in $lines) {
        if (-not $ln) { continue }
        $x = $ln.Substring(0,2)
        $path = $ln.Substring(3).Trim()
        if ($path -match '->') { $path = ($path -split '->')[-1].Trim() }  # rename
        $path = $path.Trim('"')
        if ($path -notmatch '\.(cpp|h|inl|cs)$') { continue }
        $result += [pscustomobject]@{ Status = $x.Trim(); Path = $path }
    }
    return $result
}

function Test-ReflectionChange([string]$RelPath, [string]$Status) {
    if ($Status -match '\?') {
        # untracked (new file) — whole content counts as added
        $content = Get-Content (Join-Path $script:RepoRoot $RelPath) -Raw -ErrorAction SilentlyContinue
        return ($content -match $ReflectionRegex)
    }
    $diff = Invoke-GitLines diff HEAD -- $RelPath
    if (-not $diff) { $diff = Invoke-GitLines diff -- $RelPath }
    $changed = $diff | Where-Object { $_ -match '^[+-]' -and $_ -notmatch '^(\+\+\+|---)' }
    return (($changed -join "`n") -match $ReflectionRegex)
}

function Get-Decision {
    $changes = Get-ChangedCppState
    if (-not $changes -or $changes.Count -eq 0) {
        return [pscustomobject]@{ Mode = "None"; Reasons = @("No uncommitted C++ changes under $SrcPathspec.") }
    }
    $reasons = @()
    $relaunch = $false
    foreach ($c in $changes) {
        $isNewOrGone = ($c.Status -match 'A' -or $c.Status -match 'D' -or $c.Status -match 'R' -or $c.Status -match '\?\?')
        if ($c.Path -match '\.(Build|Target)\.cs$') { $relaunch = $true; $reasons += "build script changed: $($c.Path)" }
        elseif ($isNewOrGone)                       { $relaunch = $true; $reasons += "file added/removed/renamed: $($c.Path)" }
        elseif (Test-ReflectionChange $c.Path $c.Status) { $relaunch = $true; $reasons += "reflection change (UCLASS/USTRUCT/UPROPERTY/etc): $($c.Path)" }
        else { $reasons += "code-body change (live-codeable): $($c.Path)" }
    }
    if ($relaunch) { return [pscustomobject]@{ Mode = "Relaunch"; Reasons = $reasons } }
    return [pscustomobject]@{ Mode = "LiveCoding"; Reasons = $reasons }
}

# --- Decide -----------------------------------------------------------------
if ($Force -ne "Auto") {
    $decision = [pscustomobject]@{ Mode = $Force; Reasons = @("forced by -Force $Force") }
} else {
    $decision = Get-Decision
}

Write-Host "== ue-iterate: $($decision.Mode) ==" -ForegroundColor Cyan
$decision.Reasons | ForEach-Object { Write-Host "  - $_" }

if ($DryRun) { exit 0 }
if ($decision.Mode -eq "None") { exit 0 }

# --- Dispatch ---------------------------------------------------------------
if ($decision.Mode -eq "LiveCoding") {
    if (-not (Test-Mcp)) {
        Write-Host "Editor/MCP not running - nothing to hot-patch into. Falling back to relaunch." -ForegroundColor Yellow
        & "$PSScriptRoot\ue-relaunch.ps1"; exit $LASTEXITCODE
    }
    Write-Host "Triggering Live Coding compile..."
    $out = Invoke-McpTool -ToolName "CompileLiveCoding" -Toolset "LiveCodingToolset.LiveCodingToolset" -TimeoutSec 240
    Write-Host $out
    if ($out -match 'Result:\s*(Success|NoChanges)') { Write-Host "Live Coding applied." -ForegroundColor Green; exit 0 }
    if ($out -match 'not enabled|not loaded') {
        Write-Host "Live Coding unavailable this session - relaunching (config will enable it)." -ForegroundColor Yellow
        & "$PSScriptRoot\ue-relaunch.ps1"; exit $LASTEXITCODE
    }
    Write-Host "Live Coding did not succeed (likely a compile error above). NOT relaunching - fix and re-run." -ForegroundColor Red
    exit 1
}

# Relaunch
& "$PSScriptRoot\ue-relaunch.ps1"
exit $LASTEXITCODE
