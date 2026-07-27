# Scripts

## Backlog — `backlog.py`, `backlog_guard.py`

The mechanical half of the host. Judgment lives in `.claude/skills/backlog/` (sweep
the repo), `.claude/skills/host/` (a goal the owner brings) and
`.claude/agents/host.md`; anything deterministic lives here.

```bash
py Scripts/backlog.py list --top 7      # the audit queue, ranked
py Scripts/backlog.py validate          # schema, dup ids, dangling deps, lock conflicts
py Scripts/backlog.py reindex           # regenerate docs/backlog/INDEX.md
py Scripts/backlog.py show 12           # one task with its resolved score
py Scripts/backlog.py next-id           # next free 3-digit id
py Scripts/backlog.py sweep-report --json   # raw ingest surface
py Scripts/backlog.py start|review <ids>    # agents may run these
py Scripts/backlog.py approve|done|reject|park <ids>   # owner's verdict — prompts
py Scripts/backlog.py dispatch 44 --teammate flame-flicker   # approved -> in-progress
```

`dispatch` is `start` with the spawned teammate recorded. It refuses unless the task
is `approved` with every dependency closed, so `/host` cannot spawn a teammate onto
work the owner never saw — run it *before* the spawn and a refusal costs nothing. One
teammate per task; the name must match the Agent tool's own `[A-Za-z0-9][\w-]{0,63}`
constraint so it stays addressable by `SendMessage`.

Score is `(gate × risk × unblocks) ÷ cost`; `unblocks` is computed from open
dependents, so closing a task re-ranks everything downstream on the next `reindex`.

`backlog_guard.py` is a PreToolUse hook (wired in `.claude/settings.json`, matcher
`Edit|Write|MultiEdit|Bash|PowerShell`). It **denies** any file edit that sets a task
to `approved`/`done`/`rejected`/`parked`, denies hand-edits to the generated
`INDEX.md` and append-only `LOG.md`, and **asks** before any `backlog.py` privileged
verb. The ask has to be enforced by the hook rather than by omission from an
allowlist, because `.claude/settings.local.json` carries a blanket `Bash(py *)` rule.

Two gotchas the script already handles, worth not re-discovering: PyYAML is YAML 1.1,
where `id: 010` parses as **octal 8** — ids are read from the raw frontmatter, never
from the YAML load. And Windows consoles are cp1252, which cannot encode the arrows
in task titles, so stdout is reconfigured to UTF-8 before any print.

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
