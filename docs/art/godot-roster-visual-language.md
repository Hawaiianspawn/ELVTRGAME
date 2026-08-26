# Godot roster visual language — review

Reviewed from the PNGs directly: every strip named in the brief, `atlas.png` +
`manifest.json` for how they pack, `Scripts/art/atlas.py` for the 8-column direction
contract (`south, south-east, east, north-east, north, north-west, west, south-west` =
columns 0-7 left to right), `unit_sprite.gdshader` for render behaviour, and
`docs/qa/adversary_11_f1.png` / `docs/qa/adversary_7.png` for the actual hall.

## 1. The visual language as it exists today

Register is a moderate chibi (roughly 3 heads tall), rendered with soft two-tone
shading and anti-aliased edges rather than flat colour — this is full-colour PixelLab
output with baked shading, not the retired 2-bit rule, and it reads that way: pauldrons
and cloaks carry a visible highlight/mid/shadow gradient, and outlining is selective
(interior forms are shaded, not ringed in black). Side identity is carried mostly by
**value family**, weakly by hue: the player's four soldier types (`veteran`,
`halberdier`, `hammer`, `vet_ranged`) and most heroes share one cool blue-grey steel
family (pale blue-white highlight, mid slate armour, near-navy shadow); the necromancer
side (`ooze`, `undead`, `mace_undead`, `staff_undead`, `bow_undead`, `undead2`,
`armored`) sits in a duller, slightly darker grey-olive family with occasional cooler
green accents (`staff_undead`'s named "greeneyes", the ambient glow orbs in both QA
frames). Heroes break this rule on purpose — `hero_samurai` (oxblood), `hero_dwarf`
and `hero_sackhauler` (warm rust-brown), `hero_cover` (forest green), `hero_turret`
(teal-brown) — each hero is its own warm/saturated one-off rather than a member of the
steel family, which is the right instinct for "the one unit you're tracking" but is
applied inconsistently (`hero_knight` and `hero_ranger` did *not* get a one-off and sit
back inside the steel family, see §2-3). Silhouette carries direction reasonably well
in the south-facing quadrant (columns 0-2, 7) where weapons and shields project outward
from the body, but collapses toward a generic robed/armoured blob in the back-facing
quadrant (columns 3-5) across nearly every biped, ally and enemy alike — and the QA
screenshots show the player's own army walking away from the camera, so that weak
quadrant is the one seen most often in play.

## 2. Per-unit table

