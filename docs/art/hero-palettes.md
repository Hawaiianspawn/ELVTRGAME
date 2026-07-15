# Hero palettes — the Gatecamp Family (shared 8-color game budget, one shared class bright)

> **RESET 2026-07-12 — this doc's entire premise is VOID.** Owner directive,
> verbatim: *"I want to use the 2bit Demichrome as the entire color palette, no
> other variations unless I explicitly overwrite it."* See
> `aesthetic-direction.md`'s top banner for the full reset. There is no more
> Gatecamp Family, no separate Still Legion family, no exempt Unwitnessed
> register — **every sprite in the game, friend and foe alike, draws from the same
> single global 4-value palette:** `#211e20` Demichrome Dark / `#555568` Demichrome
> Steel / `#a0a08b` Demichrome Bone / `#e9efec` Demichrome Pale (these are the
> exact four hexes this doc previously called the "Still Legion family" below —
> that name is retired along with everything else here).
>
> **Explicitly retired by this reset:**
> - The **Honey Milk**-lineage Gatecamp trio (Vault Dark `#211210` / Patched Steel
>   `#5e2d20` / Kitchen Tin `#c76b2a`) and **Gatecamp Bright `#f0c260`** — gone,
>   along with every "shared warm bright" argument built on them (§2, §3 below).
> - The disjoint **Still Legion family** as a *separate* palette from the friendly
>   side — gone; Legion sprites and hero sprites now draw from the identical four
>   hexes, no cold-vs-warm split at all.
> - **The Oil 6 Unwitnessed exemption** — gone. **Said loudly, not buried: this
>   removes the Unwitnessed's richer horror register.** The whole "tragic Legion
>   vs. full-horror-only-Unwitnessed" separation (§2.2 of `aesthetic-direction.md`,
>   the horror-ceiling decision) rode partly on the Unwitnessed having a palette
>   the rest of the game didn't get — six colors instead of four, a warmer/stranger
>   ramp reserved for them alone. That palette-side distinction is now gone; the
>   Unwitnessed render in the exact same four values as everyone else and must
>   carry their horror entirely through scale, silhouette, and negative-space
>   shape (the void-with-eyes language, §1b) instead. **This is a real loss the
>   owner may want overridden back in later** — flagging explicitly so it isn't
>   silently reintroduced or silently mourned.
>
> **What survives, unchanged:** the shape-only mechanism in §3 below (rectangle /
> dot-cluster / contour / point+halo) — it was always shape-first with hue only as
> a redundant backstop, and that backstop is now gone for the *entire game*, not
> just the hero brights. Read §3/§4 below as historical precedent for how
> shape-only differentiation was built for one bright pixel; the new NPC
> silhouette brief (companion doc, see aesthetic-direction.md) applies the same
> discipline to full sprites, all four values, zero hue.
>
> Everything below this line describes the now-retired per-faction system. Kept
> as history; do not cite its hexes, "Gatecamp Bright," or the Gatecamp/Legion
> family split in any new spec.

