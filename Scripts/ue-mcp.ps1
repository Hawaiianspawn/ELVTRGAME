# Shared helpers for talking to the running Unreal editor over its MCP HTTP server,
# plus process/build utilities. Dot-source this from the other scripts:
#   . "$PSScriptRoot\ue-mcp.ps1"

$ErrorActionPreference = "Stop"

# --- Project / engine paths -------------------------------------------------
$script:RepoRoot    = Split-Path -Parent $PSScriptRoot
$script:ProjectDir  = Join-Path $script:RepoRoot "ELVTR"
$script:UProject    = Join-Path $script:ProjectDir "ELVTR.uproject"
$script:DllPath     = Join-Path $script:ProjectDir "Binaries\Win64\UnrealEditor-ELVTR.dll"
$script:McpUrl      = "http://127.0.0.1:8000/mcp"

# Run git and return stdout lines, swallowing native stderr (e.g. CRLF warnings)
# so it never trips ErrorActionPreference=Stop.
function Invoke-GitLines {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$GitArgs)
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        Push-Location $script:RepoRoot
        $out = & git @GitArgs 2>$null
        return $out
    } finally { Pop-Location; $ErrorActionPreference = $old }
}

function Get-EngineDir {
    # Prefer the engine the running editor was launched from; else default to UE_5.8.
    $ue = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ue -and $ue.Path) { return (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $ue.Path))) }
    return "C:\Program Files\Epic Games\UE_5.8"
}

# --- MCP session ------------------------------------------------------------
$script:McpSid = $null

function Test-Mcp {
    try {
        $body = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"claude","version":"1.0"}}}'
        $r = Invoke-WebRequest -Uri $script:McpUrl -Method POST -ContentType "application/json" `
            -Headers @{ "Accept" = "application/json, text/event-stream" } -Body $body -UseBasicParsing -TimeoutSec 6
        $script:McpSid = $r.Headers['Mcp-Session-Id']
        $h = @{ "Accept" = "application/json, text/event-stream"; "Mcp-Session-Id" = $script:McpSid }
        Invoke-WebRequest -Uri $script:McpUrl -Method POST -ContentType "application/json" -Headers $h `
            -Body '{"jsonrpc":"2.0","method":"notifications/initialized"}' -UseBasicParsing -TimeoutSec 6 | Out-Null
        return $true
    } catch { return $false }
}

function ConvertFrom-McpBody([string]$Content) {
    if ($Content -match '(?m)^data:') {
        return (($Content -split "`n" | Where-Object { $_ -like 'data:*' } | ForEach-Object { $_.Substring(5).Trim() }) -join "")
    }
    return $Content
}

function Invoke-McpTool {
    param([string]$ToolName, $Arguments = @{}, [string]$Toolset = $null, [int]$TimeoutSec = 180)
    if (-not $script:McpSid) { if (-not (Test-Mcp)) { throw "MCP server not reachable on $($script:McpUrl)" } }
    $h = @{ "Accept" = "application/json, text/event-stream"; "Mcp-Session-Id" = $script:McpSid }
    $callArgs = @{ tool_name = $ToolName; arguments = $Arguments }
    if ($Toolset) { $callArgs.toolset_name = $Toolset }
    $req = @{ jsonrpc = "2.0"; id = 20; method = "tools/call"; params = @{ name = "call_tool"; arguments = $callArgs } } | ConvertTo-Json -Depth 12 -Compress
    $r = Invoke-WebRequest -Uri $script:McpUrl -Method POST -ContentType "application/json" -Headers $h -Body $req -UseBasicParsing -TimeoutSec $TimeoutSec
    $raw = ConvertFrom-McpBody $r.Content
    try {
        $o = $raw | ConvertFrom-Json
        if ($o.error) { return "MCP ERROR: " + ($o.error | ConvertTo-Json -Compress) }
        return (($o.result.content | Where-Object { $_.type -eq 'text' } | ForEach-Object { $_.text }) -join "`n")
    } catch { return $raw }
}

# --- Process / build --------------------------------------------------------
function Stop-Editor {
    param([int]$TimeoutSec = 90)
    $procs = Get-Process UnrealEditor -ErrorAction SilentlyContinue
    if (-not $procs) { return $true }
    Write-Host "Closing editor (PID $($procs.Id -join ','))..."
    foreach ($p in $procs) { $p.CloseMainWindow() | Out-Null }
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 500
        if (-not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) { break }
    }
    $procs = Get-Process UnrealEditor -ErrorAction SilentlyContinue
    if ($procs) { Write-Host "Graceful close timed out; killing."; $procs | Stop-Process -Force; Start-Sleep -Seconds 2 }
    # Wait for the DLL lock to release so a build can overwrite it.
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        try { if (Test-Path $script:DllPath) { $fs = [IO.File]::Open($script:DllPath, 'Open', 'ReadWrite', 'None'); $fs.Close() }; return $true }
        catch { Start-Sleep -Milliseconds 500 }
    }
    return $true
}

function Build-Editor {
    $engine = Get-EngineDir
    $bat = Join-Path $engine "Engine\Build\BatchFiles\Build.bat"
    Write-Host "Building ELVTREditor (Development Win64)..."
    & $bat ELVTREditor Win64 Development -project="$($script:UProject)" -WaitMutex
    return ($LASTEXITCODE -eq 0)
}

function Start-Editor {
    $engine = Get-EngineDir
    $exe = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor.exe"
    Write-Host "Launching editor..."
    Start-Process -FilePath $exe -ArgumentList "`"$($script:UProject)`""
}

function Wait-Mcp {
    param([int]$TimeoutSec = 300)
    Write-Host "Waiting for editor + MCP server (up to $TimeoutSec s)..."
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 5
        if (Test-Mcp) { Write-Host "MCP is up."; return $true }
    }
    return $false
}
