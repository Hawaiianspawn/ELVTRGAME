# stresswar_level.py -- pin AStressWarGameMode on /Game/StressWar/L_StressWar and save.
#
# The level itself is made BY HAND: in the editor, right-click Content/Spike1/L_Spike1 ->
# Duplicate, move it to Content/StressWar/L_StressWar. (Doing the duplicate + load from one
# Python pass hit the EditorServer "World Memory Leaks" fatal on 2026-08-18; don't.)
#
# Run from the editor console:  py "C:/Projects/ELVTRGAME/Scripts/stresswar_level.py"
import unreal

LEVEL = "/Game/StressWar/L_StressWar"
GAMEMODE = "/Script/ELVTR.StressWarGameMode"

if not unreal.EditorAssetLibrary.does_asset_exist(LEVEL):
    raise SystemExit("StressWar: %s missing -- duplicate L_Spike1 there in the editor first" % LEVEL)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.load_level(LEVEL):
    raise SystemExit("StressWar: load_level failed")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm = unreal.load_class(None, GAMEMODE)
world.get_world_settings().set_editor_property("default_game_mode", gm)
print("StressWar: %s game mode -> %s, saved=%s" % (world.get_path_name(), gm.get_name(), les.save_current_level()))