**Brief:** `../briefs/brief-004-hero-palettes-and-portraits.md` · **Owner decisions (locked
2026-07-11, morning):** shared anchor + shared cold-mid + shared warm-mid across all four hero
classes; one reserved bright per class, tied to its mechanic; heroes are fixed named
individuals with visible faces. **SUPERSEDED SAME DAY (2026-07-11, afternoon) — see §0.**
**Companions:** `warden-captain-bree.md` (precedent palette, now stale — flagged in §5 and in
`aesthetic-direction.md`'s stale-file list), `portrait-register.md`, `aesthetic-direction.md`
§3 Direction B / §4 decision log · **Hero specs:** `hallam.md` · `edda.md` · `merle.md` ·
`noll.md` — **also stale, not yet redrawn under this palette; see §5.**

This is the palette *system* doc. The four hero specs cite it and add nothing to it — any
future friendly-family palette (retinues, named NPCs, Gatecamp civilians) binds here.

---

## 0. Decision log — 2026-07-11, "Accept shape-only" (supersedes the four-bright system)

Owner ruling, verbatim: **"Accept shape-only."** This is a binding revision of the palette
proposal given the same day, adopted in full — including its hardest clause: **all four hero
classes now share one literal bright hex.** Roll-Gold, Waking Ember, Waylight, and Watch-Lamp
(four separate class-bright hexes) are retired and collapsed into a single value, **Gatecamp
Bright `#f0c260`**. Class identity no longer rides on hue at all — it rides **entirely** on the
shape of the thing carrying the bright (§3). This doc supersedes every hex value and every
"mutual distinguishability by hue" claim in the previous version of itself.

The revision also replaces the old three-value Gatecamp trio with a new **8-color total game
budget**, split into two disjoint four-value families sourced from named Lospec palettes per
the owner's references:

- **Gatecamp family** (friendly, warm) — `Honey Milk`-lineage: Vault Dark, Patched Steel,
  Kitchen Tin, Gatecamp Bright (§2).
- **Still Legion family** (enemy, cold) — `2-bit Demichrome`: Legion Dark, Legion Steel,
  Legion Bone, Legion Pale (§2).
- **Unwitnessed register** (exempt) — `Oil 6`: outside the combat budget entirely, non-chibi,
  vignette/set-piece only (§6).

**Hard constraint is still satisfied per-sprite:** any single sprite/palette still draws from
exactly one four-value family — never both. "8 colors" is the *game's* total combat-register
budget across two disjoint families, not a single 8-value palette on one sprite.

**What this costs, said loudly (per owner instruction not to bury losses):** the old system's
insurance policies — four distinct hues so a bright pixel told you *which class* before you
even parsed its shape, and a deliberately-cold Patched Steel that kept friendlies literally
inside the Legion's value family for the mirror-fight read — are both gone. §3 and §4 document
the replacements and are explicit about what is weaker now, not just what is different.

---

## 1. Intent

Fiction: the Gatecamp is one people — patched steel, kitchen tin, and whatever light each of
them managed to keep. A mixed party must read as *one camp walking*, not four liveries; now
that they share one literal light-color, this is truer than ever — it is one lamp's worth of
light, carried four different ways.
Gameplay: in 4-player co-op the class parse is **shape-first, full stop** (lines / mass /
motion / glow, CLASSES.md cross-class rule). The palette no longer has a hue-confirmation
layer to fall back on — shape carries **all** of the signal for which class you're looking at,
color only confirms "this pixel is spent light," never "this pixel is Hallam's light."

---

## 2. The Gatecamp family — canonical names and values

Every friendly-family sprite palette is these four values (3 body values + the one shared
bright). The family is still named **the Gatecamp Family**; names/hexes are canonical, do not
fork them.

| Hex | Name | Role |
|---|---|---|
| `#211210` | **Vault Dark** | shared dark anchor — outline/recess/underside, shared with the world's ground state |
| `#5e2d20` | **Patched Steel** | warm dark-mid — armor/recess/shadow. **No longer cold** (was `#4e5a66`, deliberately cold in the previous system). This value used to be the mechanism that kept friendlies "inside" the Legion silhouette family for the mirror-fight read (`warden-captain-bree.md` §1/§2) — that mechanism is gone; see §4 |
| `#c76b2a` | **Kitchen Tin** | warm-mid — skin, straps, cloth, tin. **Still the friend/foe value-role channel** — the Still Legion fills this same role-slot with cold Legion Steel `#555568`; warm-vs-cold in the mid-value slot is unaffected by this revision (see §4, it was never Patched Steel's job) |
| `#f0c260` | **Gatecamp Bright** | the ONE shared class bright — Vanguard, Relickeeper, Pathfinder, and Lampbearer all render this exact hex. Differentiated **only** by carrier shape (§3). No value or *mask* concept exempt here — this is a real palette entry, always counted |

### The Still Legion family (for contrast, not this doc's palette to spend)

| Hex | Name | Role |
|---|---|---|
| `#211e20` | Legion Dark | Legion anchor |
| `#555568` | Legion Steel | Legion cold-mid — fills Kitchen Tin's role-slot, cold |
| `#a0a08b` | Legion Bone | Legion warm-cold-mid / hollow-helm read |
| `#e9efec` | Legion Pale | Legion bright, doubles as the Quiet's eye-value |

- **Bree's palette is retroactively the founding member** of the Gatecamp family, but her
  spec's *hexes* are now stale under this revision (she predates it, like the four hero specs
  — see §5). Her bright is the shared Gatecamp Bright now, same as everyone else; she and Noll
  disambiguate by shape and motion alone, per her spec's own argument, which still holds.
- **Retinues use their class's palette unmodified.** Bright scarcity bites hardest there: most
  retinue units carry **zero** bright pixels (a Liberated militiaman is trio-only; only the
  Bannerman carries gold, only the wisp carries flame, etc.) — this is unchanged.

### Light-shifted variant (inside lamp radius / honest-light fixtures)

Derived via the same one-step-brighter methodology as the Bree precedent (each value nudged
toward white, hue held). **These are a working proposal, not separately owner-locked** — flag
for confirmation alongside the hero-spec redraw pass:

| Base | Shifted (proposed) |
|---|---|
| `#211210` | `#35211c` |
| `#5e2d20` | `#7c4630` |
| `#c76b2a` | `#e08c46` |

**Gatecamp Bright does not shift** — same reasoning as before, now stronger: it is the ONE
signature all four classes already share, so letting it drift would wash out the *only* signal
class identity has left. One exception carried over unchanged: **flame source pixels** (Noll's
lamp, honest-light fixtures) may render `#fff6dd` inside their own radius — the flame is the
only carrier that *is* the light source; a flapping rectangle, a glyph-dot cluster, and a
contour reflect remembered light, they do not emit rooms.

---

## 3. The one bright, four carriers — shape is the entire signature now

Every class palette = Gatecamp trio + `#f0c260`. Because hue no longer differentiates
*anything*, **shape-disjointness is not a nice-to-have, it is the only thing standing between
"this pixel is spent light" and "I know whose light this is."** Read this table as load-bearing
in a way the old hue table wasn't.

| Class · Hero | Carrier | Shape signature | Scarcity rule |
|---|---|---|---|
| Vanguard · Hallam | Banner cloth field; Bannerman mini-flags | **Flapping rectangle** — 2-value flip animation, Gatecamp Bright ↔ Vault Dark, attached to friendly geometry | Cloth and thread only. Never armor, weapon, or skin; never on a non-Bannerman unit. At rest, ≤2px on Hallam's furled banner knot |
| Relickeeper · Edda | Rune glyphs on terrain/enemies; Graver tip while inscribing; gilded seams on over-mended Awakened | **Static glyph-dot cluster** — discrete dots forming marks, never a contour, never a fill | Glyph and seam pixels only. Never an area fill, never a point-with-halo, never at rest on Edda herself |
| Pathfinder · Merle | The quarry-mark: 1px outline hugging a marked enemy's silhouette; snare-line anchor pips | **Thin moving contour/ring** — rides an enemy silhouette, never static, never filled | Contour and pip only — never a fill, never a halo'd point, never on any friendly sprite. Merle at rest carries none (§4) |
| Lampbearer · Noll | Lamp flame pixel + 1px halo dither; Guided wisp points; honest-light fixtures | **Point with halo** — still = safety, drifting = wisps | Flame, halo, and wisp points only. Never armor, cloth, skin, or eye catchlights |

**Mutual distinguishability at 500 units — shape ALONE, no hue backstop:**
rectangle (area, flip-animated) / dot-cluster (static, discrete) / contour (moving, hugs an
enemy) / point+halo (still or drifting, always circular-ish). Four disjoint shape classes,
zero shared silhouette grammar between any two. This has to hold at 1–2px readability, which
is a harder bar than the old system asked of it — the old system had hue as a second, redundant
check; this one does not.

**Mark protection (audit rule for every future palette — REWRITTEN, shape-only):**
there is no hue-collision check anymore; every bright pixel in the game is the same hex. The
audit is now:

1. Exactly one shape class may render `#f0c260` per carrier type: rectangle-flip → Vanguard
   banners only; dot-cluster → rune marks only; thin-contour → quarry marks only;
   point+halo → lamps/honest-light only.
2. No single sprite may combine two shape classes of bright in the same silhouette (a banner
   may not also carry a point+halo; a marked enemy may not carry both a rune-cluster and a
   quarry-contour without both reading as legible, separate marks, not one ambiguous blob).
3. Any new class, retinue unit, or item that wants to spend the bright must be assigned exactly
   one of these four shapes, or must propose and justify a fifth shape class that is
   silhouette-disjoint from all existing four (new canon proposal territory, not a quiet reuse).

**Unwitnessed fence, restated:** the Unwitnessed sit outside this 8-color budget entirely (§6)
and do not use `#f0c260` at all — no fence needed against hue-counterfeiting since they don't
share a palette family with the Gatecamp in the first place.

---

## 4. The Merle ruling and the mirror-fight read — what shape-only costs

Two mechanisms from the previous system leaned on hue. Both are re-derived here; both are
weaker than what they replace, and that is stated on purpose, not discovered by accident later.

### 4a. The quarry-mark's old hue insurance is gone

The previous ruling (kept for the record) argued the quarry-mark should be **warm** — grammar
over color: safety is warm glow (point + halo), a claim is a warm contour, friendlies are never
outlined in any bright, so a warm outline on an enemy has no safety signal to counterfeit. That
grammar argument **still holds** and is why the contour carrier still reads correctly on an
enemy. What's gone is the *second* leg of the old ruling: the mark's hex (`#d9f0b8`, pale
green) was deliberately hue-separated from the flame's hex (`#ffe9c2`, warm cream) as a
belt-and-suspenders insurance policy — "even a colorblind-adjacent or 1px read cannot confuse
mark with lamp." Under shape-only, mark and flame are **the literal same hex**. The only thing
separating a quarry-mark from a lamp flame at 1px is now: contour (rides a silhouette, no
halo, always moving with the target) vs. point+halo (circular, halo dither, still or drifting
independent of any enemy). This is a real loss of redundancy — flagging it here rather than
letting a future artist rediscover it by shipping a confusable 1px asset.

### 4b. The mirror-fight read — mechanism replaced, not repaired

**Old mechanism (retired):** Patched Steel `#4e5a66` was deliberately cold, matching the Still
Legion's cold mid-values in kind, which kept friendly units literally inside the Legion's value
family — the load-bearing trick behind Bree "reading as a Legion shield with the crown scoured
off," never quite foreign, never quite them (`warden-captain-bree.md` §1/§2).

**New mechanism:** Patched Steel is now warm (`#5e2d20`), part of the same warm family as
Vault Dark and Kitchen Tin — there is **no shared hex** between the Gatecamp and Legion
families anymore, cold or otherwise. The mirror-fight read now rides on two weaker, non-hex
signals instead:

1. **Near-anchor-parity** — Vault Dark `#211210` and Legion Dark `#211e20` sit close enough in
   value (not hue) that both sides still read as "one dark world" rather than a lit-vs-unlit
   split, at horde scale.
2. **Matching chibi proportion** (2026-07-11 chibi-combat ruling) — both sides render at
   identical chibi proportions, so silhouette-family match does the work the shared cold hex
   used to do.

**This is a conscious downgrade, not an oversight.** Value-parity plus proportion-parity is
softer than a literal shared hex: it holds at horde scale and at distance (where the mirror
sequence needs it most) but is more vulnerable at close, static zoom, where the old system made
confusion structurally impossible and the new one merely makes it unlikely to matter in play.
`warden-captain-bree.md`'s eventual redraw (see §5) needs to rewrite her §1/§2 argument around
this replacement mechanism, not just swap in new hex values.

**What did NOT change:** the friend/foe *value-role* channel — warm mid = friendly, cold mid =
Legion — was never Patched Steel's job in the first place (re-reading the precedent: it was
always Kitchen Tin `#c76b2a` vs. Legion Steel `#555568` in the same role-slot). That channel is
fully intact under this revision.

