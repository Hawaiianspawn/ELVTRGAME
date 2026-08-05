# Scripts

## Backlog — `paca.py`, `paca_guard.py`

**The backlog is [Paca](https://github.com/Paca-AI/paca)** — self-hosted at
<http://localhost:8090>, project **Kindled**, stack in `C:\Projects\paca`. It holds
the tasks, the board, the statuses and the history. Reach it three ways: the web board,
the `paca` MCP server (`/paca`, `/paca-prioritize`, …), or `paca.py`.

`docs/backlog/` is a frozen archive of the 81 tasks that closed before the move on
2026-08-01. Tasks keep their `task-NNN` handle in Paca's `Legacy ID` field, so every id
in the repo's prose still resolves.

`paca.py` is deliberately thin. It exists only for the things Paca has no opinion about
— **this project's locks and this project's approval gate** — and reimplements nothing
Paca already does:

```bash
py Scripts/paca.py list --status proposed     # ranked by score
py Scripts/paca.py list --epic wave-measurement
py Scripts/paca.py validate                   # lock conflicts, cycles, dangling deps
py Scripts/paca.py waves 63,64,65             # what can run at once, and what waits
py Scripts/paca.py next-id                    # next free 3-digit legacy id
py Scripts/paca.py sweep-report --json        # raw ingest surface, read from the repo
py Scripts/paca.py new spec.json              # file a proposed task
py Scripts/paca.py review <ids>               # agents may run this
py Scripts/paca.py approve|done|reject|park <ids>   # owner's verdict — prompts
py Scripts/paca.py dispatch 44 --teammate flame-flicker   # approved -> in-progress
```

`dispatch` refuses unless the task is `approved` with every dependency closed and no
in-flight task holding a conflicting lock, so `/host` cannot spawn a teammate onto work
the owner never saw — run it *before* the spawn and a refusal costs nothing. One
teammate per task; the name must match the Agent tool's own `[A-Za-z0-9][\w-]{0,63}`
constraint so it stays addressable by `SendMessage`.

Score is `(feel × risk × perf × unblocks) ÷ cost`; `unblocks` is computed from open
dependents, so closing a task re-ranks everything downstream.

`paca_guard.py` is a PreToolUse hook (wired in `.claude/settings.json`, matcher
`Edit|Write|MultiEdit|Bash|PowerShell`). It **asks** before any `paca.py` privileged
verb — `approve`, `reject`, `park`, `done` — and **denies** edits to the frozen records
in `docs/backlog/` (`INDEX.md`, the signpost, is exempt). The ask has to be enforced by
the hook rather than by omission from an allowlist, because `.claude/settings.local.json`
carries a blanket `Bash(py *)` rule, and a hook `ask` overrides an allow rule.

Credentials live in `Scripts/.paca.json` (gitignored) or `PACA_API_URL` /
`PACA_API_KEY` / `PACA_PROJECT_ID`. If the stack is down, bring it up with
`docker compose --env-file .env up -d --scale ai-agent=0` from `C:\Projects\paca`.

One gotcha worth not re-discovering: Windows consoles are cp1252 and cannot encode the
em-dashes in task titles, so stdout is reconfigured to UTF-8 before any print.

---

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
