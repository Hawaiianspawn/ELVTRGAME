# Shared helpers for talking to the running Unreal editor over its MCP HTTP server,
# plus process/build utilities. Dot-source this from the other scripts:
#   . "$PSScriptRoot\ue-mcp.ps1"

$ErrorActionPreference = "Stop"

# --- Project / engine paths -------------------------------------------------
$script:RepoRoot    = Split-Path -Parent $PSScriptRoot
$script:ProjectDir  = Join-Path $script:RepoRoot "ELVTR"
$script:UProject    = Join-Path $script:ProjectDir "ELVTR.uproject"
$script:DllPath     = Join-Path $script:ProjectDir "Binaries\Win64\UnrealEditor-ELVTR.dll"
# The editor-only module (breadboard and future in-engine tooling). A build that touches
# only this one legitimately leaves UnrealEditor-ELVTR.dll alone, so the "did the build
# actually write anything" guard below has to watch both DLLs or it cries wolf.
$script:EditorDllPath = Join-Path $script:ProjectDir "Binaries\Win64\UnrealEditor-ELVTREditor.dll"

# The MCP plugin's port is editable in Editor Preferences, which writes to the
# per-project *saved* config and overrides the checked-in default. Read the
# saved value first or Wait-Mcp polls a dead port and reports a false timeout.
function Get-McpPort {
    $configs = @(
        (Join-Path $script:ProjectDir "Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini"),
        (Join-Path $script:ProjectDir "Config\DefaultEditorPerProjectUserSettings.ini")
    )
    foreach ($cfg in $configs) {
        if (-not (Test-Path $cfg)) { continue }
        $match = Select-String -Path $cfg -Pattern '^\s*ServerPortNumber\s*=\s*(\d+)' | Select-Object -Last 1
        if ($match) { return [int]$match.Matches[0].Groups[1].Value }
    }
    return 8000
}

$script:McpPort     = Get-McpPort
$script:McpUrl      = "http://127.0.0.1:$($script:McpPort)/mcp"

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
    # FOUR levels up, not three: the exe lives at <Engine>\Engine\Binaries\Win64\UnrealEditor.exe,
    # so three only reaches <Engine>\Engine and every path built from it gains a doubled
    # "Engine\Engine". The fallback below returns the right shape, which is why this only ever
    # broke when an editor happened to be running — e.g. a crashed one still lingering.
    if ($ue -and $ue.Path) {
        return (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $ue.Path))))
    }
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
    $watched = @($script:DllPath, $script:EditorDllPath)
    $before = @($watched | ForEach-Object { if (Test-Path $_) { (Get-Item $_).LastWriteTimeUtc } else { [DateTime]::MinValue } })
    $out = & $bat ELVTREditor Win64 Development -project="$($script:UProject)" -WaitMutex 2>&1 | ForEach-Object { "$_" }
    $out | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { return $false }
    if ($out -match 'Unable to build while Live Coding is active') { return $false }
    # Guard against a "success" that wrote nothing (stale module would silently load).
    # ANY watched module being rewritten counts: an editor-tooling-only change rebuilds
    # UnrealEditor-ELVTREditor.dll and leaves the runtime module untouched, which is correct.
    $after = @($watched | ForEach-Object { if (Test-Path $_) { (Get-Item $_).LastWriteTimeUtc } else { [DateTime]::MinValue } })
    $rewrote = $false
    for ($i = 0; $i -lt $watched.Count; $i++) { if ($after[$i] -ne $before[$i]) { $rewrote = $true } }
    # -join first: $out is a string ARRAY, and -match/-notmatch on an array filters it rather
    # than returning a bool, so the bare form was truthy for any build whose output had even
    # one non-matching line — i.e. it reported every up-to-date build as a failure.
    if ((-not $rewrote) -and (($out -join "`n") -notmatch 'Target is up to date')) {
        Write-Host "Build reported success but neither $($script:DllPath) nor $($script:EditorDllPath) was rewritten - treating as failure." -ForegroundColor Yellow
        return $false
    }
    return $true
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