### 4c. The one-flame rule is now trivially true

Bree's lamp and Noll's lamp being "the same value on purpose" used to be a specific canon
proposal requiring its own justification. Under shape-only, **every** bright is the same value
by construction — the diegetic "every honest flame descends from the Foundling Lamp" reading
is still true and still nice to have in WORLD.md (§6.3 below), but it is no longer doing
mechanical work; the point+halo *shape* is what's actually reserved to lamps now, not the hex.

---

## 5. Stale-file flag — RESOLVED 2026-07-12

`warden-captain-bree.md`, `brees-stairwell.md`, `hallam.md`, `edda.md`, `merle.md`, `noll.md`
have all been redrawn in place against this revision (both the chibi-proportion ruling and the
shape-only palette revision, in one pass). They now cite Gatecamp Bright `#f0c260` and the
current trio hexes throughout; no file in the game still cites Roll-Gold/Waking Ember/Waylight/
Watch-Lamp as distinct hexes. `noll.md` also had a stale he/him pronoun draft reconciled to the
current she/her canon (`docs/narrative/noll.md`, CLASSES.md §4) as part of the same pass.
Naming: the four hero files stayed role-only in prose (no proper names), consistent with
CLASSES.md's 2026-07-11 reversal; the two Bree files kept her name, since she is a named NPC
(WORLD.md §5), not a playable class, and is unaffected by that reversal. All six kept their
legacy filenames for cross-reference stability.

