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

## 6. When *not* to use a team

Sequential work, same-file edits, or anything with heavy dependencies between steps. Use subagents
or just one session — the coordination overhead and the per-teammate token cost aren't worth it.
Research, review, debugging with competing hypotheses, and cross-layer feature work are where teams
earn their keep.
