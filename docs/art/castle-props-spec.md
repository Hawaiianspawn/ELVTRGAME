# Castle hallway props — cosmetic set (2026-08-24)

Target: `godot/scripts/Battle.gd` battle hall. 2.5D pinhole hallway, 8-facing billboards
bottom-aligned on the ground, drawn at ~1.3x pixel scale. Walls near-black tiled stone,
ceiling fades to black. The only light and only accent colour is **sickly green** —
`castle-art-pass.md` "Owner pick": *near-black stone, no visible sky, grime, collapsed
sections, the green glow as the only light.*

## Intent

Fiction: a keep nobody has lived in for a long time, lit by something that is not fire.
Gameplay: props are scenery, not units, so they must **never compete with the swarm** —
dark bodies, one green carrier at most. The `flame` set is the exception: it replaces the
procedural sconce dots in `_draw_walls` (green circles at `wall_h * 0.6`, marching toward
the camera on `scroll` every 160 world units). Those dots are the hall's **speed cue**; a
flame prop must keep doing that job — the green glow must dominate its silhouette so the
eye tracks it at fog distance.

## Palette (in words — prompt-driven, nothing enforces it)

| Role | Value |
|---|---|
| body / stone / iron | near-black, desaturated cold grey |
| wood / cloth | desaturated grey-brown, no warmth |
| the one accent | sickly green glow (flame, ghost, moss, mimic eye) |
| forbidden | blue, gold, orange, red, any warm light |

Every prompt carries the phrase **"near-black stone, desaturated grey wood, sickly green
glow as the only colour accent, no blue no gold no orange no warm light"** so each call
is self-contained. Off-palette leaks are expected (castle-art-pass #4/#5 needed retries
for exactly this); budget one retry per variant and reject on any warm hue.

Green-budget rule per prop: `flame` — green is the subject. `mirror` — one faint green
ghost, nothing else. `chest` — none, except the mimic hint's eye glint and bone-chest
bone-glow. `table` / `chair` — none (moss allowed as a dull green-grey, never glowing).

## Generation contract

`create_8_direction_object`, `view: "low top-down"`, size **64×64**, transparent
background, one prompt per variant, prompts below verbatim. Multi-candidate returns enter
review (`get_object` → `select_object_frames`); pick on **silhouette at 64px**, not detail.
Ground contact must be the lowest opaque row (billboards are bottom-aligned; a floating
prop reads as a bug).

## Props

### 1. `flame` — the speed flame

Readability: the green flame/glow must be the largest bright mass in every one of the 8
rotations; the fixture is a dark stalk under it. If a rotation hides the flame behind the
fixture, reject it. Flame should sit in the top third of the cell so it lands near
`wall_h * 0.6` when bottom-aligned.

| Slug | Silhouette feature |
|---|---|
| `flame_sconce` | wall bracket, flame off one side (asymmetric) |
| `flame_brazier` | wide low bowl on three legs, flame wide and squat |
| `flame_candelabra` | tall thin stalk, three small flames in a row |
| `flame_skull_lantern` | skull on a pole, green leaking from eye sockets and jaw |
| `flame_cage_lamp` | hanging iron cage on a chain, flame inside bars |
| `flame_urn` | cracked stone urn, flame spilling from the crack and the mouth |

### 2. `table`

Readability: table top must read as a flat plane from all 8 directions — legs are the
rotation carrier, keep them thick (2px min at 64). The overturned one must never read as a
standing table from any angle (legs up).

| Slug | Silhouette feature |
|---|---|
| `table_trestle` | long, low, two X-trestles, twice as wide as deep |
| `table_round` | circular top, single centre pedestal |
| `table_broken` | one leg gone, top tilted down one corner |
| `table_cloth` | square table, grey cloth draped to the floor, legs hidden |
| `table_map` | rectangular, scrolls and a candle stub on top, cluttered top |
| `table_overturned` | on its side, four legs pointing at the viewer |

### 3. `chair`

Readability: the back is the rotation carrier — high backs must stay tall from behind,
stools must stay backless. Toppled must read as toppled in all 8 (legs out sideways).

| Slug | Silhouette feature |
|---|---|
| `chair_throne` | very high back, wide arms, tallest prop in the set |
| `chair_stool` | three legs, no back, smallest prop in the set |
| `chair_bench` | long, no back, wide as a trestle table |
| `chair_broken` | back snapped off halfway, one splintered leg |
| `chair_iron` | thin iron rod frame, see-through back |
| `chair_toppled` | on its side, legs pointing sideways |

### 4. `mirror`

Readability: the glass must stay a pale-grey plane with a **faint green figure** inside
it from the front three facings; from behind, the frame backing is plain dark — the ghost
is allowed to vanish. Frame shape carries the variant; ghost is shared.

| Slug | Silhouette feature |
|---|---|
| `mirror_ornate` | tall rectangle, heavy carved frame (gilt gone grey) |
| `mirror_iron` | plain thin iron frame, rectangle |
| `mirror_oval` | oval on a stand, glass cracked corner to corner |
| `mirror_cheval` | tall full-length glass tilted in a swivel frame |
| `mirror_shard` | rectangle with the lower third of the glass missing |
| `mirror_draped` | square mirror with a grey dust sheet half pulled off one side |

### 5. `chest` — CLOSED state

Readability: lid line must be visible from all 8 (a horizontal seam at ~40% height); the
lock/clasp faces the front three facings. Chests are the only prop players will walk to,
so silhouette width is the variant carrier.

