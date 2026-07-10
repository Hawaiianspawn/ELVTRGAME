# Iteration scripts

Fast, safe iteration on ELVTR C++ changes. After editing code, run **one** command:

```powershell
pwsh Scripts\ue-iterate.ps1
```

It inspects your uncommitted changes under `ELVTR/Source` and picks the right path:

| Change | Path taken | Speed |
|---|---|---|
| `.cpp` body edits, tuning constants, non-reflected header tweaks | **Live Coding** hot-patch into the running editor | seconds, no restart |
| New/removed/renamed files, reflection changes (`UCLASS/USTRUCT/UPROPERTY/UFUNCTION/GENERATED_BODY` — e.g. a new Mass fragment or processor), `*.Build.cs` / `*.Target.cs` | **Safe relaunch**: close → wait for exit → build → relaunch → wait for MCP | ~1 min |

### Options
```powershell
pwsh Scripts\ue-iterate.ps1 -DryRun            # print the decision + reasons, do nothing
pwsh Scripts\ue-iterate.ps1 -Force LiveCoding  # force hot-patch
pwsh Scripts\ue-iterate.ps1 -Force Relaunch    # force full rebuild + restart
pwsh Scripts\ue-relaunch.ps1                   # just do the safe relaunch
```

### How it decides
Uses `git status` / `git diff` on your working tree (your uncommitted work is the "changes since last build"). If Live Coding compile fails, it reports the compiler diagnostics and does **not** relaunch (a relaunch build would fail too — fix the error and re-run).

### Requirements (already configured in this project)
- Live Coding enabled by default — `ELVTR/Config/DefaultEditorPerProjectUserSettings.ini` (`[/Script/LiveCoding.LiveCodingSettings] bEnabled=True`).
- MCP server auto-starts on port 8000, with `LiveCodingToolset` enabled in `ELVTR.uproject` (lets the script trigger a hot-compile and read the result).
- The editor must be running for the Live Coding path; if it isn't, the script falls back to a relaunch.

### For your close/reopen automation
Point it at `ue-iterate.ps1` instead of a blind close/reopen. That removes the spurious
"Missing ELVTR Modules" dialog: it either hot-patches (no restart) or does a real build
before relaunching, so the module is always current, and it waits for the old process to
exit before building (no DLL-lock race).
