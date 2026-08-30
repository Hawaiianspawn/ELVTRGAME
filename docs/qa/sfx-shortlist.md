# Sonniss GDC bundle shortlist — Kindled

Narrowed from the three Sonniss GameAudioGDC filelists to what the Godot build needs. Companion
to `sfx.md` (the 33 wired slots) — this is the shopping list, that is the provenance record.
Filenames are verbatim from the sheets; `P7`/`P8`/`P9` = which bundle holds the file.

## Bundles

| Part | Bundle | Filelist sheet | Rows |
|---|---|---|---|
| 9 | GDC 2026, 5 zips (7.47 GB) | https://docs.google.com/spreadsheets/d/1MkoGwA6FfgNXhye9wLnY0gNLvjEp4H2iYXM1YxMI6Qs | 413 |
| 8 | GDC 2024, 9 zips | https://docs.google.com/spreadsheets/d/1Gnk0_PXG-HdRmttxridsb8lkfu64v2vlvB1iocL2Qjk | 1050 |
| 7 | GDC 2021-2023, 14 zips | https://docs.google.com/spreadsheets/d/1HAJdNA-QIug2IZjUV-DwCo1IZ-XJ4Vv9gWIaumNScfc | 501 |

Zips: `https://downloads.sonniss.com/Sonniss.com-GDC2026-GameAudioBundle{1..5}of5.zip`,
`…GDC2024-GameAudioBundle{1..9}of9.zip`, `…GDC2023-GameAudioBundle{1..14}of14.zip`.
Archive index: https://sonniss.com/gameaudiogdc — note it links the 2023 sheet for both Part 7
and Part 8; the real Part 8 sheet is the `1Gnk0…` one above.

The sheets don't say which zip holds which library, so narrowing is per-bundle, not per-zip.
Part 9 is small — grab all five. Parts 7/8: grab all, or list each zip before extracting.

Drop everything at `RawArt/Audio/sonniss/<pack>/`, then per row:

    py Scripts/art/sfx.py pick "RawArt/Audio/sonniss/<pack>/<original>.wav" <slot>.wav --len <dur>

and fill the Source/Original/Duration columns in `sfx.md`.

## Existing 33 slots (`sfx.md`)

First candidate is the recommendation; the rest are fallbacks.

### Melee / ally

| Slot | Candidates |
|---|---|
| `sword_swing_metal` | P9 `METLFric_SWING SCRAPE Swift Melee Weapon Swing With A Long Blade 14_DDUMAIS_MWP2.wav`; P7 `SWSH_Swing 3 Large 03_DDUMAIS_NONE.wav` |
| `spear_thrust_metal` | P9 `WEAPBlnt_Spear And Stick Impact, Wooden MKH 2_344 Audio_Medieval Weapons Vol 2.wav`; P9 `WEAPSwrd_Sword Slide Cuts, Metallic, Impact CM4 2_344 Audio_Medieval Weapons Vol 2.wav` |
| `hammer_swing_heavy` | P9 `SWSH_SWING IMPACTS Quick Heavy Weapon Swing To Thud Impact Var 01_DDUMAIS_MWP2.wav`; P7 `SWSH_Swing 3 Large 03_DDUMAIS_NONE.wav` |
| `mace_swing_heavy` | same two as hammer, pitch-shifted so they read apart |
| `armor_swing_heavy` | P9 `METLMisc_Metal, Slow Whoosh, Rattle, Pass By x4 01_344 Audio_Elemental Palette Designed Vol 1.wav` |
| `whoosh_blade_spin` | P9 `DSGNStngr_Action Deploy Units Sword Slice Special Move Layered Swish 04_ESM_TDG.wav`; P7 `SWSH_Sword Slash Impact V2 Assorted 18_DDUMAIS_NONE.wav` |
| `impact_flesh_light` | P9 `FGHTImpt_4 x Punch, Body 02_344 Audio_Cinematic Fight Vol 1.wav`; P8 `WEAPAxe_Long Two-Handed Axe Flesh Hit_JSE_MW.wav` |
| `impact_armor_heavy` | P9 `METLImpt_METAL SWING HIT Weapon Swing To Metallic Body Impact And Resonant Tail 01_DDUMAIS_MWP2.wav`; P8 `WEAPArmr_Metal Shield Block Hits_JSE_MW.wav` |
| `impact_bulldoze_heavy` | P7 `Impact 021.wav` (Epic Impacts Vol. 1); P8 `Bluezone_BC0294_modern_cinematic_impact_022.wav` |
| `impact_ground_slam` | P8 `Bluezone_BC0294_modern_cinematic_impact_boom_003.wav`; P9 `Impact Cut Sweep.wav` (Colossal Impacts); P7 `Impact 038.wav` |
| `death_grunt_male` | P9 `VOXReac_Construction Kit Male Flutter Death Vocal Stuttered Long 05_ESM_HC4.wav`; P7 `VOXScrm_Male in Shock 4_344 Audio_Screaming.wav` |
| `death_grunt_heavy` | P7 `Heavy Body Crunch 02.wav` (Flesh, Bones & Gore) under the grunt above; P9 `CREAHmn_Violent Humanoid Creature Exhale Short 4_SNDBTS_VB-SE.wav` |

