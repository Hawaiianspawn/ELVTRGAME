# Agent Teams — ELVTR working reference

**Created:** 2026-07-26 · Upstream docs: https://code.claude.com/docs/en/agent-teams

Agent teams run several full Claude Code sessions side by side. One session is the **lead**; the
others are **teammates** with their own context windows that message each other directly and share a
task list. Unlike subagents — which only report back to the caller — you can open a teammate's
transcript and talk to it yourself.

Already enabled on this machine via `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS: "1"` in
`~/.claude/settings.json`. Nothing per-project is needed.

---

## 1. Teammate types available

Any agent definition in `.claude/agents/` can back a teammate. Mention the type by name in the spawn
prompt. The definition's `tools` allowlist and `model` are honoured; its body is *appended* to the
teammate's system prompt.

| Type | Tools | Safe to own |
| --- | --- | --- |
| `performance-director` | Read, Glob, Grep, Write, Edit, Bash, PowerShell | `docs/perf/` |
| `gameplay-director` | same | `SYSTEMS.md`, `docs/data/` |
| `pixel-art-director` | Read, Glob, Grep, Write, Edit | art specs — never image files |
| `narrative-director` | same | narrative prose; ends visual work with an art brief |
| `ui-director` | + WebFetch, Artifact, Skill | `docs/ui/` + published HTML mockups |
| `claude` | everything | catch-all when no director fits |

None of the five declare a `model:`, so they inherit the **Default teammate model** from `/config`
(set to Sonnet). Name a model in the spawn prompt when a teammate needs Opus.

---

## 2. Recipes

### Diff review — three lenses on one branch

The safest first team: read-only, independent lenses, zero file contention.

```
Spawn three teammates to review the uncommitted diff on <branch>.
Name them perf, gameplay, and correctness.
- perf: use the performance-director agent type. Frame-time and allocation cost of the
  Mass processor changes at swarm scale.
- gameplay: use the gameplay-director agent type. Whether the changes match what GDD.md
  and SYSTEMS.md promise.
- correctness: a plain teammate. Lifetime/ownership bugs, Mass chunk-iteration errors,
  thread safety, anything that crashes PIE.
All three are read-only — no source edits, no builds, no editor launch. Have perf and
correctness agree on who owns any overlapping finding before reporting. Synthesize for me.
```

### Swarm perf investigation — competing hypotheses

For a frame-time regression whose cause isn't obvious. The adversarial framing is the point:
sequential investigation anchors on the first plausible theory and stops.

```
Frame time regressed to <X> ms at <N> entities. Spawn 4 teammates, each investigating a
different hypothesis: spatial query cost, avoidance/collision, the render bridge in
SwarmRenderActor, and fragment-layout cache behaviour. Have them message each other to
disprove each other's theories, like a scientific debate. Use Opus for the two that need
to read the Mass processor internals. Land the surviving consensus in docs/perf/.
```

### Feature slice — parallel directors, disjoint files

Works because these three never write the same file.

```
Spawn three teammates for the <feature> slice: narrative-director writing the fiction,
pixel-art-director writing the sprite spec, ui-director writing the screen spec and an
HTML mockup. Each owns only its own doc — narrative touches docs/narrative/, art touches
the brief, ui touches docs/ui/. Nobody edits GDD.md. Require plan approval from the
ui-director before it publishes an Artifact.
```

---

## 3. The rule that actually bites

**Give every teammate a disjoint set of files.** Two teammates editing the same file overwrite each
other — there's no merge. In this repo that means: only one teammate touches `GDD.md`, only one
touches `SYSTEMS.md`, and review teams should be read-only unless you've assigned ownership
explicitly.

Sizing: 3–5 teammates, 5–6 tasks each. Three focused teammates beat five scattered ones, and token
cost scales linearly per teammate.

---

## 4. Driving the team

In-process mode (the only mode available here — see §5):

| Key | Effect |
| --- | --- |
| ↑ / ↓ | select a teammate in the agent panel |
| Enter | open its transcript; typing sends it a message |
| Esc | interrupt the selected teammate's turn |
| `x` | stop the selected teammate |
| Ctrl+T | toggle the shared task list |

Idle rows hide 30 s after the whole panel goes idle — hidden is not stopped. Message the teammate by
name to bring its row back. More than three idle teammates collapse into one `N idle agents` row.

Shut one down by name: *"Ask the perf teammate to shut down."* Team directories clean up on session
exit; there's no manual teardown.

---

## 5. Gotchas specific to this setup

