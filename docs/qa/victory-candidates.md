# Victory / wave-clear candidates — 2026-08-31

The owner wants to replace the current wave-clear cue. Candidates below were processed with the
house treatment (`py Scripts/art/sfx.py pick <src> <out> --len 1.5 --retro`) so they audition the
way a landed cue actually sounds; audition files live in `RawArt/Audio/victory-candidates/`.
To land one: re-run `pick` with the out-name `stinger_victory.wav` (no directory), then update the
`wave_clear` row in `docs/qa/sfx.md`. Wiring (`units.json` `hero_sfx.wave_clear`, `Battle._wave_done()`)
stays untouched.

**Current cue**: `stinger_victory.wav` — P7 RPG Orchestral Essentials `Discovery_01-03_Organ-A_WET.wav`,
1.50s, retro. Why it feels weak: it is a soft church-organ "discovery" motif — a gentle wet swell with
no transient front, so after a hall of undead collapses it reads as a save-point noise, not a held line
winning. The 1.5s cap also fades it mid-ring.

All candidates 1.50s, 44.1kHz 16-bit mono, -1 dBFS peak, retro-treated. Rank 1 = top recommendation.

| # | File | Source (Sonniss) | Dur | Character | Rank — why |
|---|---|---|---|---|---|
| 01 | `01_horn_braam_dark.wav` | P9 `Jake Fielding - Cinematic Horn Braams/DSGNBram____Cinematic Horn Braam, Epic, Cinematic, Dark, Instrument, Huge-32.wav` | 1.50s | Huge dark brass stab — low horn hit with a grim tail | **1** — triumphant-but-grim in one gesture; instant front, fits the castle register exactly |
| 02 | `02_horns_folk_raw.wav` | P7 `Ivo Vicic - Bellmen - Folk Custom/23 BELLMEN_Traditional horns.wav` | 1.50s | Raw folk horn blast, field-recorded — ragged medieval war-horn call | **2** — the most "a garrison sounding the all-clear" of the set; rougher, very in-world |
| 03 | `03_bell_toll_church.wav` | P9 `Ivo Vicic - Church Bells/04 Church Bells, Near Distance, In Church Tower-3 Different Bell 02.wav` | 1.50s | Real church-tower bell toll, near — strike plus shimmering ring | **3** — bell toll over a cleared hall is strong theming; slightly soft attack after retro |
| 04 | `04_gong_hit.wav` | P8 `Orbital Emitter - Cinematic Transitions for Editors Volume 2/80,TheGong.wav` | 1.50s | Single big gong strike, long dark bloom | **4** — weighty and ominous; reads more "chapter turn" than "you won" |
| 05 | `05_drum_steel_impact.wav` | P8 `BluezoneCorp - Modern Cinematic Impact/Bluezone_BC0294_modern_cinematic_impact_percussion_009.wav` | 1.50s | Hard drum-and-steel percussion hit, very front-loaded punch | **5** — punchiest transient of the set, but tonally close to the combat impact cues, may not read as a reward |
| 06 | `06_ensemble_swell_grim.wav` | P7 `InspectorJ - RPG Orchestral Essentials (Music FX)/Mystery_01-11_Ensemble-Large_WET.wav` | 1.50s | Large orchestral ensemble swell — closest thing to a low choir hit in the library | **6** — right darkness but swells instead of hitting; same soft-front problem as the current cue |

No true choir one-shot exists in the curated library; 06 is the nearest register. If 01 wins but
wants more weight, `--layer` it over 04's gong (`sfx.py pick` supports `--layer <wav> --layer-db -8`).
