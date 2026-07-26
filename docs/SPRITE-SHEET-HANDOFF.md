# Sprite sheet handoff — attack + hit frames (2026-07-25)

**Art is DONE and the C++ is written. What remains is editor work + a build.**
This session could not do either: `unreal-mcp` answers on `:9000` but its tools never
registered, and the editor being open blocks UBT via Live Coding.

## Status

| Step | State |
|---|---|
| PixelLab art generated | **done** — `RawArt/Renders/swarm-units-v1/` |
| Sheet assembled + palette-snapped | **done** — `RawArt/T_Swarm_2bit.png`, 272×136 |
| C++ index widened (4×2, shared decode) | **done, NOT COMPILED** |
| Texture reimport in editor | **todo** |
| `NS_Swarm` Sub UV 2×2 → 4×2 | **todo** |
| `Swarm.DebugRender 0` + PIE check | **todo** |

## The sheet

`RawArt/T_Swarm_2bit.png` — 272 × 136, 68px cells, **4 columns × 2 rows**:

```
col:    0        1        2       3
      walk0    walk1   ATTACK   HIT      <- row 0 = brood
      walk0    walk1   ATTACK   HIT      <- row 1 = retinue
```

Verified to contain **exactly** the four locked Demichrome hexes and nothing else
(`#211e20` 3081px · `#555568` 722px · `#a0a08b` 352px · `#e9efec` 97px).

Frames chosen (all sources retained under `RawArt/Renders/swarm-units-v1/`, so this is
a one-line re-run of `Scripts/build_swarm_sheet.py` to change):

| Cell | Source |
|---|---|
| brood walk0 / walk1 | `walk/south/frame_001`, `frame_003` |
| brood attack | `attack_v3/south/frame_003` — crouched, horns forward, pale mouth |
| brood hit | `hit_v3/south/frame_003` — horns and arms flung wide |
| retinue walk0 / walk1 | `walk/south/frame_001`, `frame_003` |
| retinue attack | `hit_v3/south/frame_004` — club raised, arms out |
| retinue hit | `attack_v3/south/frame_003` — crouched low, arms tucked |

**The retinue attack/hit sources look swapped and are not.** The `taking-punch`-derived
frame turned out to be the only one where the club reads as a clear raised silhouette,
and the `cross-punch`-derived one reads as absorbing a blow. Chosen on how they read at
sprite scale, not on which template produced them.

## Why the poses were regenerated in v3

The first pass used template animations (`cross-punch`, `taking-punch`). They **broke
character identity**: both characters rotated to profile, the soldier's helmet became
hair, and the brood lost its horns and grew a tail. Silhouette is the load-bearing
identity mechanism under strict global palette
(`docs/art/npc-silhouette-brief.md`), so those were rejected. v3 mode anchors to the
character's own south rotation and preserved helmet, horns, and front-facing framing.
The template versions are still on disk under `animations/attack/` and `animations/hit/`
(kept per the retention rule in `docs/PIXELLAB-MCP.md`) — reject them explicitly when
you've seen them.

## Remaining editor steps

1. **Reimport the texture.** `Content/Spike1/T_Swarm_2bit` → right-click → Reimport, or
   re-drag `RawArt/T_Swarm_2bit.png`. Confirm **Filter = Nearest**, NoMipmaps, sRGB on
   (`ELVTR/SETUP-EDITOR.md` §1). The texture changes shape (was a small 2×2), so check
   the import didn't reset the filter.
2. **`NS_Swarm` → Sprite Renderer → Sub UV = 4 × 2.** Was 2 × 2. Nothing warns you if
   this is wrong; every unit just wears the wrong frame.
3. **Build.** Close the editor first — a new `USTRUCT` was added earlier this session and
   Live Coding will report success then crash the next PIE:
   ```
   & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" `
       ELVTREditor Win64 Development -Project="C:\Projects\ELVTRGAME\ELVTR\ELVTR.uproject" -WaitMutex
   ```
4. **`Swarm.DebugRender 0`** — the debug boxes are still the default renderer, so the
   sprites are invisible until this is flipped. This is the step that actually shows the
   attack frames.
5. Verify in PIE, then update `docs/GATE1-FUN-PROTOTYPE.md` §3b (drop its "no sprite
   frame yet / the swing is a 4px lean" caveat) and §5's known-gaps line.

## Owner calls still open

- **The enemy has no canon.** `docs/narrative/FLAME-FOUNDATION.md` leaves "does the dark
  have monsters, or is the dark itself the enemy?" open and states "No antagonist." The
  brood sprite is an explicit **placeholder** built to satisfy the readability contract in
  `npc-silhouette-brief.md` (c) — flat Dark, Pale only as eyes — not a canon proposal.
- **The brood attack frame spends Pale on an open mouth.** The brief reserves Pale on a
  Quiet-type unit to "a bare eye-dot (or small irregular cluster)". A pale mouth is a new
  Pale shape. It is the single most readable attack tell on an all-black silhouette, and
  the brief's restriction is written about units *at rest* — but it is a deviation and
  wants a ruling.
- **Friendly-NPC framing conflict.** `npc-silhouette-brief.md`'s style lock puts friendly
  NPCs (militia included) in a *bust-forward icon* composition, explicitly "not the 3/4 low
  top-down walking-sprite view". A bust icon cannot walk or swing. The same brief's
  horde-scale line ("a block of visibly uneven small silhouettes... holding a line")
  describes walking units, so these were authored as walking sprites and the bust lock read
  as governing the avatar/portrait register.
- **These are 4-direction characters used as 1 direction.** The sim only flips in X
  (`SwarmAnim::FlipBit`), so only the south rotation is in the sheet. East/north/west were
  generated and are on disk — wiring real facing is the `RENDERING-LIGHTING.md` §4a
  directional-bucket work, still open.
- ~~**Unit cam shows one static cell for every soldier.**~~ **CLOSED 2026-07-25.** Each
  billboard now carries its own `Cell`, taken from `SwarmSheet::CellForBits()` — the same
  call the world renderer makes — so the panel plays per-unit walk/attack/hit and draws
  brood from their own row instead of the old flat red quad. `SpriteCell` is gone;
  `Emberkeep.UnitCamProj.BroodTint` (default 0) is the only remaining override, and it just
  washes the reserved red back over brood sprites if you want the old high-contrast read.
  Fixed on the way: the brush's `ImageSize` was the whole sheet rather than one cell, so
  every unit in the panel was drawn at the sheet's 2:1 aspect — twice as wide as tall.

## Cell-index defaults that MOVED

The retinue row no longer starts at cell 2, it starts at **4**. Updated in
`Saved/SwarmExecOnPlay.txt`, `Scripts/populate_cvar_preset.py`, and the CVar declarations:
`Emberkeep.UnitCamProj.SpriteCell` 2 → **4** (retinue walk0), and
`Emberkeep.UnitCamProj.HeroCell` 0 → **6** (retinue attack; 0 is now a *brood* frame).