- **In-process only.** Split panes need tmux or iTerm2; Windows Terminal is unsupported. Don't set
  `teammateMode`.
- **`/resume` and `/rewind` do not restore in-process teammates.** After resuming, the lead may try
  to message ghosts. Tell it to spawn fresh ones.
- **No nested teams.** Teammates can't spawn teammates, and an in-process teammate's own subagents
  can't run in the background.
- **`skills` and `mcpServers` frontmatter is ignored for teammates** — they load skills and MCP from
  project + user settings like a normal session, rather than from the agent definition.
- **A teammate may report "no such tool" for MCP and still have it.** Observed 2026-07-26: a
  `performance-director` teammate first said it had no `unreal-mcp` tool, then later made live calls
  (`list_toolsets`, `describe_toolset`) against the server on port 9000. MCP tools appear to be
  deferred rather than blocked by the definition's `tools:` list, so a teammate that hasn't surfaced
  them yet will honestly report their absence. Don't design around the first answer — if a teammate
  needs MCP, say so in the spawn prompt so it goes looking. (An earlier version of this file claimed
  the `tools:` allowlist gates MCP outright. That was wrong.)
- **A teammate treats its spawn prompt as binding.** Telling it mid-run that a restriction is lifted
  does not reliably override an instruction like "do not launch the editor" in its original prompt —
  and it shouldn't. Re-scope by spawning a fresh teammate with the right prompt, or accept the
  refusal and do that leg yourself.
- **Permission prompts surface in the lead session**, not the teammate's. Approve them there. The
  existing allowlists in `.claude/settings.local.json` already cover most PowerShell/Bash/MCP calls.
- **One editor.** Only one teammate at a time should drive `unreal-mcp` or launch the Unreal editor —
  two teammates building or PIE-ing at once will fight over the same process.
- **Storage.** Team config `~/.claude/teams/session-*/` (deleted at session end, never hand-edit);
  task list `~/.claude/tasks/session-*/` (persists, retention follows `cleanupPeriodDays`).
- **Teammates load `CLAUDE.md` but not the lead's conversation history.** Put task-specific context
  in the spawn prompt.

---

## 6. Where the tasks come from

Don't invent a teammate's task list in the spawn prompt — take it from
`docs/backlog/INDEX.md`. Each task file carries a **paste-ready spawn prompt**, written
self-contained precisely because of §5's rule that teammates load `CLAUDE.md` but not the
lead's conversation history.

The backlog also mechanises the two rules in this document that actually bite:

- **§3, disjoint files.** Every task declares `owns:` globs. `py Scripts/backlog.py validate`
  fails if two simultaneously-active tasks claim overlapping paths, and `approve` refuses a
  transition that would create the collision. This is why `task-038` exists as its own task:
  five design specs each wanted to write `SYSTEMS.md`, so the shared write was split out.
- **§5, one editor.** Tasks declare `resources:` — `unreal-editor`, `mcp-9000`,
  `pixellab-credits`. The same check treats them as mutexes, so two teammates can never be
  approved into driving the editor at once. Credits are locked because they cost real money.

### Threading one project across the team

A goal too big for one teammate becomes a **fan**: sibling tasks sharing an `epic:` slug,
cut so their `owns:` globs are disjoint, plus a **join task** owning whatever shared file
they converge on with `depends-on` naming every sibling.

```
approve 44,45,46,47              one prompt, one collision dry-run over the whole fan
dispatch 44 --teammate palette   \
dispatch 45 --teammate hud        }  one dispatch + spawn per sibling, in parallel
dispatch 46 --teammate lore      /
dispatch 47 --teammate fold      refused until 44-46 close — that refusal is the schedule
```

`approve` takes the fan at once because approval is a judgment over the whole plan.
`dispatch` deliberately takes one id, because a teammate name identifies one spawned agent.
`py Scripts/backlog.py epic <slug>` prints where the fan stands and which siblings are
dispatchable right now.

The width is capped by §5, not by ambition: `unreal-editor`, `mcp-9000` and
`pixellab-credits` are global mutexes, so two siblings that both build cannot be approved
together. Specs fan wide; build work belongs in the join. And since teammates cannot spawn
teammates, the lead carries every dispatch and handback — 3–4 threads is the practical
ceiling, same as §3's sizing.

Dispatch is gated by `py Scripts/backlog.py dispatch <id> --teammate <name>`, which records
which teammate holds which task and **refuses any task that is not `approved` with its
dependencies closed**. Run it before spawning: if it refuses, no tokens have been spent. The
`teammate:` stamp is what lets a resumed session tell a live teammate from a §5 ghost.

