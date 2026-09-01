# Kindled combat SFX — cue table

Sound source is the Sonniss GameAudioGDC bundles (royalty-free, https://sonniss.com/gameaudiogdc),
extracted under `RawArt/Audio/<bundle>/<pack>/` (Parts 7/8/9, gitignored). All 33 slots plus
the three tank cues were picked 2026-08-29 with `--retro` (11025 Hz sample-hold, 8-bit, ~4.6 kHz anti-alias lowpass); candidates per slot are in `sfx-shortlist.md`.
Silent stand-ins for any new slot come from `py Scripts/art/sfx.py placeholders`.

To (re)pick a row:

    py Scripts/art/sfx.py pick "RawArt/Audio/<bundle>/<pack>/<original file>.wav" <file> --len <duration>

then update this table's Source pack / Original filename / Duration columns — that's the record
that a cue has been picked (per `docs/art/CHARACTER-PIPELINE.md`).

## Unit cues (`units.json` → `"<type>": {"sfx": {...}}`)

| Unit type | Cue | File | Search terms | Source pack | Original filename | Duration |
|---|---|---|---|---|---|---|
| veteran | attack | `sword_swing_metal.wav` | sword, whoosh, swing, steel | P9 Melee Weapons Sound Effects Pack 2 | `METLFric_SWING SCRAPE Swift Melee Weapon Swing With A Long Blade 14_DDUMAIS_MWP2.wav` | 0.80s |
| veteran | hit | `impact_flesh_light.wav` | flesh, impact, hit, punch | P9 Cinematic Fight Vol. 1 | `FGHTImpt_4 x Punch, Body 02_344 Audio_Cinematic Fight Vol 1.wav` | 0.50s |
| veteran | death | `death_grunt_male.wav` | death, grunt, voice, male | P9 Humanoid Creatures Vol 4 | `VOXReac_Construction Kit Male Flutter Death Vocal Stuttered Long 05_ESM_HC4.wav` | 1.20s |
| veteran | ability (whirl) | `whoosh_blade_spin.wav` | whoosh, sword, spin, blade | P9 Tower Defense Game | `DSGNStngr_Action Deploy Units Sword Slice Special Move Layered Swish 04_ESM_TDG.wav` | 1.00s |
| halberdier | attack | `spear_thrust_metal.wav` | sword, whoosh, thrust, polearm | P9 Historical Weapons Vol. 2 | `WEAPBlnt_Spear And Stick Impact, Wooden MKH 2_344 Audio_Medieval Weapons Vol 2.wav` | 0.80s |
| halberdier | hit | `impact_flesh_light.wav` | flesh, impact, hit, punch | P9 Cinematic Fight Vol. 1 | `FGHTImpt_4 x Punch, Body 02_344 Audio_Cinematic Fight Vol 1.wav` | 0.50s |
| halberdier | death | `death_grunt_male.wav` | death, grunt, voice, male | P9 Humanoid Creatures Vol 4 | `VOXReac_Construction Kit Male Flutter Death Vocal Stuttered Long 05_ESM_HC4.wav` | 1.20s |
| halberdier | ability (sweep) | `impact_bulldoze_heavy.wav` | impact, heavy, charge, crash | P7 Epic Impacts Vol. 1 | `Impact 021.wav` | 1.00s |
| hammer | attack | `hammer_swing_heavy.wav` | ice, hit, impact, crack | owner drop (P7 Pole Position The Frozen Lake) | `RawArt/Audio/Ice - HIT - Frozen Lake Surface - Impact - Break Through to Water - Wide AB - MKH8060 CUT SHORTER.wav` `--len 0.8` (owner pick 2026-08-31; was MWP2 swing-thud) | 0.80s |
| hammer | hit | `impact_armor_heavy.wav` | armor, impact, metal, clang | P8 Melee Weapons | `WEAPArmr_Metal Shield Block Hits_JSE_MW.wav` | 0.70s (8 kHz) |
| hammer | death | `death_grunt_heavy.wav` | death, grunt, heavy, voice | P7 Flesh, Bones & Gore | `Heavy Body Crunch 02.wav` | 1.20s |
| hammer | ability (slam) | `impact_ground_slam.wav` | impact, slam, ground, drum | P8 Stone Impact + P7 Building Collapse | `Bluezone_BC0297_stone_impact_hammer_015.wav` `--layer "Bluezone_BC0275_building_collapse_debris_falling_rock_rubble_008.wav" --layer-db -8` | 0.70s |
| vet_ranged | attack | `bow_release_arrow.wav` | bow, arrow, release, twang | P9 Melee Weapons Sound Effects Pack 2 | `WEAPWhip_WHIP Snap Crack 05_DDUMAIS_MWP2.wav` | 0.50s |
| vet_ranged | hit | `impact_flesh_light.wav` | flesh, impact, hit, punch | P9 Cinematic Fight Vol. 1 | `FGHTImpt_4 x Punch, Body 02_344 Audio_Cinematic Fight Vol 1.wav` | 0.50s |
| vet_ranged | death | `death_grunt_male.wav` | death, grunt, voice, male | P9 Humanoid Creatures Vol 4 | `VOXReac_Construction Kit Male Flutter Death Vocal Stuttered Long 05_ESM_HC4.wav` | 1.20s |
| vet_ranged | ability (volley) | `arrow_volley_whoosh.wav` | arrow, whoosh, volley, bow | P9 Elemental Palette Designed Vol. 1 | `WINDDsgn_Wind, Rush, Whoosh, Long x5 01_344 Audio_Elemental Palette Designed Vol 1.wav` | 1.00s |
| ooze | attack | `ooze_splat_wet.wav` | ooze, wet, splat, slime | P8 Gore Mini Pack | `GORESplt_Gore Splatter 01_JSE_GMP.wav` | 0.70s |
| ooze | hit | `impact_flesh_wet.wav` | flesh, wet, squish, impact | P8 Gore Mini Pack | `GOREFlsh_Flesh Drops on Floor 03_JSE_GMP.wav` | 0.60s |
| ooze | death | `death_ooze_dissolve.wav` | death, ooze, dissolve, slime | P9 Vox Bestiae - Source Elements | `CREAAqua_Aquatic Creature Gurgling 2_SNDBTS_VB-SE.wav` | 1.50s |
| undead | attack | `bone_swing_light.wav` | bone, whoosh, swing, light | P7 Transition Whooshes Vol. 1 | `SWSH_Woodstick Swish 03_JSE_TW1.wav` | 0.60s |
| undead | hit | `impact_bone_crack.wav` | bone, crack, impact, skeleton | P7 Melee Weapons Sound Effects Pack 1 | `WOODImpt_Impact Wood 23_DDUMAIS_NONE.wav` | 0.50s (8 kHz) |
| undead | death | `death_undead_moan1.wav` | zombie, moan, guttural, deathrattle | owner drop (ESM Humanoid Creatures 4) | `RawArt/Audio/CREAHmn_Designed Orc Male Attack Long Heavy Hit Charged Up 03_ESM_HC4 CUT INTO SMALL BITES .wav` `--start 0 --len 0.9 --pitch -4` (bite 1 of 3, owner pick 2026-08-31) | 0.90s |
| mace_undead | attack | `mace_swing_heavy.wav` | impact, whoosh, heavy, mace | P7 Melee Weapons Sound Effects Pack 1 | `SWSH_Swing 3 Large 03_DDUMAIS_NONE.wav` | 0.51s |
| mace_undead | hit | `impact_bone_crack.wav` | bone, crack, impact, skeleton | P7 Melee Weapons Sound Effects Pack 1 | `WOODImpt_Impact Wood 23_DDUMAIS_NONE.wav` | 0.50s (8 kHz) |
| mace_undead | death | `death_undead_moan2.wav` | zombie, moan, guttural, deathrattle | owner drop (ESM Humanoid Creatures 4) | same source `--start 0.9 --len 0.9 --pitch -4` (bite 2 of 3) | 0.90s |
| staff_undead | attack | `magic_staff_cast.wav` | magic, spell, staff, cast | P9 Emotion and Magic | `magic, action gesture, evil presence, onslaught-004.wav` | 1.20s |
| staff_undead | hit | `impact_bone_crack.wav` | bone, crack, impact, skeleton | P7 Melee Weapons Sound Effects Pack 1 | `WOODImpt_Impact Wood 23_DDUMAIS_NONE.wav` | 0.50s (8 kHz) |
| staff_undead | death | `death_undead_collapse.wav` | death, undead, collapse, bone | P7 Zombie Specimens Vol. 2 + P8 Gore Mini Pack | `CREAHmn_Test Subject 4 05_344 Audio_Zombie Specimens Vol 2.wav` `--pitch -6 --layer "GORESplt_Gore Splatter 01_JSE_GMP.wav"` (moan bite 3 cut 2026-08-31: too long) | 1.50s |
| bow_undead | attack | `bow_release_arrow.wav` | bow, arrow, release, twang | P9 Melee Weapons Sound Effects Pack 2 | `WEAPWhip_WHIP Snap Crack 05_DDUMAIS_MWP2.wav` | 0.50s |
| bow_undead | hit | `impact_bone_crack.wav` | bone, crack, impact, skeleton | P7 Melee Weapons Sound Effects Pack 1 | `WOODImpt_Impact Wood 23_DDUMAIS_NONE.wav` | 0.50s (8 kHz) |
| bow_undead | death | `death_undead_collapse.wav` | death, undead, collapse, bone | P7 Zombie Specimens Vol. 2 + P8 Gore Mini Pack | `CREAHmn_Test Subject 4 05_344 Audio_Zombie Specimens Vol 2.wav` `--pitch -6 --layer "GORESplt_Gore Splatter 01_JSE_GMP.wav"` | 1.50s |
| archer_undead | attack | `bow_release_arrow.wav` | bow, arrow, release, twang | P9 Melee Weapons Sound Effects Pack 2 | `WEAPWhip_WHIP Snap Crack 05_DDUMAIS_MWP2.wav` | 0.50s |
| archer_undead | hit | `impact_bone_crack.wav` | bone, crack, impact, skeleton | P7 Melee Weapons Sound Effects Pack 1 | `WOODImpt_Impact Wood 23_DDUMAIS_NONE.wav` | 0.50s (8 kHz) |
| archer_undead | death | `death_undead_collapse.wav` | death, undead, collapse, bone | P7 Zombie Specimens Vol. 2 + P8 Gore Mini Pack | `CREAHmn_Test Subject 4 05_344 Audio_Zombie Specimens Vol 2.wav` `--pitch -6 --layer "GORESplt_Gore Splatter 01_JSE_GMP.wav"` | 1.50s |
| horse_undead | attack | `horse_charge_impact.wav` | impact, horse, charge, hoof | P7 Epic Impacts Vol. 1 | `Impact 045.wav` | 1.00s |
| horse_undead | hit | `impact_bone_crack_heavy.wav` | bone, crack, heavy, impact | P7 Flesh, Bones & Gore | `Heavy Body Crunch 02.wav` | 0.80s |
| horse_undead | death | `death_horse_collapse.wav` | death, horse, collapse, animal | P8 Horsin' Around – Character Play Set | `ANMLHors_Horse, Quarter Horse, Mare, Snort_Uberduo_HORS_Mono-002.wav` | 0.71s |
| armored | attack | `armor_swing_heavy.wav` | armor, whoosh, heavy, metal | P9 Elemental Palette Designed Vol. 1 | `METLMisc_Metal, Slow Whoosh, Rattle, Pass By x4 01_344 Audio_Elemental Palette Designed Vol 1.wav` | 0.90s |
| armored | hit | `impact_armor_heavy.wav` | armor, impact, metal, clang | P8 Melee Weapons | `WEAPArmr_Metal Shield Block Hits_JSE_MW.wav` | 0.70s (8 kHz) |
| armored | death | `death_armor_collapse.wav` | death, armor, collapse, metal | P8 Torturing Metal | `METLCrsh_Drop Fall Metal Rattle Scrap Debris_06_MWSFX_TM.wav` | 0.73s |

## Hero cues (`units.json` → top-level `"hero_sfx"`)

| Cue | File | Search terms | Source pack | Original filename | Duration | Wired? |
|---|---|---|---|---|---|---|
| hit | `hero_hurt_grunt.wav` | voice, hurt, grunt, pain | P9 Humanoid Creatures Vol 4 | `VOXReac_Construction Kit Male Flutter Death Vocal Stuttered Long 05_ESM_HC4.wav` | 0.50s (same voice as veteran death, onset only) | Yes — `Battle.gd` where `hero_hp -= d` |
| low_hp | `hero_low_hp_warn.wav` | ui, stinger, warn, heartbeat | P9 System & UI Feedback Elements | `Interface Deny Low Fat Dark.wav` | 0.74s | Yes — once per wave, crossing below 25 hp |
| wave_clear | `stinger_victory.wav` | horn, victory, fanfare, medieval | P7 Ivo Vicic Bellmen - Folk Custom | `23 BELLMEN_Traditional horns.wav` `--len 0.7 --pitch -2 --detune 22 --echo 170,0.5` (echo+synth take on candidate 02, replaces the organ swell 2026-08-31) | 1.21s | Yes — `Battle._wave_done()` |
| lose | `stinger_defeat.wav` | stinger, drum, defeat, low | P7 RPG Orchestral Essentials | `Failure_01-10_Ensemble-Small_DRY.wav` | 1.43s | Yes — the `hero_hp <= 0.0` branch |
| relic | `ui_relic_chime.wav` | ui, chime, magic, sparkle | P9 Fantasy Game 2 | `MAGAngl_Magic Light Spell Enchantment Potion Effect Tonal Bright 03_ESM_FG2.wav` | 1.20s | Yes — the `say("Relic: ...")` site |

## Ambient (looping, `Sound.ambient()` — not a pooled cue)

| Where | File | Source | Recipe | Duration |
|---|---|---|---|---|
| Main menu | `ambient_menu.wav` | owner drop (344 Audio Geophonic Design) | `RawArt/Audio/AMBDsgn_Descending, Metallic, Drone, Scraping_344 Audio_Geophonic Design.wav` — `sfx.py loop --start 62 --len 24 --xfade 1.5`, plays at -14 dB, loop seam crossfaded | 24.0s |

## Spell cues (`spells.json` → per-spell `"sfx"`)

| Spell | File | Search terms | Source pack | Original filename | Duration |
|---|---|---|---|---|---|
| bolt | `magic_bolt_cast.wav` | magic, spell, cast, zap | P7 Magic Sound FX Pack 2 | `MagicPack2_ElectricitySpells_Spell2-002.wav` | 1.00s |
| heal | `magic_heal_chime.wav` | magic, spell, chime, heal | P7 Dreamcatcher | `Harp_Glissando_10b.wav` | 1.20s |
| wall | `magic_wall_cast.wav` | magic, spell, cast, rumble | P7 Magic Sound FX Pack 2 | `MagicPack2_Earth_MeteorShower003.wav` | 1.50s |

34 distinct filenames cover 41 cue slots (units share files where the source category matches —
e.g. every bow user shares `bow_release_arrow.wav`, every undead shares its bone/collapse pair).

## Tank cues (`Battle.gd` literals, not data-driven)

| Cue | File | Source pack | Original filename | Duration | Wired? |
|---|---|---|---|---|---|
| gatling shot | `gatling_shot.wav` | P7 The Black Powder Guns Library | `Rifled Flintlock Pistol M1820 - FIRING - Close - MS Decoded - VP88.wav` | 0.11s (8 kHz, -2 st; bang only) | Yes — `_fire_gatling()` |
| cannon fire | `cannon_fire.wav` | P8 Steampunk Weapon And Textures | `Bluezone_BC0296_steampunk_weapon_cannon_shot_013_02.wav` | 1.50s | Yes — `_fire_cannon()` |
| shell explosion | `cannon_explosion.wav` | P8 Detonation - Explosion | `Bluezone_BC0277_explosion_mortar_002_01.wav` | 1.50s (`--pitch -3`) | Yes — `_cannon_explode_at()` |