---

## 6. The Unwitnessed exemption

The Unwitnessed are **exempt from the 8-color combat budget entirely** — they are large and
non-chibi per the 2026-07-11 chibi-combat ruling, and live in the vignette/set-piece register,
not the gameplay-sprite register this doc governs. Their palette is **Oil 6**:
`#fbf5ef` `#f2d3ab` `#c69fa5` `#8b6d9c` `#494d7e` `#272744`. No fence against the Gatecamp
Bright is needed here (§3) since the two registers never share a screen-space palette pull.

---

## Depends on

- **#6 (global vs per-faction palettes): this doc assumed per-faction; RESOLVED
  2026-07-12 the other way — strict global palette.** As predicted in the line
  below (written before the reversal happened), the entire 8-color/two-family
  structure has collapsed exactly as warned. This doc is now historical; see the
  reset banner at the top and `aesthetic-direction.md` for the current single
  global palette.
- **#5 (flipbooks vs flat-shaded 3D):** palette rules are renderer-agnostic — Neither. The
  rectangle-flip carrier (Vanguard banners) assumes a 2-value flip animation, which is cheapest
  on flipbooks; the four hero *sheet* specs assume flipbooks, see each (currently stale, §5).

---

## Canon proposals

1. **The Gatecamp Family, revised (CLASSES.md cross-class notes) — supersedes the previous
   canon proposal 2:** *all friendly-family palettes = Vault Dark `#211210` + Patched Steel
   `#5e2d20` + Kitchen Tin `#c76b2a` + one shared bright, Gatecamp Bright `#f0c260`; classes
   differ only in the carrier shape spending that bright, never in hue.* Makes the owner's
   2026-07-11 "accept shape-only" ruling citable canon.
2. **Shape-carrier registry (CLASSES.md, new — mechanical, not flavor):** *the four reserved
   bright-carrier shapes — flapping rectangle (Vanguard), static dot-cluster (Relickeeper),
   thin moving contour (Pathfinder), point+halo (Lampbearer) — are a closed, disjoint set;
   any future class or system that wants to spend the bright must add a fifth shape,
   silhouette-disjoint from all four, rather than reuse one.* This is now the game's only
   protection against bright-pixel ambiguity and should be enforced in review, not just docs.
3. **The one-flame rule (WORLD.md flavor, one line) — unchanged in substance, now flavor-only:**
   *every honest flame below descends from the Foundling Lamp.* No longer load-bearing
   mechanically (§4c) but still worth the sentence.
4. **Amendment to light-temperature law — likely moot, flagged for owner review, not
   re-proposed here:** the previous doc's canon proposal 1 (warm glow = honest light, cold =
   Crown/deep, bright contours are claims not lights) argued from hue. With only one hue in
   play on the friendly side, this proposal may no longer need a decision at all — leaving it
   unresolved rather than re-asserting it, since re-litigating it isn't this revision's job.