Two workflows, one task store:

| | Intake | Who spawns |
| --- | --- | --- |
| `/backlog` | sweeps the repo for latent work → ranked queue → owner approves | the owner, or the lead when asked |
| `/host "<goal>"` | a goal the owner brings → clarify → one drafted task → plan presented → owner approves | the lead, immediately on approval |

The `host` agent proposes, ranks and drafts; it never approves and never spawns. Spawning
belongs to the lead session, always after an approval that `backlog_guard.py` prompted on.

## 7. When *not* to use a team

Sequential work, same-file edits, or anything with heavy dependencies between steps. Use subagents
or just one session — the coordination overhead and the per-teammate token cost aren't worth it.
Research, review, debugging with competing hypotheses, and cross-layer feature work are where teams
earn their keep.

## 8. Capturing evidence from a PIE session (task-048)

This repo's standard is on-screen evidence — a diff plus "it works" is not accepted. Getting a
sharp, correctly-posed screenshot of a PIE session an agent is driving over MCP used to be broken
three separate ways. One of the three is now fixed; the other two are dead ends, documented below
so nobody re-discovers them the hard way.

### The recipe

`Swarm.DebugShotAfter <seconds>` takes one screenshot that many seconds after `BeginPlay`, via a
`USceneCaptureComponent2D` on `ASwarmRenderActor` (`DebugCaptureComponent` — the actor and CVars
live in `ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp`). It needs **no OS focus**: a scene
capture issues its own render command instead of waiting for Slate to paint the game viewport, so
it fires correctly even while the editor window sits unfocused behind everything else.

```powershell
. "C:\Projects\ELVTRGAME\Scripts\ue-mcp.ps1"; Test-Mcp | Out-Null

# 1. Edit Saved/SwarmExecOnPlay.txt: set Swarm.DebugShotAfter to a few seconds past whatever your
#    scenario needs to settle. RESTORE IT TO 0 WHEN DONE — this file is live tuning, not a scratchpad.
#    Also want Swarm.DebugRender 0 (Niagara sprites) — see the warning below.

# 2. Start PIE and just wait through warmup; the shot fires and lands on disk during this call,
#    unattended, whether or not the editor window has focus.
$options = @{ bSimulate = $false; playMode = "PlayMode_InViewPort"; warmupSeconds = <ShotAfter + a few> }
Invoke-McpTool -ToolName "StartPIE" -Toolset "EditorToolset.EditorAppToolset" -Arguments @{ options = $options }

# 3. Find the file. It logs its own path, so grep the log instead of guessing a filename:
Invoke-McpTool -ToolName "GetLogEntries" -Toolset "EditorToolset.LogsToolset" `
  -Arguments @{ category = "LogTemp"; pattern = "SwarmDebug: capture written"; maxEntries = 5 }
