# NPC silhouette brief — three representative NPCs under strict global palette

**Depends on:** activates `aesthetic-direction.md`'s 2026-07-12 reset (Direction A locked,
GDD #6 resolved to strict global palette). Supersedes nothing on its own — reads
`hero-palettes.md`'s retired per-faction system for precedent only.

**Friendly-NPC style lock (owner directive, 2026-07-12):** every friendly NPC — Liberated
militia included — is anchored to the composition of
`Artboard/Gameplay Avatars/crops/` (the chopped EarthBound/Mother reference,
`59df83c0…jpg`): a **bust-forward icon**, oversized round head occupying nearly the whole
frame, minimal-to-no visible body below it, single bold black outline, flat fill, near
head-on framing (not the 3/4 low top-down walking-sprite view used for heroes and
Legion/Quiet enemies). This is a *composition/proportion* lock layered on top of the
palette-and-silhouette mechanism below — the (a) militia mechanism (ragged silhouette,
light-dominant values, no Pale at rest) still governs *how it stays disjoint from
enemies*; this note governs *how it's framed and posed*. Enemies (Legion, Quiet) are
**not** covered by this lock and keep the low top-down chibi body proportions already in
use unless told otherwise.

**The palette (all three, no exceptions):**

| Hex | Name | Role |
|---|---|---|
| `#211e20` | Demichrome Dark | darkest — void, recess, outline, negative space |
| `#555568` | Demichrome Steel | dark-mid — armor plate, cloth, hard material |
| `#a0a08b` | Demichrome Bone | pale-mid — skin, bandage, worn/soft material |
| `#e9efec` | Demichrome Pale | brightest — reserved: eyes, marks, lamps, glyphs only |

No hue exists to help. Every NPC below is disjoint from the other two on **silhouette
regularity + which values dominate + whether Pale appears and in what shape** — never on
color.

---

## (a) Liberated militia — Vanguard retinue, friendly rescued soldier

**Reads as:** a patched, individual person pressed into a rank — motley, not uniform.

**Mechanism:** silhouette is **irregular/ragged** — mismatched scavenged gear breaks the
outline (a bandaged arm, an improvised club instead of a spear, an open or missing
helmet). Value pattern skews **light**: Bone (skin/bandage) and Steel (patched armor)
roughly even, Dark used only for recess/outline, never as a body-filling mass. **Pale is
almost always absent** — a Freed/Militia unit at rest carries zero Pale pixels; only the
rare Bannerman spends a Pale rectangle-flip (2-frame flag), per the shape-carrier
registry surviving the reset (`hero-palettes.md` §3, now applied globally, not just to
class brights). At horde scale: a block of *visibly uneven* small silhouettes, individual
and human, holding a line.

## (b) Still Legion soldier — tragic enemy, intact cold uniform

**Reads as:** the person is gone; only the uniform, perfect and repeated, remains.

**Mechanism:** silhouette is **clean and symmetrical** — closed dome helm, squared shield
rectangle, identical from unit to unit (formation-slot precision, no individual variation
even where militia would show one). Value pattern skews **dark**: the body is a mostly
solid Dark mass with Steel only at helm rim/shield/blade edge; Bone appears only as a
single thin band (a collar, a sash) — never skin. **Pale never appears on the body at
rest** — same reservation as militia, but for a different reason: Legion has nothing left
to shine. This is the inverse read of (a) on both axes at once — regular vs. ragged, dark
vs. light-dominant — so the two stay disjoint even though they share every hex and a
near-identical chibi humanoid proportion.

## (c) Quiet creature — void-with-eyes, the dark itself

**Reads as:** not a person at all — an absence with something looking out of it.

**Mechanism:** breaks both silhouette rules above at once. No bilateral-symmetric humanoid
proportion, no held-object rectangle — an **amorphous, non-geometric mass** (tendril/blob
edges, no straight lines). **Zero internal value modeling**: the entire body is flat
Demichrome Dark, no Steel or Bone at all — this is the tell that separates it from both
human factions at a glance, since even the darkest Legion soldier still carries a Steel
rim and a Bone band. The only other value present is **Pale, as a bare eye-dot (or small
irregular cluster), with no halo dither** — halo-dither point+glow is reserved to the
Lampbearer/honest-light carrier shape (`hero-palettes.md` §3) and must never appear on a
Quiet unit, or it will misread as a friendly light source instead of a threat. At horde
scale: patches of pure black in the crowd with a few pale points staring back — unmistakable
against both the light-dominant militia blocks and the dark-but-Steel-rimmed Legion ranks.

---

## Three-way disjointness check (the actual audit)

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| Militia | ragged/irregular humanoid | Bone+Steel (light) | none at rest; Bannerman flag-flip only |
| Legion | clean/symmetrical humanoid | Dark (heavy), thin Steel rim | none, ever |
| Quiet | amorphous, non-geometric | Dark, flat, zero internal modeling | bare eye-dot(s), no halo |

Any two of these three axes disagreeing is enough to disambiguate at 1–2px; all three
disagreeing (as designed) makes it disambiguate at horde scale even in motion blur or
partial occlusion.

## Depends on

- **#6:** requires strict global palette (now resolved). Built for exactly this state —
  does not degrade gracefully back to per-faction; if #6 is ever reversed again, this
  doc's "why they don't collide" column stops being load-bearing and should be re-checked
  against whatever hue channel returns.
- **#5:** Neither — silhouette/value rules are renderer-agnostic; frame counts and sheet
  layout are out of scope for this brief (see brief-driven follow-up specs per NPC).

## Canon proposals

None. This brief operationalizes the 2026-07-12 owner reset; it does not propose new
canon beyond restating the existing faction-language table (WORLD.md §3) in
silhouette-only terms.
