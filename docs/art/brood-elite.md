# Brood Elite — swarm-punishing elite enemy

| | |
|---|---|
| **Subject** | Brood elite — swarm-punishing enemy, monster kind, moving |
| **Cell** | 56×56 (not 48: the reared mass and splayed tendril fan overhang a 48px cell; +8px keeps the fan off neighboring cells in the atlas) |
| **Request path** | `docs/data/art/requests/brood-elite.json` |
| **Texture name** | `T_BroodElite` |
| **Binds to** | GDD §7 (swarm-punishing elites as soft cost backing retinue upkeep; anti-cap philosophy) · GDD "Design tensions" (screen readability: value channel per faction; Hold is porous — the tide bites what is near its path) · `aesthetic-direction.md` Direction B bundle (4-value philosophy, honest-light rule, outline/dither rules, brood language) + 2026-07-28 amendment (full colour ships; value model survives as hierarchy) + 2026-07-31 amendment (arms register open — no bearing here, but binding on any kit read) + §4 decision 2 (red reserved: never enemy-coding) · FLAME-FOUNDATION §2 (every fiction names its mechanic), §5 (no new proper nouns) |

## 1. Intent

**Fiction.** Of the brood: amorphous, non-geometric, one of the dark's creatures — but
where the walkers flow *along* the ground toward whatever is near their path, this one
*rears*. It is a mass of the dark that stands up over a crowd and falls on it. That
image exists to do one job: it is the visible body of GDD §7's **swarm-punishing
elite**, the soft cost that backs retinue upkeep against the anti-cap philosophy. The
player who banks a huge clumped army must see, at horde scale, the specific shape that
exists to punish that clump — and see it *before* it strikes, because the rear-up is
the telegraph. No name, no lore beyond "of the brood"; the unit is its mechanic wearing
the dark's shape language (FLAME-FOUNDATION §2/§5).

**Gameplay.** Punishes clumped masses with an area strike. The sprite serves that
mechanic three ways: the **vertical reared silhouette** is the threat identifier at any
distance (this is the anti-clump unit — scatter); the **rear-up pose held before the
strike** is the honest telegraph window (the counterplay is repositioning, so the read
must precede the hit); and a **strike-radius telegraph** marks where the punishment
lands so it is legible, not arbitrary — the soft cost only governs army size if the
player can see it coming and route around it. Whether that radius mark may be a
*bright, light-emitting* ring is gated on an open owner call (§2 slot 4); the pose
telegraph carries the read on its own until that lands.

## 2. Palette

Per the 2026-07-28 amendment the game ships full colour: this table is the **value
hierarchy** the full-colour render must preserve (what dominates, what is scarce, what
never appears), not a literal four-hex quantize. Slots follow the Direction B
philosophy — shared darkest anchor, faction channel third, bright scarce and earned.

| Slot | Value | Role on this sprite |
|---|---|---|
| 1 | Darkest anchor (Vault Dark family) | The entire body mass, flat and unmodeled. Silhouette does all the work; internal detail would break the brood read. |
| 2 | Dark-mid | Sprite-vs-sprite separation only (Direction B outline rule: darkest-value outlines where sprite meets sprite, none against dark floors) and the interior folds visible when reared. |
| 3 | Faction channel (sickly, per Direction B's brood slot) | A thin membrane rim along the tendril-fan edge. This is the 1–2px faction tell that keeps it enemy-coded against warm-channel friendlies (GDD readability tension: one value channel per faction). The strike-radius telegraph also lives here: during the held rear-up, the membrane rim widens and the interior folds (slot 2) flush to this sickly value — a palette shift within the body, not emitted light, so the tell reads at horde scale without touching the open light question below. |
| 4 | Bright | **Absent — at rest and, for now, in the telegraph too.** The obvious upgrade — a bright ring of bare points marking the strike radius — would make this enemy *emit its own light*, and whether anything but the bearer's flame may emit light is an **open owner call** (the 2026-07-31 prop-light question: "lit by the bearer's flame" vs. "ambient dark, not props" — neither decided). A creature of the dark self-lighting cannot be settled by a sprite spec; **flagged for owner decision, do not build.** Until it lands, the telegraph is slot-3 only (above). If the owner opens emitted light to enemies: sickly channel pushed bright, never warm (warm bright = honest safety, reserved) and **never red** (rubrication is cost/temptation only, never enemy-coding — §4 decision 2). |

Dither: 2×2 minimum on the moving mass, per Direction B. No 1px halftone (static-UI
register only).

## 3. Silhouette

**Shape language.** Amorphous and non-geometric — no straight lines, no bilateral
kit-shapes, no held-object rectangles; the brood language, kept. Spent on a **vertical**
mass: a narrow rooted base, a body that rears to near cell height, and a splayed
radial fan of tendrils at the top — the shape of something cresting over a crowd. One
enclosed **negative-space hole** through the upper mass (a gap the background reads
through) gives it a topology no walker has. The scale-and-complexity gap against the
chibi gameplay register is deliberate: this thing should not share the crowd's cuteness.

**Reads as:** *the dark rearing up over the crowd it is about to fall on.*

**South frame.** Facing camera: base narrow (≤⅓ cell width) so the height reads;
mass rises past ¾ cell height; tendril fan splayed and asymmetric at the top edge;
the negative-space hole open and readable at horde scale; membrane rim (slot 3) on the
fan edge; zero bright pixels — slot 4 is empty pending the owner call (§2).

**Disjoint from the nine brood walker variants.** All nine walkers (task-059 atlas)
are low, wide-aspect, ground-hugging masses — their variant axis spends topology and
interior *within* that low aspect. The elite takes the one axis no walker uses:
**vertical aspect**. Tall-and-rooted vs. low-and-flowing disambiguates at 1–2px,
silhouette alone, motion included — a walker never rises past half cell height, the
elite never spreads past a third of cell width at the base. The enclosed hole and the
crest-fan are the backup tells for partial occlusion inside a horde. Any one of the
three axes (aspect, hole, fan) disagreeing is enough; all three disagreeing is the
design.