# -> Saved/Screenshots/SwarmDebugShot_<timestamp>.png, at Swarm.DebugShotWidth x Swarm.DebugShotHeight
#    (default 1920x1080 — independent of the actual window/viewport size on screen).
```

Read the PNG with the `Read` tool once you have the path.

**`Swarm.DebugRender` must be `0` (Niagara sprites) for the swarm to actually appear in the
shot.** `DrawDebugSolidBox` primitives (`Swarm.DebugRender 1`, the debug-box path) are **not
visible to any scene capture** — verified empirically both for this component and, earlier and
independently, for a "unit cam" `SceneCaptureComponent2D` experiment. With `DebugRender 1` the
capture still succeeds and still shows the flame pool, just with zero units, which reads like a
bug in the capture rather than a mode mismatch — the log line names which mode was active for
exactly this reason. (The `Saved/SwarmExecOnPlay.txt` duplicate-`Swarm.DebugRender`-lines
landmine this paragraph used to flag was fixed post-task, 2026-07-27 — one line now, under
DEBUG/RENDER/CAPTURE, `0`, with the stale "Niagara draws nothing" justification marked RETRACTED.)

**The capture is the RAW SCENE, not the styled game view — do not use it to judge art.** Despite
`CaptureSource = SCS_FinalColorLDR` (the same setting that, per an earlier independent "unit cam"
investigation, DOES normally bake the demichrome pass in), the demo capture above came back with
the flame pool as a handful of flat, blown-out, near-white rectangular facets instead of the
dithered 4-value Bayer-quantized pool the player actually sees — none of the visible pixels sample
to a colour on the locked `demichrome` ramp (`GSwarmPalettePresets` in `SwarmRenderActor.cpp`).
So:
- **Good for**: geometry, formation shapes, ranks, spawn arcs, positions, counts, framing —
  anything structural, which is everything task-048 and task-047 needed it for.
- **Not valid for**: judging palette, dither, contrast, or anything the post-process decides. A
  teammate reporting "the palette reads correctly" from one of these shots would be reporting on
  an image the player never sees.

Not fixed here (out of task-048's scope by the owner's instruction — record only). Two candidate
causes, neither confirmed, both cheap to test in a follow-up task before assuming a deeper
architectural block:
1. **Exposure convergence.** `DebugCaptureComponent` does one manual `CaptureScene()` call with no
   per-frame history, so it has no eye-adaptation state built up the way the live game camera
   (rendering continuously) does. A single-shot capture likely meters/tonemaps against just that
   frame, which could easily crush the visible range into the top palette bucket instead of the
   full graduated ramp. Fix to try: force a fixed/manual exposure on the capture's
   `PostProcessSettings` that matches the live camera's converged value, instead of letting it
   auto-expose cold.
2. **Double gamma on export.** The render target is created `RTF_RGBA8_SRGB`, but
   `SCS_FinalColorLDR` output is *already* gamma-encoded (it's the tonemapped, post-processed
   final color). Writing already-encoded LDR color into an sRGB-flagged target risks a second
   gamma pass on export, which over-brightens highlights disproportionately — consistent with
   "blown out to flat white" specifically, rather than "post-process absent." Fix to try:
   `RTF_RGBA8` (no `_SRGB`) instead.

### 8c. "git diff on SwarmExecOnPlay.txt is empty" proves NOTHING

`ELVTR/Saved/SwarmExecOnPlay.txt` is **gitignored** — `.gitignore:5` excludes all of
`ELVTR/Saved/`. It is not tracked, so `git diff` on it is **always empty**, whatever you did
to it.

Four separate agents in one session cited that empty diff as proof they had cleanly restored
the owner's tuning after a test run. None were being careless in any other respect; they all
reached for a check that cannot fail. **Do not use it, and do not accept it as evidence.**

This matters more than a bookkeeping nit: that file **overrides the C++ defaults at play
time**, so it is effectively the game's real configuration — and every value in it is a
tuning decision someone made deliberately.

**The real check:** diff the working copy against the tracked canonical copy at
`ELVTR/Config/SwarmExecOnPlay.canonical.txt`.

```
diff ELVTR/Config/SwarmExecOnPlay.canonical.txt ELVTR/Saved/SwarmExecOnPlay.txt
```

If you intentionally changed tuning, update the canonical copy and commit it. If you only
added a temporary line for a test, that diff is what proves you removed it again.

Values marked `(owner-tuned)` in that file differ deliberately from the C++ default and must
survive any test you run. Never "correct" one back toward its source default.

### 8b. Never take a raw desktop-region screenshot on this machine

**Use the MCP-scoped `CaptureEditorImage` / `CaptureViewport` tools, never a raw
screen-coordinate grab** (`System.Drawing`, PowerShell screen capture, or equivalent).

This is not a style preference. task-050 tried a raw desktop-region grab and it captured
**unrelated windows belonging to other sessions on this shared machine** — another agent's
chat and terminal content, and a different project entirely. The teammate deleted the image
immediately, never used it, and switched to the scoped tool; handled correctly, but the
image existed on disk for a moment and it did not have to.

The MCP capture tools are scoped to the editor window. A screen-coordinate grab captures
whatever happens to be on screen, which on this box includes other people's work.

If the editor window is too small in the capture, **minimise the other editor panels** so the
tool's resolution budget goes to the PIE window — task-050 got from ~1280×340 to ~1280×688
that way. Do not reach for a raw grab to get more pixels.

### 8a. Unfocused PIE runs at ~3fps — set this before ANY timing measurement

**`EditorPerformanceSettings.bThrottleCPUWhenNotForeground` defaults to `true`, which caps the
whole engine to roughly 3fps the moment a PIE session starts without OS focus.** Every
agent-driven PIE run in this repo is unfocused by definition, so this is on by default for all
of us.

Found by task-021 while measuring frame cost: its first capture came back a flat 333ms/frame
with **no correlation to entity count at all**, which is the signature to watch for. Fix, on the
live CDO — no restart and no config edit needed:

```
ObjectTools.set_properties on EditorPerformanceSettings CDO:
    bThrottleCPUWhenNotForeground = false
