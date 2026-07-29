---
id: 062
title: Bring the art tooling onto the full-colour reversal — palette.json still enforces a retired lock
status: done
agent: claude
owns: ["docs/data/art/palette.json", "Scripts/art/coverage.py", "Scripts/art/pixelpipe.py", "Scripts/art/authored_states.py", "Scripts/art/silhouette_report.py", ".claude/skills/art-coverage/SKILL.md", ".claude/skills/sprite/SKILL.md"]
resources: []
depends-on: []
epic: ""
evidence: "`/art-coverage` run against the existing full-colour unit sprites (T_Soldier_Knight_*, T_Soldier_Archer_*) reports zero `off-ramp` findings where today it would flag every one of them, and `palette.json` no longer describes itself as enforcing the 4-value lock. Plus a stated before/after count of off-ramp findings across the current roster."
score: {feel: 2, risk: 1, cost: 2}
source: user
teammate: palette-tooling
decided: "2026-07-28 done"
---

## Why now

On 2026-07-28 the owner reversed the colour gate. `Emberkeep.Quantize` is `0`; the game
ships full colour. `task-057` implemented that **in the engine** — the bypass, the N-value
quantizer, the Breadboard rows — and it owned `docs/data/art/palette.json`, but it only
added the trial-palette and `palette_steps` data. The **art tooling was never brought
along**, and it is now actively wrong in three ways:

1. **`palette.json` describes itself as the enforcement mechanism for a retired rule.** Its
   `updated` field reads `2026-07-25` and its `note` reads: *"Machine-readable form of the
   palette locked by the 2026-07-12 reset... this file exists so the pipeline can ENFORCE
   it. If they disagree, the markdown wins and this file is the bug."* The markdown now
   disagrees — `aesthetic-direction.md` opens with the reversal. **By its own stated rule,
   this file is currently the bug.**

2. **`coverage.py` flags every full-colour sprite as broken.** Its `off-ramp` finding is an
   exact-hex membership test against the 4 Demichrome values (`coverage.py:80-111`), so
   every pixel of the full-colour PixelLab roster — the knight, the archer, the retinue
   character, the brood-ooze family — registers as a defect. The audit that exists to tell
   you what art is missing now reports the art you deliberately made as wrong.

3. **`demichrome-4` is the hardcoded default in ~7 call sites** across `pixelpipe.py`
   (lines 155, 206, 434, 515, 831, 1062, 1179, 1346) and `coverage.py`. A request that does
   not name a palette gets the retired one silently.

Nothing breaks in the running game — this is tooling, not runtime. What breaks is every
judgement the tooling makes about art from here on, which is why it scores `gate: 2`: it
does not block a gate item itself, it corrupts the instrument you would use to tell whether
one is met.

**This task deliberately does NOT decide the new colour standard.** There isn't one written
down, `task-039` surfaced four open questions about hero hues that are the owner's to
settle, and answering them piecemeal is how a third palette pass happens. The job here is
narrower and fully separable: **stop enforcing the retired rule.** "No enforced ramp" is a
complete, correct description of full colour and needs no further decision.

## Done when

- **`palette.json` tells the truth.** `demichrome-4` is preserved in full — it is history,
  not garbage, and the repo keeps its record — but relabelled as historical/not-enforced
  rather than the locked current palette. The `note` and `updated` fields no longer assert
  a lock that was lifted. The `retired_hexes` registry stays exactly as-is; it is still the
  correct answer to "was this hex ever retired."
- **The off-ramp check stops firing on full colour.** Whether that is opt-in, informational,
  or gated on a request naming a palette is your call — but `/art-coverage` on the current
  roster must stop reporting the deliberate full-colour sprites as defects. Say which
  mechanism you chose and why.
- **No call site silently defaults to `demichrome-4`.** A request that names a palette still
  gets it — the 4-value path must keep working for anything that explicitly asks, including
  the `light_shift` key-ladder that `task-039` just re-pointed six art specs at. Removing
  the default must not remove the capability.
- **The two skill docs match the code.** `art-coverage/SKILL.md` and `sprite/SKILL.md` both
  describe the locked 4-value ramp as current; correct them without deleting their history.
- **Measured, not asserted.** Report the off-ramp finding count across the current roster
  before and after.