| Slug | Silhouette feature |
|---|---|
| `chest_wooden` | plain plank chest, two iron bands, domed lid |
| `chest_iron` | fully iron-plated, riveted, flat lid, squat |
| `chest_ornate` | carved, taller than wide, clawed feet |
| `chest_coffer` | small box, half the size of the others, single clasp |
| `chest_mimic` | wooden chest, lid slightly ajar, one sickly green eye glint in the gap |
| `chest_bone` | chest built from ribs and skulls, faint green in the sockets |

#### Chest OPEN edit (`create_object_state`, later)

One line, applied to each closed chest as a state:
`same chest, lid thrown fully open, interior dark and empty, no gold no coins no treasure, faint sickly green glow inside, same materials and palette`

(For `chest_mimic` swap "interior dark and empty" for "rows of teeth inside, green tongue".)

## Depends on

Neither (#5 does not apply — these are Godot `Sprite3D` billboards, not the Niagara
swarm). This set does **not** go through `pixelpipe.py` (no 4-value collapse, no 48px
lock) — it is a Godot hall asset at 64px per the owner brief, rendered in full colour
under the #6 lift.

## Canon proposals

- Note the 48×48 lock in `SETUP-EDITOR.md` as scoped to swarm units; hall props are 64px.
  If the owner wants one lock for everything, these prompts regenerate at 48 unchanged.
- The mimic and bone chests imply undead/necromancer set-dressing; canon names no faction.
  That is consistent with the "necromancer's green glow" already in `Battle.gd _draw`.

## Prompts

```json
[
{"prop":"flame","slug":"flame_sconce","prompt":"Rusted iron wall sconce bracket holding a large sickly green flame, flame bigger than the bracket, asymmetric off one side, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"flame","slug":"flame_brazier","prompt":"Wide low iron floor brazier bowl on three squat legs, broad sickly green flame filling the bowl and spilling over, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"flame","slug":"flame_candelabra","prompt":"Tall thin tarnished iron standing candelabra, three green candle flames in a row on top, stalk very narrow, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"flame","slug":"flame_skull_lantern","prompt":"Skull mounted on a short iron pole, sickly green fire pouring out of the eye sockets and jaw, glow larger than the skull, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"flame","slug":"flame_cage_lamp","prompt":"Hanging rusted iron cage lamp on a short chain, sickly green flame burning inside the bars and glowing through them, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"flame","slug":"flame_urn","prompt":"Cracked stone urn on the floor, sickly green flame spilling from its mouth and leaking through a long crack in the side, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_trestle","prompt":"Long low wooden trestle table, twice as wide as deep, two X-shaped trestle legs, scratched and dusty, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_round","prompt":"Round wooden table on a single thick centre pedestal, warped top, dust and cobwebs, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_broken","prompt":"Rectangular wooden table with one leg missing, top tilted down to the floor on that corner, splintered, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_cloth","prompt":"Square table fully draped in a torn grey cloth reaching the floor, legs hidden, cloth stained and dusty, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_map","prompt":"Rectangular wooden map table cluttered with rolled parchment scrolls, a burnt-out candle stub and a dagger on top, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"table","slug":"table_overturned","prompt":"Wooden table overturned on its side, all four legs pointing toward the viewer, top facing away, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_throne","prompt":"Very tall high-backed wooden throne chair with wide arms, carved back rising far above the seat, cracked and dusty, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_stool","prompt":"Small three-legged wooden stool, no back, round seat, very small and low, scuffed, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_bench","prompt":"Long low wooden bench, no back, plank seat on two slab legs, wide and low, worn and dusty, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_broken","prompt":"Wooden chair with its back snapped off halfway and one leg splintered, leaning, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_iron","prompt":"Chair made of thin rusted iron rods, see-through open frame back, spindly legs, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chair","slug":"chair_toppled","prompt":"Wooden chair knocked over lying on its side, legs pointing sideways, back flat on the floor, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_ornate","prompt":"Tall rectangular standing mirror in a heavy carved frame, gilt faded to dull grey, faint sickly green ghost figure visible in the pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_iron","prompt":"Plain rectangular mirror in a thin rusted iron frame on a simple stand, faint sickly green ghost figure visible in the pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_oval","prompt":"Oval mirror on a stand, glass cracked corner to corner, faint sickly green ghost figure visible in the pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_cheval","prompt":"Tall full-length cheval mirror tilted in a swivel wooden frame on two feet, faint sickly green ghost figure visible in the pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_shard","prompt":"Rectangular standing mirror with the lower third of the glass missing, shards on the floor, faint sickly green ghost figure in the remaining pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"mirror","slug":"mirror_draped","prompt":"Square standing mirror with a grey dust sheet half pulled off one side, faint sickly green ghost figure visible in the uncovered pale glass, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_wooden","prompt":"Closed wooden plank treasure chest with two dark iron bands and a domed lid, dusty, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_iron","prompt":"Closed squat chest fully plated in riveted rusted iron, flat lid, heavy padlock, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_ornate","prompt":"Closed ornate carved chest taller than it is wide, clawed feet, carvings worn grey, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_coffer","prompt":"Closed small wooden coffer box, half the size of a chest, single iron clasp, plain, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_mimic","prompt":"Closed wooden chest with the lid slightly ajar and a single sickly green eye glinting in the gap, otherwise ordinary, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"},
{"prop":"chest","slug":"chest_bone","prompt":"Closed chest built from bleached grey ribs and skulls lashed with iron, faint sickly green glow in the skull sockets, ruined castle prop, run-down not lived-in, near-black stone, desaturated grey wood, sickly green glow as the only colour accent, no blue no gold no orange no warm light, transparent background"}
]
```
