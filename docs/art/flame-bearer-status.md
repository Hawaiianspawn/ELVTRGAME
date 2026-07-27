# The flame bearer — status checkpoint, not a new round

**Purpose of this file:** the owner asked (via the team lead) to "look into making the main
character that holds the light." That work already happened —
`docs/art/protagonist-prototypes.md` (four generated candidate directions) and
`docs/art/vanguard.md` (a full incumbent spec built on the winner of that round's own
argument) are both committed, both current, and neither is stale. This file does not
repeat them. It (1) states what already exists so nobody re-scopes from zero, (2) records a
QC pass over the four already-generated renders that nobody had checked against their own
buildability rules, (3) checks the in-engine state against what those docs assume, and
(4) consolidates the open questions that are genuinely still blocking a decision, pulling
together canon proposals that are currently scattered across three files.

**Binds to:** `docs/art/protagonist-prototypes.md`, `docs/art/vanguard.md`,
`docs/narrative/FLAME-FOUNDATION.md`, `docs/art/aesthetic-direction.md`, `CLASSES.md`,
`docs/data/art/palette.json`, `docs/RENDERING-LIGHTING.md` §4a/§4b.

---

## 1. What already exists — read this before scoping anything new

- **`docs/art/protagonist-prototypes.md`** ran exactly the scoping question the brief
  asked for: "how does the only light in the world attach to a person." Four candidate
  relationships between body and fire were designed, each with a palette table, a
  disjointness audit against every other spec in the game, a horde-scale readability
  call, and a rig verdict: **01 The Standard** (flag on a pole, incumbent), **02 The
  Ember Core** (fire caged in the chest, empty hands), **03 The Tender** (fire in a bowl
  held in both hands), **04 The Brazier** (fire in a back-mounted basket across the
  shoulders).
- All four were **actually generated**, at both 48px and a 128px concept pass, and
  **quantized** to the locked Demichrome ramp. Files exist at
  `RawArt/Renders/protagonist-0{1..4}/r1/quantized/{anchor,concept128}/…/south.png`
  (plus all 8 rotations, retained per the retention rule). Nothing from this batch has
  been imported into `/Game/` — `protagonist-prototypes.md` §10 says so explicitly and
  it is still true.
- **`docs/art/vanguard.md`** is a complete, buildable 48×48 / 4×4-sheet spec for
  prototype 01 worn by the Vanguard class specifically: pixel map, sheet layout
  (`SubImageIndex = direction × 2 + walkBit`), animation notes, and a light/dim-shift
  spec (full ramp always — the hero is the calibration reference other units dim
  against). It is the most finished of the five hero/class art docs in the repo.
- **No decision has been recorded anywhere.** I grepped every doc under `docs/` for the
  four prototype names and for "winner" — the only hits are the round's own file and its
  four JSON requests. Nobody has told the pipeline (or any doc) which of the four the
  owner picked, or whether the owner has looked at the 128px renders at all.

**Consequence: there is very little left to scope from scratch.** The open work is a
decision plus a fidelity pass, not a fresh design round.

## 2. QC pass on the four existing renders — new finding

I looked at the four quantized `concept128` south frames against each prototype's own
buildability language in `protagonist-prototypes.md` §6–§9. None of the four is a clean
match to its own spec as generated. This matters because it changes what "pick a winner"
actually commits to right now — the pixels in hand are not yet the pixels the prose describes
for three of the four.

| Proto | Spec calls for | What the quantized render actually shows |
|---|---|---|
| **01 Standard** | no cape/cloak (explicit "must be absent," `vanguard.md` §3.7); a Pale flag detached above the crown | The render carries a full dark cloak/cape down the back — a silhouette-breaking element the spec forbids outright. The pole is present but no clearly detached Pale rectangle reads at the top; it's ambiguous rather than a hard rejection. |
| **02 Ember Core** | `steel`-dominant body, Pale ≤3% caged in a small chest slot | Body reads closer to **Dark**-dominant than Steel. The chest device reads as a grey plate/screen rather than a lit ember — it does not clearly carry Pale at all in the still frame. This is the prototype the spec already predicted would lose on horde-scale findability (§7); the render doesn't even land the close-up version of its own idea. |
| **03 Tender** | hooded closed loop, bowl of fire in both hands, `bone`-dominant | **Best match of the four.** Hood, closing arm loop, and a visible lit bowl all read as specced. If a decision has to be made from these renders as-is rather than from the prose, this is the only one that doesn't need a re-roll first. |
| **04 Brazier** | fire in a **back-mounted ribbed basket riding across the shoulders**, `mixed` dominance | The render shows a **front-carried black box/crate held at chest-to-waist height**, not a shoulder-mounted bar — a different silhouette than the one the spec argues for and audits against 01 and 02 in §5. The scattered light flecks read as jewels on a box, not fire between ribs. |

**What this means, plainly:** picking among these four *images* today would not be
picking among the four *designs* the prose describes — 01, 02, and 04 have drifted from
their own spec in the generation. `protagonist-prototypes.md` §11 already names the
failure mode ("any prototype may come back as a generic [something]... re-roll once
before drawing conclusions") and its own re-roll triggers (wrong dominant value, missing
defining prop) are tripped by 01, 02, and 04. This is not a new design problem — the
prose in `protagonist-prototypes.md` is sound and doesn't need rework — it's a
generation-fidelity gap between the spec and the art in hand.

