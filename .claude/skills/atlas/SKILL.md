---
name: atlas
description: Add, remove or verify a variant in Kindled's composite sprite atlases (team-units, enemy-units) without hand-typing the 16-entry-per-variant frames block and frame_map matrix. Wraps Scripts/art/atlas.py plus the pack and UE-import steps, which are now drivable end to end over MCP. Use when the user runs /atlas or asks to add a unit look/variant to a sheet, grow or shrink an atlas, check an atlas for drift, or asks why a sprite sheet and the sim disagree about what a row means.
---

# atlas — the sheet matrix, generated

A composite request carries two blocks nobody should type: `composite.sources[].frames`
(16 entries per variant) and `output.frame_map` (16 cells per variant). `team-units.json`
is 24 variants — **768 mechanical lines for 24 real facts**. Both are fully derivable:

```
frames["<direction>.walk<w>"]           = "<rotations-dir>/<direction>.png"   (walk0 == walk1)
frame_map[col + (variant*2 + w) * 8]    = "<prefix>:<direction>.walk<w>"
directions, in column order: south south-east east north-east north north-west west south-west
```

Verified against all three shipped atlases 2026-07-31: 688 frame entries and 688
`frame_map` entries reproduced with zero mismatches. `Scripts/art/atlas.py` owns the
derivation; the ordered source list is the only truth.

## Commands

```
py Scripts/art/atlas.py check --all              # drift check, run this first and last
py Scripts/art/atlas.py check team-units
py Scripts/art/atlas.py sync  team-units         # rewrite the derived blocks in place
py Scripts/art/atlas.py add   team-units --prefix archer-foo \
      --rotations RawArt/Renders/archer-foo/raw/state00/rotations
py Scripts/art/atlas.py remove team-units --prefix archer-foo
```

`add` appends by default. **Appending is the safe move** — `--at N` renumbers every later
variant, and the index is what `Swarm.TeamVariantWeights`, task-095's knight stat rows and
`brood-variants.json` weights are keyed on. The script warns but cannot stop you.

## The four things that must agree

This is the whole failure mode: change one, the horde decodes garbage. `check` verifies
the first three and prints the fourth.

| # | Place | Checked? |
|---|---|---|
| 1 | `composite.sources[].frames` + `output.frame_map` | yes, re-derived |
| 2 | `output.grid` = `[8, variants*2]` | yes |
| 3 | `SwarmSheet::Team` / `::Enemy` in `SwarmFragments.h` | yes, read + compared — **never edited**, bump it yourself |
| 4 | the emitter's Sub UV in `NS_Swarm.uasset` | no — a binary asset. `check` prints the value it must hold |

For #4, **read it back from the asset; never assume the write took** (task-059's own
instruction, and the same trap as the material Custom-node writes).

## Full pass, MCP end to end

Steps 1-2 are local scripts. Step 3 works over MCP as of 2026-07-31: the sandbox still
cannot `import unreal`, but the editor console's `py` reaches the real interpreter, and
`KindledConsoleToolset.Exec` reaches the console.

1. `py Scripts/art/atlas.py add <id> --prefix … --rotations …`
2. `py Scripts/art/pixelpipe.py pack <id>` — writes `RawArt/Sheets/T_*.png`
3. `Exec('py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" <id>')`
4. Widen the emitter's Sub UV, then re-read it.
5. `py Scripts/art/atlas.py check --all`

**`Exec` returns an empty string for `py`.** Output goes to the log, so read it back:
`LogsToolset.GetLogEntries(Category:"", Pattern:"SPRITE import")`. `Category:""` is
required — it defaults to `LogsToolset` and errors otherwise. And send MCP calls **one at
a time**; two in one message closes the socket.

## Do not

- Do not hand-edit `frames` or `frame_map`. `sync` overwrites both, and a hand edit that
  drifts is exactly what `check` exists to catch.
- Do not reorder a block to "tidy" it. Indices are load-bearing.
- Do not use `Scripts/build_swarm_sheet.py` — superseded 2026-07-25, writes to a different
  path, and having both live is how the stale sheet gets imported.
- Do not assume the tree is yours. These requests are actively edited by other sessions —
  run `check --all` before you start and diff before you commit.
