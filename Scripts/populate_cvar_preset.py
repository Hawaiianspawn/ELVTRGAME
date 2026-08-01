# populate_cvar_preset.py
#
# Builds ELVTR's Console Variables Editor presets so you can LOAD one and see only
# the dials for what you're testing (Presets menu in the Console Variables Editor).
#
# Presets created/refreshed (assets under /Game/SwarmControls):
#   SwarmConsoleVariablesAsset  — everything (all tuning CVars)
#   CVP_Lighting                — flame + dither + unit shading (the look)
#   CVP_Combat                  — HP/DPS/melee + spawn distance + hit reaction (feel)
# These mirror the canonical groups in the cvars skill; edit VALUES/PRESETS below to
# change the set, the baked values, or to add a scenario preset with custom values.
#
# Run from the Unreal editor console (~ then type):
#     py "C:/Projects/ELVTRGAME/Scripts/populate_cvar_preset.py"
# or Tools > Execute Python Script... and pick this file.
#
# Uses UConsoleVariablesEditorFunctionLibrary (a BlueprintCallable API the MCP sandbox
# cannot reach but editor Python can). Each preset is rebuilt to EXACTLY its set via
# load -> remove-all -> add -> copy, using RemoveCommandFromCurrentPreset.

import unreal

FOLDER = "/Game/SwarmControls"
BASE_ASSET = FOLDER + "/SwarmConsoleVariablesAsset"  # the "everything" preset + duplication seed

# One place for each tuning CVar's baseline value (the source defaults). A preset is
# just a list of names; values are pulled from here so they stay consistent across
# presets. For a scenario preset with different values, bake a "Name Value" list directly.
VALUES = {
    # Lighting — the bearer's flame spotlight
    "Swarm.Flame": "1",
    "Swarm.FlameRadius": "900",
    "Swarm.FlameCoreRadius": "330",
    "Swarm.FlameFalloff": "2.0",
    "Swarm.FlameIntensity": "0.55",
    "Swarm.FlameFlicker": "0.06",
    "Swarm.FlameStiffness": "55",
    "Swarm.FlameDamping": "0.6",
    # Dither  (WorldDitherScale/BandWidth are owner-tuned, not the src defaults 12/0.326)
    "Swarm.DitherWorldAnchor": "1",
    "Swarm.WorldDitherScale": "8",
    "Swarm.DitherBandWidth": "0.5",
    # Per-unit shading
    "Swarm.UnitLightFloor": "0.28",
    # Brood visibility — the additive floor that makes the tide legible, and the near-camera
    # band that dissolves it back to authored art up close (task-084/089)
    "Swarm.BroodAdd": "0.05",
    "Swarm.RawNear": "420",
    # Enemy / combat
    "Swarm.BroodMaxHP": "60",
    "Swarm.BroodDPS": "35",
    "Swarm.RetinueMaxHP": "130",
    "Swarm.RetinueDPS": "30",
    "Swarm.MeleeRange": "95",
    "Swarm.MaxAttackersPerUnit": "4",   # safety clamp only; no longer a rate limiter
    # Cleave, per team. Retinue swing at everyone adjacent; brood commit to one target.
    "Swarm.RetinueTargetsPerHit": "8",
    "Swarm.BroodTargetsPerHit": "1",
    "Swarm.HeroMaxHP": "500",
    "Swarm.HeroDPS": "55",
    "Swarm.HeroMeleeRange": "190",
    # Hit reaction — swing cadence / flash / knockback (GATE1 §3b)
    "Swarm.SwingInterval": "0.9",
    "Swarm.SwingStrikeAt": "0.35",
    "Swarm.SwingLunge": "12",
    "Swarm.HitFlashTime": "0.1",
    "Swarm.KnockbackDistance": "35",
    "Swarm.KnockbackTime": "0.1",
    # Body size — per team, overriding the render actor's UPROPERTYs (added 2026-07-26)
    # Per-unit variation. Rides in the spare high bits of the render buffer's anim int32,
    # so the amplitude retunes live with no respawn. Niagara ignores it; boxes + cam only.
    "Swarm.BroodSizeJitter": "0.2",
    "Swarm.RetinueSizeJitter": "0",
    # Horde movement — how the tide crosses the dark (added 2026-07-26)
    "Swarm.BroodSpeed": "320",
    "Swarm.BroodAggroRange": "600",
    "Swarm.BroodSeparation": "60",
    "Swarm.BroodSeparationWeight": "1.4",
    "Swarm.BroodSeparationCap": "6",
    "Swarm.BroodWalkHz": "6",
    # Horde arrival — where the tide comes from and how ragged it lands
    "Swarm.BroodSpawnRadiusMin": "2500",
    "Swarm.BroodSpawnArc": "360",
    "Swarm.BroodSpawnArcCenter": "0",
    "Swarm.BroodSpeedJitter": "0.15",
    # Game camera — slides to keep the hero centred in the strip the HUD doesn't cover
    "Kindled.Cam.HudBias": "1",
    "Kindled.Cam.HudBiasLerp": "6",
}