## 3. In-engine reality check

The team lead's brief framed the hero as now visible at battlefield scale via the fixed
Niagara sprite path. Checked against the code: **that's true for retinue and brood, not
for the hero.** `ELVTR/Source/ELVTR/Spike/SpikeHeroPawn.cpp:115-123` sets the hero's
entire visual representation to the engine's built-in `/Engine/BasicShapes/Cube.Cube`
static mesh, scaled `(0.6, 0.6, 1.2)`. The hero pawn is not a Niagara particle, is not
on `T_Swarm_2bit`, and has no sprite of any kind today — it is a literal grey box. None
of the four protagonist candidates, nor `vanguard.md`'s finished sheet, are wired to
anything the player can currently see. So "the hero is seen at both battlefield and
Unit Cam scale" is the target state these docs describe, not the current one.

## 4. The open questions — consolidated

Three separate docs (`vanguard.md` canon proposal 2, `protagonist-prototypes.md` canon
proposal 1, and this file) all converge on the same fork. Pulling them into one place:

1. **Is there one bearer silhouette, or four (one per class)?** `CLASSES.md` gives all
   four classes — Vanguard, Relickeeper, Pathfinder, Lampbearer — their own "hero"
   identity, and `vanguard.md` canon proposal 2 (unratified) proposes that each class
   burns its flame in its own registered shape-carrier: Vanguard = banner rectangle,
   Relickeeper = rune dot-cluster, Pathfinder = mark contour, Lampbearer = point+halo
   (the only one that actually *glows*). Under that reading, `protagonist-prototypes.md`
   already answered the Vanguard's case (01 = the Standard = `vanguard.md`), and
   prototypes 02/03/04 are candidates for the *other three classes'* bearer sprites, not
   competitors to 01 for one universal role. But `protagonist-prototypes.md` §13
   proposal 1 says 02/03/04 "quietly assume" a class-*independent* protagonist instead —
   the round was written without deciding which of these it was doing. **This is the
   single highest-leverage decision left:** it determines whether the deliverable is one
   sheet or four, and it turns 02/03/04 from "also-rans in a contest 01 already won" into
   "the Relickeeper/Pathfinder/Lampbearer's own answers to the same question," each
   worth finishing on its own terms.
2. **Has the owner actually seen the four 128px concepts?** No doc records a decision.
   Worth confirming before spending more generation budget in either direction — and if
   the answer is "not yet," that review should happen before any re-roll, since the QC
   findings in §2 might not survive a decision that some of these directions are already
   out of contention.
3. **Is a 3D-rigged protagonist actually near-term, or a someday option?**
   `protagonist-prototypes.md` treats "may be rigged, may become a 3D model" as a live
   selection criterion (the **Rig** line in every prototype section) and flags that 02
   and 03 translate cleanly to 3D while 01's flag and 04's basket mostly don't. GDD open
   question #5 (flipbooks vs. flat-shaded 3D) is explicitly still unresolved
   project-wide. If 3D rigging is a real near-term plan for *this specific character*,
   that's a thumb on the scale toward 02/03 regardless of how §2's QC pass reads, and it
   should be said out loud rather than discovered after a flipbook sheet ships.
4. **What rendering path does the hero itself take?** `vanguard.md` assumes a standalone
   `T_Hero_Vanguard` sheet (4×4, its own SubUV index), separate from `T_Swarm_2bit`, with
   the hero staying a distinct always-full-ramp reference rather than joining the swarm's
   render buffer. That's a reasonable reading of "the hero is the calibration reference"
   (`vanguard.md` §7), but nothing confirms whether the intended next step is (a) a
   billboard/sprite component on `SpikeHeroPawn` reading that standalone sheet, or (b)
   folding the hero into the Niagara swarm as a fifth row (which would need a
   `SwarmSheet` change — out of scope for this file, since that header is currently
   owned by the camera work). Whoever builds this next needs that answered, not assumed.
5. **If 03 (Tender) stays live, it collides with an existing retinue unit.**
   `protagonist-prototypes.md` §13 proposal 3, not repeated in full here: soldier-05
   "Flame-tender" is the same fiction (hooded figure, caged fire in a vessel) at a
   smaller rank, and a congregation containing a visibly smaller copy of its own god
   reads as confusion rather than hierarchy. Needs an owner ruling before both ship,
   regardless of how question 1 resolves.

## 5. What I did not do

I did not pick a winner, generate anything, re-roll any anchor, or edit
`protagonist-prototypes.md` / `vanguard.md` / `CLASSES.md` / any file outside my own
lane. Nothing here changes canon. This is a status report and a question list, per the
brief.

## Depends on

**GDD #5 (flipbooks vs. flat-shaded 3D):** open question 3 above is exactly this
dependency landing on a specific character; no recommendation is made here beyond
restating that it should be answered on purpose for this character rather than by
default.

**GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated.

## Canon proposals

None new. This file consolidates canon proposals already standing in
`docs/art/vanguard.md` (proposal 2) and `docs/art/protagonist-prototypes.md`
(proposals 1 and 3) rather than adding to them — resolving question 1 above should
resolve all three at once.