| Unit | Side | Silhouette at hall scale | Value/contrast vs. dark hall | What breaks the language | One-line fix |
|---|---|---|---|---|---|
| `veteran` | Ally | Fine south/side (cols 0-2, 6-7): shield bump + sword read clean. **Weak** cols 3-5 (north-east/north/north-west): shield and sword vanish behind a plain cloak blob. | Mid steel-grey, close in value to the hall's stone walls (both mid-grey/khaki in `adversary_11_f1.png`) but denser and moving, so it still separates in practice. | Col 4 (north, pure back view): no weapon, no shield edge, just a draped cape — nearly the same shape as `armored` col 4 (see §3). | Keep a sliver of the shield or a hilt visible past the cape edge in cols 3-5. |
| `halberdier` | Ally | Fine on every column — the polearm is a persistent vertical line even from directly behind (col 4 still shows the shaft over the shoulder). | Same steel family as `veteran`. | Nothing structural; closest thing to a model citizen in the set. | None needed. |
| `hammer` | Ally | Fine col 0/2 (hammer head projects past the body); weaker col 4 where the hammer head sits close to the same grey as the torso plate. | Same steel family; the hammer head has almost no value step against the armour, so at 24px it can merge into "big grey guy" rather than reading "hammer". | Col 4 (north): hammer head and shoulder pauldron are nearly the same grey value, no separating rim light. | One-shade-lighter highlight strip on the hammer head only. |
| `vet_ranged` | Ally | Fine — the drawn bow reads as a diagonal line in cols 0-2 and 6-7; back cols 3-5 drop to a small quiver hump, weaker but still a bump, not a blank blob. | Same steel family. | Minor: back-view quiver hump is small enough to lose at 24px. | Non-issue at review priority; lowest of the four soldiers. |
| `hero_knight` | Hero | Reads as "another veteran" — same pose grammar (shield+sword), same steel-grey family, no unique accent. | Same steel family as the soldiers around it. | Col 0 (south) is visually a slightly better-lit `veteran`; nothing marks it as the one tracked unit. | Give it one hero-only accent colour (crest, cloak trim) no soldier uses. |
| `hero_turret` | Hero | Strong — the only wide-based, non-bipedal silhouette in the roster; unmistakable at any column. | Dark teal/brown base against the hall reads as a distinct mass, good contrast. | None. | None needed. |
| `hero_sackhauler` | Hero | Strong — big rounded pack silhouette breaks the humanoid outline on every column. | Warm terracotta/orange, clearly outside both the ally steel and enemy olive families. | None. | None needed. |
| `hero_dwarf` | Hero | Strong — visibly shorter/wider than every other biped, reads by height alone before colour. | Warm brown, distinct. | None. | None needed. |
| `hero_cover` | Hero | **Fails.** Col 0 (south) is close to a solid green mass with almost no humanoid break — the ghillie read that's meant to say "stealth" instead reads as "not a legible unit". | Forest green sits next to the necromancer's green glow orbs (`docs/qa/adversary_11_f1.png`, the green dots scattered mid-hall) and the enemy family's cooler tint — worst hue proximity to the ambient FX of anything in the roster. | Every column — this is a base-hue problem, not a single-frame one. | Warm the green toward olive/brown or add one bright non-green accent (sash, buckle) so it can't be mistaken for lighting FX. |
| `hero_samurai` | Hero | Strong — oxblood/maroon armour plus a diagonal katana silhouette across the back is unlike anything else in the set. | Warmest, most saturated unit in the roster; pops hard against the grey hall. | None. | None needed. |
| `hero_ranger` | Hero | Same problem as `hero_knight`: hooded steel-blue cloak + bow reads as "another `vet_ranged`", no hero-only marker. | Steel-grey family, close to `bow_undead`'s hooded-cloak shape too (see §3). | Col 5-6 (north-west/west): hooded back silhouette is close to both `vet_ranged` (ally) and `bow_undead` (enemy). | Same fix as `hero_knight` — a hero-only accent, plus consider warming the cloak off pure steel-blue. |
| `ooze` | Enemy | Fine col 0-2 (spear + hunched hooded posture reads as enemy-coded); weaker back cols. | Duller, greener grey than the ally steel family — correct direction for side-coding. | Minor back-view blob, same class of problem as every other biped. | Low priority. |
| `undead` | Enemy | **Weakest in the roster.** Bare-stance, no weapon, no shield on any of the 8 columns — nothing to key a silhouette off besides a slightly hunched posture. | Grey value sits closer to the ally steel family than any other enemy — the one enemy least separated from allies by colour. | All 8 columns equally: an unarmed grey humanoid blob is the single hardest shape in the set to tell apart from a stray `veteran`. | Either lean into "weakest, plainest enemy" on purpose (darken it further so it reads as fodder-tier) or give it a visible claw/tatter silhouette detail. |
| `mace_undead` | Enemy | Col 0 (south): shield left + weapon raised right is the **same pose grammar as `veteran` col 0** (shield left + sword raised right) — see §3, this is the closest-confusable pair. | Grey value close enough to `veteran` that the pose is doing more identity work than the colour is. | Col 0 specifically — open both `veteran.png` and `mace_undead.png` side by side at column 1 (south) and the read is "two versions of the same guy". | Shift `mace_undead`'s shield/armour value toward the duller enemy-olive family already used by `ooze`/`undead` so the shared pose no longer shares the palette too. |
| `staff_undead` | Enemy | Reads fine as "robed staff-user" at every column; the named "greeneyes" identity marker is not visibly perceptible at review scale. | Mid grey-olive, correct family. | Source name promises a green-eye glow (`ranged-undead/z5_bracedstaff_undead_greeneyes`) that isn't legible in the strip even at full 88px cell — it will not survive downscale to a 24px on-screen unit. | Enlarge/brighten the eye glow to a hard 2-3px dot if it's meant to be a readable tell. |
| `bow_undead` | Enemy | Fine silhouette shape (hood point + bow) at every column. | **Darkest unit in the whole roster** — near-black cloak. Good contrast against the mid-grey hall walls up close, but this is a ranged threat (200 range per `units.json`) meant to be seen *before* it's dangerous, and the brief's world uses black depth fog at distance: the darkest silhouette in the set is the one most likely to disappear into that fog first. | Not a single-frame break — a values choice that works against this unit's own gameplay role (it should telegraph earlier than melee enemies, not later). | Lift the cloak or rim edge one value so it survives the fog falloff before effective range. |
| `undead2` (Bone archer) | Enemy | Col 0: crossbow held forward reads clearly as "archer" — same pose grammar and similar mid-grey value as `vet_ranged` (ally archer), see §3. | Lighter grey than the rest of the enemy family — closer to ally steel than `ooze`/`undead`/`armored` are. | Col 0 (south) is the confusable one — a grey archer holding a forward ranged weapon reads the same regardless of side here. | Shift toward the duller enemy-olive family, same fix direction as `mace_undead`. |
| `horse_undead` | Enemy | Strongest silhouette in the set by construction — the only quadruped, unmistakable at any column. | Blue-grey body with cream/white rib highlights that are noticeably brighter than anything else in the roster, allies included. | Not a failure, but a budget note: this fodder-tier flanker (dmg 12, not a boss per `units.json`) currently reads as the single brightest, most eye-catching thing on screen after the necromancer's green glow. | Dial the rib highlight down one step so brightness is reserved for actual elites/bosses. |
| `armored` | Enemy | Fine col 0 (hood + shield + mace reads as enemy). **Weak** cols 3-5: collapses to the same big draped-cloak blob as `veteran`'s back view — see §3. | Grey-green cloak, close in value (not quite hue) to `veteran`'s tan-grey cape. | Cols 3-5 specifically, directly comparable to `veteran` cols 3-5. | Push the cloak further toward grey-green and away from `veteran`'s warmer tan-grey so the two blobs separate even from directly behind. |