```

**Restore it afterwards.** task-021 did.

**This retroactively explains two earlier misdiagnoses, and both were recorded as facts before
anyone found the real cause:**
- task-045 concluded the PIE window "freezes when it does not have focus" and closed with its
  yaw clamp unverified.
- task-047 took two captures ~50s of sim time apart, found them near-identical, and concluded
  the same. It could not certify the brood ranks it had just built.

Neither was frozen. Both were throttled to ~3fps, so ~50s of wall clock advanced almost nothing
on screen. **If you are timing anything, or waiting on simulated time to pass, set this first or
your numbers and your screenshots are fiction.**

No evidence currently rules out a real architectural gap (e.g. the demichrome material bound in a
way scene captures can't see) — grep found no C++ site that attaches `M_PP_Demichrome` as a
blendable (`AddCachedPPBlend`, `WeightedBlendables`, etc.), so it's presumably bound via a
`PostProcessVolume` placed in the level, which standard engine behaviour extends to *any* view at
that location, captures included — meaning there's no obvious reason the two causes above
shouldn't be sufficient. But that's inference, not verification; the honest answer is "probably
reachable, two concrete things to try first," not "confirmed reachable."

### Why this exists — the three walls, and why two are dead ends

1. **`Swarm.DebugShotAfter` didn't fire unfocused.** It used to call `FScreenshotRequest::
   RequestScreenshot`, which is fulfilled inside `UGameViewportClient::Draw()` — a call that Slate
   simply never makes for an unfocused/occluded window. task-047 saw a full auto-fight run to
   completion, sim ticking the whole time, with the request queued and never fulfilled. **Fixed**:
   `TakeDebugShot()` now drives `DebugCaptureComponent->CaptureScene()` directly, which needs no
   window paint at all.
2. **The MCP `CaptureViewport` tool shows the flame pool but zero swarm.** Root cause, confirmed
   by capturing both back to back during a live PIE session: `CaptureViewport` renders the
   **persistent editor world**, not the transient PIE world — the capture came back with
   editor-only actor gizmo icons (a lightbulb, a gamepad) and a flat unlit grid, none of which
   exist in the actual game. The swarm actor and its Niagara/debug-box output only exist in the
   PIE world, so this tool structurally cannot see them no matter the camera pose passed in. Not
   fixable from `SwarmRenderActor.cpp` — it isn't a swarm-side bug. **Left alone**; use the
   `Swarm.DebugShotAfter` recipe above instead, which captures the actual PIE world by living
   inside an actor that's spawned into it.
3. **Desktop capture (`CaptureEditorImage`) is too low-res to certify detail.** Confirmed:
   during `PlayMode_InViewPort`, PIE renders inside the level-viewport panel alongside the
   Outliner/Details/Breadboard docks — in one measured capture the actual game view occupied
   roughly a 380x230px corner of a 1280x446 desktop screenshot. Units land at single-digit
   pixels. **Left alone**; `Swarm.DebugShotAfter`'s render target is sized by
   `Swarm.DebugShotWidth`/`Height` (default 1920x1080) independent of any window, which is a
   strictly better fix than trying to enlarge the PIE panel on screen.

### Landmine, confirmed again this task

Clicking into the PIE viewport via `SlateInspectorToolset` **ejects the PIE session outright**
(first hit in task-047). Don't drive PIE state through it; use `EditorToolset.EditorAppToolset`
(`StartPIE`/`StopPIE`/`IsPIERunning`) and the `Saved/SwarmExecOnPlay.txt` exec-file hook for
everything console-command-shaped, same as the recipe above.

### A tooling note: which MCP surface actually works

In this task's session, none of the `mcp__unreal-mcp__*` tools (`list_toolsets`, `call_tool`, etc.)
were reachable via `ToolSearch` under any query, despite being declared in `.mcp.json` and allowed
in `.claude/settings.local.json` — they never surfaced as deferred tools at all, not even the
"reports absent, actually present" case §5 already documents for teammates. What worked
unconditionally was `Scripts/ue-mcp.ps1`'s `Invoke-McpTool`, which talks to the same HTTP JSON-RPC
endpoint directly (`http://127.0.0.1:<port>/mcp`) via `Invoke-WebRequest`, bypassing the Claude MCP
client entirely — the same approach `ue-iterate.ps1`/`ue-relaunch.ps1` already use for
`CompileLiveCoding`. If a teammate's `mcp__unreal-mcp__*` tools don't show up, don't conclude MCP
is down — check with `Test-Mcp` from that script before assuming the editor/plugin isn't running.