### Ranged — GAP: no bow in any bundle

| Slot | Candidates |
|---|---|
| `bow_release_arrow` | nearest: P9 `WEAPWhip_WHIP Snap Crack 05_DDUMAIS_MWP2.wav` (pitch down); P7 `SWSH_Woodstick Swish 03_JSE_TW1.wav` |
| `arrow_volley_whoosh` | P9 `WINDDsgn_Wind, Rush, Whoosh, Long x5 01_344 Audio_Elemental Palette Designed Vol 1.wav`; P7 `W_a_P_Epic_Whoosh_8.wav` |

### Ooze

| Slot | Candidates |
|---|---|
| `ooze_splat_wet` | P8 `GORESplt_Gore Splatter 01_JSE_GMP.wav`; P9 `WOODImpt_Hit Blood Spill Splat Wood Impact Light Hit Squelch Small Thump 03_ESM_TDG.wav` |
| `impact_flesh_wet` | P8 `GOREFlsh_Flesh Drops on Floor 03_JSE_GMP.wav`; P9 `GORESplt_Gore Designed Transient Heavy Impact Smash 01_ESM_HALG.wav` |
| `death_ooze_dissolve` | P9 `CREAAqua_Aquatic Creature Gurgling 2_SNDBTS_VB-SE.wav`; P9 `WATRImpt_Impact Water Deep Submerge Bubble Drown Ship Hit 05_ESM_EMWI.wav` |

### Undead

| Slot | Candidates |
|---|---|
| `bone_swing_light` | P7 `SWSH_Woodstick Swish 03_JSE_TW1.wav`; P8 `WOODMvmt_FoleyMovement01_InMotionAudio_Wood.wav` |
| `impact_bone_crack` | P8 `GOREBone_Bone Breaks Celery 01_JSE_GMP.wav`; P8 `WOODBrk_Snap09_InMotionAudio_Wood.wav` |
| `impact_bone_crack_heavy` | P7 `Heavy Body Crunch 02.wav`; P8 `Bluezone_BC0297_stone_impact_015.wav` |
| `death_undead_collapse` | P7 `CREAHmn_Test Subject 2 12_344 Audio_Zombie Specimens Vol 2.wav` + P8 `WOODBrk_Snap09_InMotionAudio_Wood.wav` tail; P9 `HMNBrth_Construction Kit Male Screeching Breath Inhale Weak Squeal 05_ESM_HC4.wav` |
| `magic_staff_cast` | P9 `magic, action gesture, evil presence, onslaught-004.wav` (Emotion and Magic); P9 `FIREWhsh_Whoosh Fire Deep Growl Monster Saturated Crisp 03_ESM_EMWI.wav`; P7 `W_a_P_Spell_Whoosh_19.wav` |
| `horse_charge_impact` | P7 `FEETHors_Draft Horse Trot On Concrete 03 STEREO_DRCA_HOCA_UsiPro.wav` + P7 `Impact 045.wav` |
| `death_horse_collapse` | GAP (no horse death anywhere): P8 `ANMLHors_Horse, Quarter Horse, Mare, Snort_Uberduo_HORS_Mono-002.wav` over P7 `Heavy Body Crunch 02.wav` |
| `death_armor_collapse` | P8 `METLCrsh_Drop Fall Metal Rattle Scrap Debris_06_MWSFX_TM.wav`; P9 `WEAPArmr_Metal Shield Spin On Floor, Buckler MKH_344 Audio_Medieval Weapons Vol 2.wav`; P8 `Toolbox - DROP.wav` |

### Hero