LIGHTING = [k for k in VALUES if k.startswith("Swarm.Flame")
            or k.startswith("Swarm.Dither") or k == "Swarm.WorldDitherScale"
            or k.startswith("Swarm.Unit")
            or k in ("Swarm.BroodAdd", "Swarm.RawNear")]
COMBAT = ["Swarm.BroodMaxHP", "Swarm.BroodDPS", "Swarm.RetinueMaxHP", "Swarm.RetinueDPS",
          "Swarm.MeleeRange", "Swarm.MaxAttackersPerUnit", "Swarm.HeroMaxHP", "Swarm.HeroDPS",
          "Swarm.HeroMeleeRange", "Swarm.BroodSpawnRadiusMin", "Swarm.RetinueTargetsPerHit", "Swarm.BroodTargetsPerHit"]
# Hit reaction rides on the Combat preset: you tune how a blow FEELS in the same sitting
# as how much it hurts, and SwingInterval is the seam between the two.
HITREACTION = ["Swarm.SwingInterval", "Swarm.SwingStrikeAt", "Swarm.SwingLunge",
               "Swarm.HitFlashTime", "Swarm.KnockbackDistance", "Swarm.KnockbackTime"]
COMBAT = COMBAT + HITREACTION
# The horde is now its own sitting: everything about how the tide moves, packs, targets
# and arrives, plus the brood stat block it is judged by. Deliberately overlaps COMBAT on
# the brood stats — you cannot tell whether BroodSpeed is right without BroodDPS in reach.
HORDE = ["Swarm.BroodMaxHP", "Swarm.BroodDPS", "Swarm.BroodTargetsPerHit",
         "Swarm.BroodSizeJitter", "Swarm.RetinueSizeJitter",
         "Swarm.BroodSpeed", "Swarm.BroodAggroRange", "Swarm.BroodSeparation",
         "Swarm.BroodSeparationWeight", "Swarm.BroodSeparationCap", "Swarm.BroodWalkHz",
         "Swarm.BroodSpawnRadiusMin", "Swarm.BroodSpawnArc",
         "Swarm.BroodSpawnArcCenter", "Swarm.BroodSpeedJitter"]
EVERYTHING = list(VALUES.keys())

# Preset asset name -> ordered list of CVar names (values pulled from VALUES).
PRESETS = {
    "SwarmConsoleVariablesAsset": EVERYTHING,
    "CVP_Lighting": LIGHTING,
    "CVP_Combat": COMBAT,
    "CVP_Horde": HORDE,
}

lib = unreal.ConsoleVariablesEditorFunctionLibrary
eal = unreal.EditorAssetLibrary
REPLACE = unreal.ConsoleVariablesEditorPresetImportMode.REPLACE_EXISTING


def names_in(asset):
    """Command names saved in an asset (the out-param comes back as a tuple in Python)."""
    res = lib.get_list_of_commands_from_preset(asset)
    return list(res[1]) if isinstance(res, tuple) else list(res)


def get_or_create(name):
    path = FOLDER + "/" + name
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    # Prefer creating an EMPTY asset so a subset preset can never inherit other groups'
    # rows. Falls back to duplicating the base seed (which the clear step below empties).
    try:
        at = unreal.AssetToolsHelpers.get_asset_tools()
        created = at.create_asset(name, FOLDER, unreal.ConsoleVariablesAsset, None)
        if created is not None:
            return created
    except Exception as e:
        unreal.log_warning("CVE: create_asset failed for %s (%s); duplicating base instead" % (name, e))
    if not eal.does_asset_exist(BASE_ASSET):
        unreal.log_warning("CVE: base seed asset missing, create it once by hand: " + BASE_ASSET)
        return None
    return eal.duplicate_asset(BASE_ASSET, path)  # cleared below, so its rows don't matter


for name, cvars in PRESETS.items():
    asset = get_or_create(name)
    if asset is None:
        unreal.log_warning("CVE: could not get/create preset '%s'" % name)
        continue

    # Make it the active list, then strip it to empty so the result is EXACTLY our set.
    lib.load_preset_into_console_variables_editor(asset, REPLACE)
    for existing in names_in(asset):
        lib.remove_command_from_current_preset(existing)

    added = 0
    for cv in cvars:
        if lib.add_validated_command_to_current_preset(cv + " " + VALUES[cv]):
            added += 1
        else:
            unreal.log_warning("CVE[%s]: failed to add '%s' (is the CVar registered?)" % (name, cv))

    lib.copy_current_list_to_asset(asset)
    eal.save_loaded_asset(asset)
    unreal.log("CVE preset '%s': added %d/%d, asset now holds %d rows: %s"
               % (name, added, len(cvars), len(names_in(asset)), names_in(asset)))

# Leave the editor on the "everything" preset rather than the last subset processed.
base = unreal.load_asset(BASE_ASSET)
if base:
    lib.load_preset_into_console_variables_editor(base, REPLACE)
unreal.log("CVE: done — presets: %s" % ", ".join(PRESETS.keys()))