## Spawn prompt

```
You are executing task-062 in Kindled/Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch.

THE PROBLEM
On 2026-07-28 the owner reversed the colour gate: Emberkeep.Quantize is 0 and the game
ships FULL COLOUR. Read the AMENDMENT 2026-07-28 blockquote at the top of
docs/art/aesthetic-direction.md — that is the authority.

task-057 implemented the reversal in the ENGINE and is closed. But the ART TOOLING still
enforces the retired 4-value Demichrome lock:

1. docs/data/art/palette.json is dated 2026-07-25 and its own note says it exists "so the
   pipeline can ENFORCE" the 2026-07-12 lock, and that "if they disagree, the markdown wins
   and this file is the bug." The markdown now disagrees. This file is currently the bug,
   by its own rule.
2. Scripts/art/coverage.py's "off-ramp" finding is an exact-hex membership test against the
   4 Demichrome values (see ~lines 80-111). Every full-colour PixelLab sprite in the
   project — T_Soldier_Knight_*, T_Soldier_Archer_*, the retinue character, the brood-ooze
   family — fails it. The audit reports deliberate art as defective.
3. "demichrome-4" is a hardcoded default in ~7 call sites in Scripts/art/pixelpipe.py
   (around lines 155, 206, 434, 515, 831, 1062, 1179, 1346) and in coverage.py.

WHAT YOU ARE AND ARE NOT DECIDING

You are NOT inventing the new colour standard. There isn't one written down. task-039 just
surfaced four open questions about whether hero classes regain individual hues, and those
are the owner's to settle. Do not answer them, do not imply an answer, do not invent
replacement hexes or a new named palette.

You ARE removing the enforcement of a rule that was lifted. "There is no enforced ramp" is
a complete and correct description of full colour. That is separable from the open
questions and is the entire job.

WHAT YOU OWN
  docs/data/art/palette.json
  Scripts/art/coverage.py
  Scripts/art/pixelpipe.py
  Scripts/art/authored_states.py
  Scripts/art/silhouette_report.py
  .claude/skills/art-coverage/SKILL.md
  .claude/skills/sprite/SKILL.md

Nothing else. In particular do NOT touch docs/art/** — task-039 just finished repairing six
specs there and its work is awaiting the owner's review. Do not touch ELVTR/Source or
ELVTR/Content; this is tooling only and must not change the running game.

PRESERVE, DO NOT DELETE
- Keep the demichrome-4 palette definition in full. It is history and this repo keeps its
  record (see docs/GDD-TODO.md and the pattern in aesthetic-direction.md). Relabel it as
  historical / not-enforced. Do not delete it.
- Keep retired_hexes exactly as it is. It is still the correct answer to "was this hex ever
  retired," and Scripts + the pixel-art-director rely on it.
- KEEP THE 4-VALUE PATH WORKING for anything that explicitly asks for it. Removing a
  DEFAULT is not the same as removing a CAPABILITY. task-039 just re-pointed six art specs
  at palette.json KEYS (dark/steel/bone/pale) and at the light_shift key-ladder — that must
  still resolve. If you break it, you have broken the work of the task that ran immediately
  before you.

HOW TO STOP THE OFF-RAMP CHECK FIRING — your engineering call
Options include making it opt-in, making it informational rather than a finding, or gating
it on a request explicitly naming a palette. Pick one, implement it, and say why. The bar
is: /art-coverage against the current roster stops reporting deliberate full-colour sprites
as defects, without losing the ability to check conformance for anything that has opted in.

VERIFY BY RUNNING, NOT BY READING
Run the coverage audit before your change and after it, and report the off-ramp finding
count both ways across the current roster. A claim that it "should now pass" is not
evidence. If a script cannot run in your environment, say so plainly rather than asserting
an outcome.

Also confirm you have not broken the explicit-palette path — exercise it once against
something that names demichrome-4 and show it still enforces.

HAND BACK
The before/after off-ramp counts, which mechanism you chose for the off-ramp check and why,
confirmation the explicit 4-value path still works, and anything you found that is stale in
the same way but outside your owns list (flag it, do not fix it). Do not attempt to change
your own task status — the lead owns the closing transitions.
```
