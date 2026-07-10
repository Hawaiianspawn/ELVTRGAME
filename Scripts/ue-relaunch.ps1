# Safe relaunch: close editor -> wait for full exit + DLL unlock -> build -> relaunch -> wait for MCP.
# Use for changes Live Coding cannot patch (new/removed files, reflection changes, .Build.cs).
# Called by ue-iterate.ps1, or run directly:  pwsh Scripts\ue-relaunch.ps1
[CmdletBinding()]
param([switch]$NoWaitMcp)

. "$PSScriptRoot\ue-mcp.ps1"

Stop-Editor | Out-Null

if (-not (Build-Editor)) {
    Write-Host ""
    Write-Host "BUILD FAILED - fix the compile errors above, then re-run. Editor NOT relaunched." -ForegroundColor Red
    exit 1
}
Write-Host "Build succeeded." -ForegroundColor Green

Start-Editor

if ($NoWaitMcp) { exit 0 }

if (Wait-Mcp -TimeoutSec 300) {
    Write-Host "Editor ready, MCP connected." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Editor launched but MCP did not come up in time (check for the 'Missing Modules' dialog)." -ForegroundColor Yellow
    exit 2
}