## 3. Friend/foe readability

Side-coding mostly works in the **south-facing quadrant** (columns 0-2, 7), where
allies keep the cool steel-blue family and enemies sit in a duller grey-olive family —
`ooze`, `bow_undead`, and `armored` in particular read as "not one of ours" on sight.
It breaks down in two places:

- **Closest-confusable pair: `mace_undead` (enemy) vs. `veteran` (ally), column 0
  (south) on both strips.** Open `mace_undead.png` and `veteran.png` side by side at
  the first panel: both are a shield held to the body's left and a single overhead
  weapon on the right, both in a mid steel-grey that hasn't been pushed toward either
  side's family strongly enough to carry the read alone. Since `mace_undead` counters
  nothing and dies easily while `veteran` is the army's frontline recommended against
  `archer_undead`/`undead`, mixing them up in a fast-moving hall is a real read failure,
  not just an aesthetic one.
- **Secondary pair, back view only: `veteran` vs. `armored`, columns 3-5 (north-east/
  north/north-west) on both strips.** Both collapse to a big draped-cloak blob with no
  weapon or shield visible, close in value. This matters more than a typical back-view
  weakness because both QA frames (`adversary_11_f1.png`, `adversary_7.png`) show the
  player's own army walking away from the camera — the back-facing quadrant is the
  *default* view of the allied line, not an edge case.
