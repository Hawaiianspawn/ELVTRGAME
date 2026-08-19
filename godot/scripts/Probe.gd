extends Node
## Evidence tool. `godot --path godot -- --probe=battle,8` loads a scene, waits N seconds,
## prints FPS and saves user://probe_<scene>.png. Inert without the arg.

func _ready() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--probe="):
			_run(a.trim_prefix("--probe="))


func _run(spec: String) -> void:
	var parts := spec.split(",")
	var scene := parts[0]
	var secs := float(parts[1]) if parts.size() > 1 else 5.0
	Game.charged = parts.size() > 2 and parts[2] == "charged"
	await get_tree().process_frame
	get_tree().change_scene_to_file(Game.SCENES[scene])
	await get_tree().create_timer(secs).timeout
	print("PROBE %s fps=%d" % [scene, Engine.get_frames_per_second()])
	var img := get_viewport().get_texture().get_image()
	var path := "user://probe_%s.png" % scene
	img.save_png(path)
	print("PROBE saved ", ProjectSettings.globalize_path(path))
	get_tree().quit()