| Slot | Candidates |
|---|---|
| `hero_hurt_grunt` | P9 `HMNBrth_Panting Male 02 04_SNDBTS_VH.wav` (cut one breath); P7 `VOXScrm_Male in Shock 4_344 Audio_Screaming.wav` |
| `hero_low_hp_warn` | P9 `Interface Deny Low Fat Dark.wav`; P7 `CCTS - 099 - R Clock.wav`; P9 `DSGNBass_Tone Downer (Reverb)_344 Audio_Bass Drops and Downers Vol 3.wav` |
| `stinger_victory` | P7 `Discovery_01-03_Organ-A_WET.wav` (RPG Orchestral Essentials); P8 `80,TheGong.wav`; P9 `Game Entry Happy Short.wav` |
| `stinger_defeat` | P7 `Failure_01-10_Ensemble-Small_DRY.wav`; P7 `Failure_05-01_Piano-A_DRY.wav`; P9 `DSGNBass_Bass Drop & Downer Slow 10_344 Audio_Bass Drops & Downers.wav` |
| `ui_relic_chime` | P9 `MAGAngl_Magic Light Spell Enchantment Potion Effect Tonal Bright 03_ESM_FG2.wav`; P7 `Enchanting_Bells_4.wav` |

### Spells — low priority (`spells.json` cues have no call site in the Godot build)

| Slot | Candidates |
|---|---|
| `magic_bolt_cast` | P7 `MagicPack2_ElectricitySpells_Spell2-002.wav` |
| `magic_heal_chime` | P7 `Harp_Glissando_10b.wav`; P9 `UIMisc_Kalimba 3 Up_CB Sounddesign_APPlicable Sounds.wav` |
| `magic_wall_cast` | P7 `MagicPack2_Earth_MeteorShower003.wav`; P7 `DESTRClpse_Massive Wall Collapsing_JSE_SD.wav` |

## New slots the build needs (not in `sfx.md` yet)

### Tank — `Battle.gd` gatling (`bow_release_arrow.wav` stand-in) and cannon (`impact_ground_slam.wav` stand-in)

| Cue | Candidates |
|---|---|
| cannon fire | P8 `Bluezone_BC0296_steampunk_weapon_cannon_shot_013_02.wav`; P7 `GS broadside 001.wav` (Naval Warfare); P7 `Potato Gun Shot Apples Heavy Blast AB.wav`; P8 `Haubits FH77 Howitzer - FIRING - Multiple Shots 2 - FRONT - MKH8060.wav` |
| shell explosion | P7 `Bluezone_BC0277_explosion_mortar_002_01.wav`; P8 `EXPLReal_Medium Realistic Explosion 15_DDUMAIS_NONE.wav`; P7 `GS cannonball impact 005.wav` |
| gatling shot (12/s pooled, pitch-varied — no true loop in the bundles) | P8 `Bluezone_BC0296_steampunk_weapon_gun_shot_026_02.wav`; P8 `Ak 5 - FIRING - Hit Metal Armor Plate - Single Shots 2 - HANDHELD NEAR SHOOTER - MS - 418-S.wav` |
| turret traverse | P8 `Bluezone_BC0296_steampunk_weapon_mechanism_texture_007.wav`; P9 `METLFric_Large Metal Box, Drag, Geofon_344 Audio_Extreme Winds Vol 1.wav` |
| cannon ready click | P9 `MECHLtch_Click Deep Mechanism Latch Button Nearfield Thunk 02_ESM_HDLM.wav`; P7 `CHAINImpt_InsJ_Chains_Metal_Dropping_Close_01-03.wav` |
| shake / rubble tail | P7 `Bluezone_BC0275_building_collapse_debris_falling_rock_rubble_008.wav`; P7 `DIRTCrsh_Stone Sand Trickle Reverberant 01_JSE_SD.wav` |

### World