- Worth flagging even though it's a hero, not a soldier: `hero_ranger` sits close
  enough to both `vet_ranged` (ally) and `bow_undead` (enemy) in hooded-cloak silhouette
  and steel-blue value that a player scanning for "where's my hero" among a ranged
  skirmish has three similar silhouettes to sort through.

## 4. Ranked fix list

1. **`mace_undead.png` vs. `veteran.png`, column 0.** Recolour `mace_undead`'s shield
   and armour plate toward the duller enemy-olive family already established by
   `ooze`/`undead`, off the shared steel-grey. Aseprite recolour on the existing PNG.
   No pixellab credits.
2. **`veteran.png` vs. `armored.png`, columns 3-5.** Push `veteran`'s cape warmer/tan
   and `armored`'s cloak further grey-green so the two back-view blobs split in hue as
   well as value. Aseprite recolour on both existing PNGs. No credits.
3. **`bow_undead.png`, all columns — fog-swallow risk.** Lift the cloak/hood rim one
   value so this 200-range threat stays visible through the black depth fog before it's
   close enough to hit the player. Aseprite value edit on the existing PNG. No credits.
4. **`hero_cover.png`, all columns — green camouflage against ambient FX.** Warm the
   ghillie green toward olive/brown, or add one small bright non-green accent. Likely
   needs a real repaint of the base green rather than a simple levels nudge — Aseprite
   edit on the existing PNG is enough if the accent-only route is taken; a full hue
   change big enough to read as a new "material" may be worth a regeneration pass.
   Recommend trying the Aseprite accent first (no credits) before spending a regen.
5. **`hero_knight.png` and `hero_ranger.png` — no hero-only marker.** Add one small
   high-contrast accent (crest colour, cloak trim) that no soldier strip uses, so the
   tracked hero doesn't visually vanish into its own veterans/rangers. Aseprite edit on
   both existing PNGs. No credits.
6. **`staff_undead.png` — "greeneyes" identity marker not legible at review scale.**
   Enlarge/brighten the eye glow to a hard 2-3px dot. Aseprite edit on the existing PNG.
   No credits (this is restoring an already-generated detail, not adding new content).
7. **`undead2.png` vs. `vet_ranged.png`, column 0.** Shift `undead2`'s grey toward the
   duller enemy family, same direction as fix 1. Aseprite recolour on the existing PNG.
   No credits.
8. **`undead.png` — weakest, most ally-adjacent silhouette in the roster.** Either
   commit to it reading as the plainest/darkest fodder enemy (further darken, no
   weapon needed) or add a torn/claw silhouette detail so it isn't read purely by
   process of elimination. The darken-only route is an Aseprite edit, no credits; the
   claw-detail route needs a small regeneration pass and does spend credits.

## Canon proposals

- `docs/art/aesthetic-direction.md` has no written rule yet for what a unit's
  **back-facing quadrant (columns 3-5)** must preserve. Every biped in this roster
  loses its weapon/shield silhouette from directly behind, and since the player's own
  army is seen from behind by default in this 3D hall, that quadrant deserves the same
  scrutiny the south-facing anchor frame already gets. Proposed rule: every unit must
  keep at least one asymmetric silhouette element (weapon tip, shield edge, pack, cloak
  clasp) visible in columns 3-5, not just 0-2 and 7.
- No written rule yet reserves a hue *direction* for "the one hero on the field" versus
  "the army around it." Six of seven current heroes already got a one-off warm/saturated
  palette by instinct (`hero_samurai`, `hero_dwarf`, `hero_sackhauler`, `hero_turret`,
  `hero_cover`); `hero_knight` and `hero_ranger` didn't, and read as lost soldiers as a
  result. Worth writing down as a rule rather than leaving it to accident per-hero.
