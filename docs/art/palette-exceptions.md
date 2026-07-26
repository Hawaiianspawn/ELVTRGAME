# Palette exceptions ledger

**Purpose:** `aesthetic-direction.md`'s 2026-07-12 reset locked one global 4-value ramp
and said any 5th value needs "an explicit owner exception." That's been invoked once
(the flame's focusing core) and extrapolated once without a citation (hit-flash white).
This is the running ledger so the next "is this a real exception" question gets checked
against precedent instead of re-litigated or, worse, quietly assumed. Add to it, don't
let it go stale.

## Ruling: `FlameCoreColor` — SANCTIONED

Pure white, drawn as the flame's focusing core (`docs/RENDERING-LIGHTING.md` §4b.8).
**Dated owner call, 2026-07-23.** Rendered as a genuine bypass of the demichrome
quantization LUT inside `M_PP_Demichrome` (a real material special-case, not scene
color fed through the normal pass) — it exists specifically to read as *brighter than
the brightest ramp value*, which is the whole point: Demichrome Pale (`#e9efec`) is
already ~92% white, so a focusing point that has to visually outshine the entire lit
pool needs headroom the ramp doesn't have. Scoped to the flame itself; not license for
a fifth value anywhere else. Precedent for future exceptions: **only sanction a 5th
value when the design requirement is specifically "brighter than Pale," not merely
"bright."**

## Ruling: `HitFlashColor` on the world renderer — NOT A VIOLATION, no exception needed

`SwarmRenderActor.cpp:388`, pure white, drawn via `DrawDebugSolidBox` in
`TickDebugRender`. The comment there calls it "the same class of deliberate palette
exception as the flame's white core" — that's the extrapolation task #9 flagged, and
having traced it, **it's inaccurate, not just uncited.** Unlike the flame core, this
color has no special-case material path. It's ordinary scene geometry, fed through the
*same* `M_PP_Demichrome` quantization every other debug box goes through
(`RetinueBaseAlbedo`, `BroodBaseAlbedo`, etc.) — `lum = saturate(lum + atten*Intensity)`
clamps to `[0,1]` before the threshold/LUT step, so the maximum any object in that pass
can produce is index 3, which the LUT maps to `#e9efec` (Pale) — not literal
`(255,255,255)`. Pure white here is a **reliable-input trick**, not a rendered
exception: it guarantees the flash saturates to Pale regardless of how far
`Swarm.UnitLightFloor` has darkened that unit, which is exactly the problem the
comment describes solving. Functionally correct, just mis-self-described.

**No owner exception needed.** Recommend fixing the comment (drop the
"deliberate palette exception" framing, replace with "guarantees saturating to Pale
regardless of floor-attenuated distance") so it stops reading as a precedent the next
system might copy uncritically. Moot shortly regardless — `TickDebugRender` is the
renderer task #7 resolved to retire in favor of the Niagara sprite path.

## Ruling: `HitFlashColor` on the Unit Cam panel — REJECTED, replace with Pale

`UnitCamProjector.cpp:857`: `B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha)`, citing
the world renderer's comment ("light-exempt for the same reasons"). **This one actually
is a violation, and the world-renderer comment's reasoning doesn't transfer to it.**
The Unit Cam panel is UMG, drawn *after* post-processing by design
(`docs/RENDERING-LIGHTING.md` §4d — "the panel is UMG... it does **not** get
demichrome-flattened the way the old capture did"). Nothing downstream quantizes it.
`FLinearColor(1,1,1,...)` renders as literal, unfiltered pure white on screen — a real
fifth value, in a panel that's live today, independent of the debug-box-vs-sprite-path
question that retires the other case.

**Reject the exception.** The flame core needed genuine white because the design
requirement was specifically "brighter than Pale, so it reads as a focal point that
outshines the lit pool." Hit-flash has no such requirement — it only needs to read as
"the brightest thing this unit can be," and Pale already *is* the ramp's designated
register for exactly that (`palette.json`: "brightest — lamps, eyes, marks, UI").
There's nothing for hit-flash to outshine; borrowing the flame-core precedent asks for
capability this use case doesn't need.

**Fix:** replace the literal white with Demichrome Pale —
`FLinearColor::FromSRGBColor(FColor(0xE9, 0xEF, 0xEC))` (or a shared constant if one
gets added; several files hand-roll `#e9efec` independently and a shared
`FColor DemichromePale` somewhere central would be worth its own small cleanup). At
~92% luminance versus 100%, the perceptual difference in a flash that lasts
`Swarm.HitFlashTime` (0.10s default) is not detectable in play — nothing is lost, and
the panel stops being the one place in a shipping UI where the locked ramp has a
silent hole. Not mine to land (I don't edit `.cpp`) — flagging the exact replacement
so whoever owns `UnitCamProjector.cpp` can drop it in directly.

## Standing rule for future additions to this ledger

Before sanctioning a new 5th value anywhere: ask whether the design requirement is
"needs to exceed Pale" (the flame core's actual test) or just "needs to be bright"
(which Pale already covers). Only the former is a real case for an exception; the
latter should always resolve to Pale and get logged here as a non-violation or a
rejection, not routed to the owner as a fresh ask.