| Cue | Candidates |
|---|---|
| gate open (`gate_open.png`, silent) | P7 `DOORMetl_Big Metal Gate Open Close 03_JSE_MM_Mono.wav`; P7 `DOORHdwr_Big Metal Gate Pushbar Squeak 02_JSE_MM_Stereo.wav`; P8 `DOORGate_Wooden Metal Hinge Creaks_Jake Fielding_Squeaky Gates.wav` |
| fire crackle loop (braziers, cauldron, hero orb) | P9 `FIRECrkl_Fire Crackling, Popping, Witch's Cauldron_344 Audio_Haunting Ambiences Vol 5.wav`; P8 `FIREMisc_Fire Crackling In A Woodstove_UberDuo_WOOD.wav`; P9 `24 Campfire, Dropping Fresh Pine Branches in Fire, Crackling, Sizzling Strong, Close 02.wav` |
| hall ambience bed | P8 `AMBUndr_CaveDesign01_InMotionAudio_CaveDesign.wav` + P8 `WATRDrip_SingleDrip03_InMotionAudio_CaveDesign.wav`; P9 `WINDInt_ChimneyWind05_InMotionAudio_ChimneyWind.wav`; P9 `AMBDsgn_Evil Spell Ambience_344 Audio_Ghostly Presences.wav` |
| muffled-war intro bed (`docs/design/intro-and-zones.md`) | P7 `Monster Ambience, Distant Destruction, Huge Groans.wav`; P7 `Super Low Rumbles, Apocalypse, Underwater, Groans.wav` (Haunting Ambiences Vol. 2) |
| necromancer hall hum | P9 `magic, drone, tension, spellbound, evanescence-002.wav`; P9 `DSGNSynth_Dark Loop Mystic Forest Tonal Steady Synth_ESM_SGA3.wav`; P7 `BELLMisc_Chimes, Atmos, Dark Magic, Unsetting, Magical 01_344 Audio_Cymbals From Hell Vol 3.wav` |
| hall-turn / stairs beat | P9 `Transition Braam Slow Dark Creepy.wav`; P7 `DSGNBoom_Gong, Drone, Low End 25_344 Audio_Cymbals From Hell Vol 2.wav` |
| enemy charge horn (`docs/design/battleground.md`) | P7 `23 BELLMEN_Traditional horns.wav`; P9 `DSGNBram____Cinematic Horn Braam, Epic, Cinematic, Dark, Instrument, Huge-32.wav` |
| castle bell (optional colour) | P9 `04 Church Bells, Near Distance, In Church Tower-3 Different Bell 02.wav` |

### UI

| Cue | Candidates |
|---|---|
| click / select | P9 `UIClick_UI Button Analog Vintage Double Click Neutral Dry Press 11_ESM_BG.wav`; P8 `UIClick_Select Middle 29_RSCPC_USIN.wav` |
| confirm (army swap Q/E, Space-to-begin) | P9 `Interface Accept Glassy Snap.wav`; P8 `UIAlert_Confirm Middle 12_RSCPC_USIN.wav` |
| deny | P9 `Deny Muted.wav` |
| toast appear | P9 `Woosh Sweep Slide Infographics Basic.wav`; P8 `UIMvmt_Window Open Thin 05_RSCPC_USIN.wav` |
| upgrade stingers ×4 (gatling rate / cannon radius / cannon reload / tank HP) | P9 `Interface Arp Reveal Down Long.wav`, `Button Arp Twinkle.wav`, `Interface Plucks Happy.wav`, `Ting Coins.wav` |

### Enemy identity — five types currently share the generic undead files

| Type | Candidates |
|---|---|
| ghoul | P9 `CREABeast_Creature Werewolf Growl Menacing Monstrous 06_ESM_HALG.wav`; P7 `MonsterPack2_Monster06_Attack13.wav` |
| wraith | P9 `CREAEthr_Ethereal Entity Grim Pain Long 4_SNDBTS_VB-SE.wav`; P9 `DSGNEthr_Jumpscare Vocal Aggressive Whisper Harsh Distortion 02_ESM_HALG.wav` |
| bone_knight | hit P8 `WEAPArmr_Metal Shield Block Hits_JSE_MW.wav`; death P9 `WEAPArmr_Metal Shield Spin On Floor, Buckler MKH_344 Audio_Medieval Weapons Vol 2.wav` |
| plague_priest | P7 `CREAHmn_Test Subject 3 10_344 Audio_Zombie Specimens Vol 2.wav`; P9 `HMNBrth_Respirator, Specimen Breathing, Mad Scientist Lab_344 Audio_Haunting Ambiences Vol 5.wav` |
| necromancer | roar P7 `MonsterPack2_LargeMonster11_Roar03.wav`; telegraph P7 `DSGNStngr_Gong, Distorted, Riser, Stinger 06_344 Audio_Cymbals From Hell Vol 2.wav`; cast P9 `magic, action gesture, evil presence, onslaught-004.wav` |

### Music — GAP

Bundles are SFX. Only loopable beds: P9 `DSGNSynth_Dark Loop Mystic Forest Tonal Steady Synth_ESM_SGA3.wav`
and the P9 Emotion and Magic drones. Use one as the "1 music loop" stand-in or source music elsewhere.

## Skip entirely

Vehicles / cars / motorbikes / aircraft / trains, sports ambiences, crowds and walla, urban and
public spaces, sci-fi UI and droids, barbershop / kitchen / household foley, Christmas, casino,
dinosaurs, police radio, anime voices, animal packs except horse. That is most of every bundle.
