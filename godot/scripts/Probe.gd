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
	if parts.size() > 2 and parts[2] == "swap":
		# self-check: swapping lane 2's type sends its front unit home and marches the new type up
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		var old = b.front[2]
		b._cycle_lane(1, 2)
		assert(old.state == Unit.State.RETREAT, "front unit did not retreat")
		await get_tree().create_timer(4.0).timeout
		var new = b.front[2]
		assert(new != null and new != old and new.type == b.lane_types[2], "new type did not step up")
		assert(old.state == Unit.State.RANK or not is_instance_valid(old), "old unit never reached its rank slot")
		print("PROBE swap ok: %s -> %s" % [old.type, new.type])
	if parts.size() > 2 and parts[2] == "jump":
		# self-check: every jumper phase loads and runs a second without errors
		for i in range(Jump.PHASES.size()):
			await Jump._go(i)
			await get_tree().create_timer(1.0).timeout
			print("PROBE jump %d -> %s wave=%d" % [i + 1, get_tree().current_scene.scene_file_path.get_file(), Game.wave])
	await get_tree().create_timer(secs).timeout
	print("PROBE %s fps=%d" % [scene, Engine.get_frames_per_second()])
	var img := get_viewport().get_texture().get_image()
	var path := "user://probe_%s.png" % scene
	img.save_png(path)
	print("PROBE saved ", ProjectSettings.globalize_path(path))
	get_tree().quit()
