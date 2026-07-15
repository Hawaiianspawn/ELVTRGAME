# Bree's Stairwell — set-piece spec (flag N1, three states)

**Brief:** `../briefs/brief-002-brees-stairwell.md` · **Source fiction:** `../narrative/warden-captain-bree.md`
**States (N1):** **held** (permanent safe room) · **unheld** (recruited; watch ended) · **fallen** (Legion strongpoint)
**Companion:** `warden-captain-bree.md` (shares the world anchor and the honest-light proposal)

**Naming note:** unaffected by the role-only reversal — this is an environment/set-piece spec,
not a hero spec, and Bree (the NPC it's named for) keeps her name regardless (see
`warden-captain-bree.md`'s naming note). This file needed a redraw only for the palette-hex
revision and a scoped chibi-adjacency note (§3) — the barricade/lamp geometry itself has no
character proportions to redraw.

---

## 1. Intent

Fiction: unchanged — "a barricade built of Legion shields turned inward, every one scoured clean
of the crown sigil, and above them a single lamp — the only honest light the party has seen
since the gates." Gameplay, per the brief: **held must read as safe before the player commits**
— approaching at speed, from any stair direction, on a hostile floor; **fallen must be
unconfusable with held**; **unheld reads emptied, not destroyed** — she left, she didn't lose.

One rule carries all three, unchanged by either 2026-07-11 revision: **the bright value means
safety, and only the held state has it.** Held owns the floor's only warm glow; fallen has *zero*
bright pixels and restored crowns; unheld is the held geometry with the light taken down.

**What did change:** the anchor and bright hexes below now match the redrawn hero specs
(`hero-palettes.md`'s Gatecamp Family), and the Legion accent value now matches the redrawn
Still Legion family, so the whole floor — architecture and characters alike — reads off one
consistent world palette. Nothing about the *mechanism* (bright = safety, scarcity = trust)
changed; only the specific hex values did.

---

## 2. Palette table

Highgates biome environment palette. 4 values + transparent mask (mask backs the lamp/prop
cells only; barricade cells are fully opaque — does **not** count as a value).

| Hex | Name | Role |
|---|---|---|
| `#211210` | Vault Dark | outlines, stair shadow, shield recesses; also the dither ground for the lamp halo. Now the same literal hex as the Gatecamp hero family's anchor (was `#1a1c2c`) — the whole world shares one dark ground state |
| `#3e4a5a` | Gate Stone | masonry / barricade body mid-tone — environment-only value, not part of either hero-scale family (Gatecamp or Legion); unchanged by this revision |
| `#555568` | Legion Cold | edging, fittings — **the Legion faction-reserved value**: crown sigils render in this and nothing else. Re-hexed to match the redrawn Still Legion family's Legion Steel (was `#6a7b8c`), so a crown sigil here and a Legion unit's cold-mid elsewhere on the floor read as the same material |
| `#f0c260` | Gatecamp Bright | bright — **held state only**: lamp pixel + 1px halo dither. Retired (unused) in unheld and fallen. Same literal hex as every hero-scale honest light in the game (was `#ffe9c2` Watch-Lamp under the retired system) |

- **Mark protection:** quarry-mark and rune-mark carrier shapes (contour, dot-cluster) are
  absent from this palette and unapproximated — the same shape-audit rule as the hero specs
  (`hero-palettes.md` §3) applies to set-pieces too: only point+halo may spend the bright here.
- **Light-shifted variant** (held state, inside lamp radius — the honest-light rule): 
  `#211210→#35211c`, `#3e4a5a→#566274`, `#555568→#75758a`, bright unchanged. The landing itself
  sits one value brighter than the corridor feeding it — the safe read arrives before the
  player can resolve a single tile, from any approach vector.

---

## 3. Silhouette guide

Landing = 3 barricade tiles + 1 lamp/prop tile above center. Coarse mock, 1 char ≈ 2×2 px per
48×48 tile (`#`=Vault Dark, `=`=Gate Stone, `c`=Legion Cold, `*`=Gatecamp Bright, `.`=halo
dither).

```
HELD                          FALLEN
      .....                        c
      ..*..   <- one lamp         ccc      <- post-standard,
      .....                        c          cold, geometric
 #===#===#===#                #===#===#===#
 #= ==== == =#  shields       #=c==c==c==c#  <- crowns RESTORED,
 #===#===#===#  turned in,    #===#===#===#     shields re-hung
 #= ==== == =#  faces BLANK   #=c==c==c==c#     facing OUT
 #===#===#===#                #===#===#===#

UNHELD = HELD geometry, lamp cell -> empty bracket (dark), a folded
watch-ledger left on the center tile. No rubble. Nothing broken.
```

- **Held** — *reads as:* "one honest light over a wall of blank shields." Inward-turned,
  scoured shield faces mean the wall protects what's behind it; the lamp is the game's
  restoration grammar (Sanctuary / braziers) planted on architecture. Stillness completes it:
  nothing on the landing moves but the lamp flicker.
- **Unheld** — *reads as:* "a watch that ended." Identical silhouette, light gone, bracket
  empty, ledger folded. Zero damage tiles — emptied, never ruined.
- **Fallen** — *reads as:* "a crowned checkpoint where the light used to be." Shields re-hung
  facing outward with crown sigils restored in Legion Cold, barricade extended one tile each
  side (they built *over* her post), a cold geometric post-standard where the lamp hung, and a
  garrison spawn. No bright value anywhere.

**Chibi-adjacency note (scoped, not resolved here):** the barricade/lamp tiles are architecture,
not characters, so they carry no proportion of their own to redraw. The one open question this
pass surfaces but does not resolve: when Bree stands at the center tile (held state, N1 = met),
her sprite now renders at the chibi head-dominant proportion specced in `warden-captain-bree.md`
§3, and the barricade/lamp were originally laid out against the old, taller realist proportion.
Whether the lamp/shield-top heights need a pixel-level nudge to sit correctly beside a shorter,
bigger-headed unit is a follow-up art-test item, not decided here — flagged rather than guessed
at, per the exact-ratio question `aesthetic-direction.md` §4 decision 6 leaves open.

**Horde-scale check:** at gameplay zoom with 500 units on the floor, held is the only warm
glow + brightness-shifted pocket on screen (unforgeable — Legion-family palettes contain no
bright value); fallen is cold-value crowns + outward geometry + moving garrison. The two states
share zero signals: light vs. none, blank-in vs. crowned-out, still vs. patrolled. Unconfusable
at sprint speed.

---

## 4. Sheet layout

- Cell: **48×48 px** · Grid: **4×4** (power-of-two per pipeline) · Sheet: **192×192 px**
- Static level props on instanced quads through the standard pipeline (Nearest, NoMipmaps,
  Unlit Masked). SubImageIndex = `state_row * 4 + column` — the N1 flag is literally the row
  selector; one cheap parameter, no per-state materials. Grid/cell size unchanged by this
  revision.

| Cell | Row / frame |
|---|---|
| 0–3 | **HELD:** barricade-L · barricade-mid · barricade-R · lamp lit (frame A) |
| 4–7 | **UNHELD:** barricade-L · barricade-mid (+folded ledger) · barricade-R · empty bracket |
| 8–11 | **FALLEN:** barricade-L (crowned) · barricade-mid (crowned) · barricade-R (crowned) · post-standard |
| 12 | Lamp lit (frame B — flicker pair with cell 3) |
| 13 | Fallen extension tile (the +1 each side) |
| 14–15 | Reserved / blank |

---

## 5. Animation notes

- **Lamp flicker, 2f (cells 3 ↔ 12), ~1.5 Hz:** bright pixel offsets 1px, halo dither pattern
  rotates. The only motion the held state is allowed — a still landing with one living light.
- **Everything else static.** Stillness codes safety in held and absence in unheld; the fallen
  state gets its motion from garrison *units*, never from tiles. Motion budget spent where the
  silhouette table says it belongs.
- If Bree is present at her post (N1 = met/held), her unit sprite (`warden-captain-bree.md`,
  cells 4–5 post idle, now chibi-proportioned) stands at the center tile; her shoulder-lamp and
  the barricade lamp flicker on offset phases so the landing never strobes in sync.

---

## 6. Depends on

- **#5 (flipbooks vs 3D):** written for **instanced quads** (flipbook side); the sheet doubles
  as a texture atlas under flat-shaded 3D with states as mesh/material-parameter swaps, so this
  spec survives either answer — layout unchanged, only the renderer differs.
- **#6 (global vs per-faction palettes):** assumes **per-faction/per-biome swaps** (Highgates
  ramp + held-state light shift). Under a strict global palette the state contrast still works
  — it rides on bright-value scarcity and crown placement — but loses the brightness-shifted
  pocket, weakening the "safe before commit at speed" guarantee. Flagging: the brief's hardest
  requirement is materially better served by the per-faction answer.

---

## Canon proposals

1. **Depends on spec-001 proposal 2 (the honest-light rule)** — unchanged: the held state's
   one-value-brighter landing requires lamp-radius palette shift to be generalized from the
   Lampbearer ability to named honest-light fixtures.
2. **Safe-room grammar** (one line for GDD §2 pillar 4 or WORLD.md §6a) — unchanged: *safe rooms
   are the only places the bright palette value appears outside combat, and safe rooms do not
   move.* Glow + stillness = safety, globally.
