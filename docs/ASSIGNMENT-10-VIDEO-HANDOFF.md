# Assignment #10 — pipeline-run video, do-over handoff

Goal: record video evidence of the pipeline generating an asset **that lands in the
playable game on camera**. The first take failed the point: the generated gargoyle
went to `RawArt/Renders/assignment10-demo/gargoyle_green_flame.png` but was never
wired into the build, so the video showed generation without traceability.

**2026-09-01 do-over: the gargoyle prop is replaced by a new ENEMY UNIT** made through
the forge web tool. One approve click on the roster page now wires the unit into the
playable Godot build — see "The new forge feature" below.

**2026-09-01 owner verdict: enemy-armored is OUT** — they're cyborgs, not armored
units, wrong world. All 11 of its selects records are denied (parked, not deleted).
The demo family is **melee-undead**, the enemy line the game actually ships; it got a
`family.json` (base: the "Hallam (Vanguard)" undead soldier) so the family page
prefills and the approve mirror fires.

## Already done (do not redo)

- Delivery package: `Downloads\Aaron Low - Assignment 10\` with the zip, submission
  PDF, `Kindled-Pipeline-Ledger.pdf`, retrospective PDF, thumbnail. Sources:
  `docs/Kindled-Pipeline-Assignment-10.html`, `docs/Kindled-Retrospective.html`
  (both untracked — commit them when this lands).
- Repo `Hawaiianspawn/ELVTRGAME` flipped **public**; master pushed.
- Playable page live: https://hawaiianspawn.itch.io/kindeled
- Submission PDF has one placeholder left: `[VIDEO LINK — TO INSERT]`.
- First-take leftover `RawArt/Renders/assignment10-demo/`: owner REJECTED 2026-09-01
  ("gargoyle is gone") — deleted, nothing to keep.
- **The new forge feature is implemented and rehearsed end to end** (2026-09-01),
  twice: first on enemy-armored `v5_slabshield` (before the cyborg verdict), then on
  melee-undead `z1-tilt-left` — approve wired units/roster/waves, queued a template
  walk clip, auto-packed, probe showed the unit fighting in hall 1. Rehearsal data
  edits were reverted; walk frames and forge-jobs records stay as provenance.

## The new forge feature (Scripts/art/forge.py)

One page (`/`): in-game units with live stats/SFX editing on top, candidates
(approve/deny + **implement**) at the bottom, denied looks on `/denied`. The
**Implement** button on a candidate card (`implement_look`) does, in one click:

1. **Records the approve** — Selects copy + record (the Aseprite layer).
2. **Wires the Godot game** — three idempotent JSON edits: `godot/data/units.json`
   entry cloned from the `"armored"` stat template (tunable in place on the page),
   `godot/data/roster.json` art line, and a wave-0 `enemies[]` entry (count 12).
3. **Queues a walk clip** — `POST /animate-character` (template mode, south only) on
   the look's own character; a watcher thread polls, lands frames at
   `raw/<slug>/walk_south/`, and re-runs `godot_pack.py` automatically. A look with
   no recorded character id still implements — walk is reported FAILED, bob fallback.

Approve/deny alone are the verdict only. Deny on an in-game unit unwires it
(units/roster/waves) and parks its look; Army.gd's four allies + heroes are
protected. Implementing twice is a no-op. `--selftest` covers wire/unwire/implement.

## Prep OFF camera (do all of this before recording)

1. Confirm clean tree: only untracked docs + forge feature staged/committed as wanted.
2. Check PixelLab balance (family page header). A state is ~20-40 credits, the walk
   clip ~1.
3. Pick the on-camera slug + edit description — fresh slug only (`start_state`
   refuses one with renders on disk). Undead-keep register, no machinery, no glow:
   - `bone-render` — "Same undead soldier, both arms ending in long jagged bone
     blades, hunched forward ready to lunge"
   - `grave-hulk` — "Same undead soldier, hunched massive under a cracked stone
     grave slab strapped across the shoulders"
   The tail clause (head/legs visible, palette match, no glow) is appended
   automatically.
4. Close any running Kindled window (pack stops it anyway, but cleaner on camera).
5. Caution in the Candidates section: looks already in the game are hidden, but
   approving a *sibling* of a shipped look (another z1 variant, say) wires it as a
   new enemy type. Idempotent and reversible (deny it again), but approve
   deliberately.

## The take (user records screen, says "go")

1. `py Scripts/art/forge.py --family melee-undead` — browser opens the **units page**
   (`/`): everything in the game on top with live stats/SFX editing, candidates at
   the bottom, denied looks exiled to `/denied`. Scroll it, narrate the model. (Read
   the printed URL if port 8770 was busy.)
2. Nav to the **family page** (`/family/melee-undead`) — base id prefilled, balance
   and cost estimate live. Type the slug + edit description, **Generate**. ~2–4 min;
   narrate the measured contact sheet (aspect/solidity/asymmetry across all 8
   rotations) while it renders. Page auto-reloads when the rotations land.
3. New card appears: south render, flat outline, measured band, nearest-sibling Δ.
4. Back on the **units page**: the new look sits in Candidates. Pick a branch
   (existing, or new e.g. "Undead Keep"), click **Implement** — the money click: it
   records the approve, wires units/roster/waves + queues the walk clip, and on
   reload the unit has moved UP into the in-game section. (Approve/deny alone are
   just the verdict now — implement is what makes a unit.)
5. Customize it right there while the walk clip generates (~2 min, the watcher lands
   frames and repacks on its own): tweak hp/dmg/speed, pick attack/hit/death sounds —
   every edit saves straight into `godot/data/units.json` on camera.
6. `powershell -File Scripts\godot-run.ps1 -Pack` — pack output scrolls (the new slug
   appears with its `walk_south` row), headless reimport, game launches.
7. Character select → [Space] into hall 1 — wave 0 spawns 12 of the new enemy walking
   their generated-on-camera loop among the ooze and undead. Stop recording.

Fallback if generation misbehaves on camera: `z1-tilt-left` already has walk frames
on disk — clicking **Implement** on it wires instantly (walk dedupes to "already on
disk") and steps 6–7 proceed the same.

Verify without playing: `powershell -File Scripts\godot-run.ps1 -Probe "battle,12,wave=0"`
(wave is 0-indexed; wave=0 is hall 1) prints a screenshot path.

## After the video

1. User uploads (unlisted YouTube fine) and pastes the link.
2. Edit `docs/Kindled-Pipeline-Assignment-10.html`: replace
   `[VIDEO LINK — TO INSERT]` with the link.
3. Re-render + re-zip:
   ```powershell
   $pkg = "$env:USERPROFILE\Downloads\Aaron Low - Assignment 10"
   Copy-Item docs\Kindled-Pipeline-Assignment-10.html "$pkg\" -Force
   & "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe" --headless=new --disable-gpu --no-pdf-header-footer --print-to-pdf="$pkg\Aaron Low - Assignment 10 - Complete AI Dev Pipeline.pdf" "file:///$($pkg -replace '\\','/')/Kindled-Pipeline-Assignment-10.html"
   Compress-Archive -Path "$pkg\Aaron Low - Assignment 10 - Complete AI Dev Pipeline.pdf","$pkg\Kindled-Retrospective.pdf","$pkg\Kindled-Pipeline-Ledger.pdf","$pkg\itch-thumbnail.png" -DestinationPath "$pkg\Aaron Low - Assignment 10.zip" -Force
   ```
   (Chrome headless silently produced nothing here; Edge `--headless=new` works.)
4. Decide with the owner whether the new enemy stays in the shipped build (it will be
   in roster + wave 0 after the take). If it stays and the web build should match:
   `Scripts\release.ps1` + `Scripts\itch-push.ps1` (needs `$env:ITCH_API_KEY`).
5. Commit: the two submission HTML sources, this handoff, the forge.py feature, the
   new `docs/data/art/families/melee-undead/family.json`, the selects denies, and
   the roster/units/waves data. Repo commit style: sentence subject, no type prefix,
   body ≤250 chars only if the why isn't obvious.

## Deadline

Due **tonight, 1 Sept 2026, 11:59 PM ET**. The playable-link gate is already
satisfied; the video is the last open deliverable.
