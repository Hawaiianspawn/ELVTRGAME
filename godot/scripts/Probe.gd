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
	get_viewport().set_disable_input(true)   # never eat the owner's keystrokes
	get_tree().change_scene_to_file(Game.SCENES[scene])
	if parts.size() > 2 and parts[2] == "swap":
		# self-check: swapping lane 2's type sends its front unit home and marches the new type up
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		var old = b.front[2]
		var old_type: String = b.army_type
		b._cycle_army(1)
		assert(old.state == Unit.State.RETREAT, "front unit did not retreat")
		await get_tree().create_timer(5.0).timeout
		var new = b.front[2]
		assert(new != null and new != old and new.type == b.army_type, "new type did not step up")
		assert(b._ability_cd_left(b.army_type) > 0.0, "swap-in ability did not fire")
		var stragglers := 0
		for u in b.units:
			if u.team == 0 and u.type == old_type and not u.dead:
				stragglers += 1
		assert(stragglers == 0, "old type still on the field: %d" % stragglers)
		print("PROBE swap ok: %s -> %s, %d on field, pool[%s]=%d" % [old_type, b.army_type, b.units.filter(func(x): return x.team == 0).size(), old_type, b.pool[old_type]])
	if parts.size() > 2 and parts[2].begins_with("stress"):
		# stress: grow the block to N rows (default 20) and report FPS after `secs`
		var rows := int(parts[2].trim_prefix("stress")) if parts[2].length() > 6 else 20
		await get_tree().create_timer(1.0).timeout
		var b := get_tree().current_scene
		var extra := Army.block(b, b.world, Game.waves[Game.wave]["reserves"], 700.0, rows, 285.0 - 7 * Army.RANK_STEP, b._rng)
		for u in extra:
			u.died.connect(b._on_died)
			b.units.append(u)
		print("PROBE stress units=%d" % b.units.size())
	if parts.size() > 2 and parts[2] == "jump":
		# self-check: every jumper phase loads and runs a second without errors
		for i in range(Jump.PHASES.size()):
			await Jump._go(i)
			await get_tree().create_timer(1.0).timeout
			print("PROBE jump %d -> %s wave=%d" % [i + 1, get_tree().current_scene.scene_file_path.get_file(), Game.wave])
	await get_tree().create_timer(secs).timeout
	print("PROBE %s fps=%d" % [scene, Engine.get_frames_per_second()])
	var sc := get_tree().current_scene
	if "units" in sc:
		var wds := []
		for u in sc.units:
			if u.team == 0 and u.state == Unit.State.RANK:
				wds.append(snappedf(u.wd, 1.0))
		wds.sort()
		print("PROBE rank wd min=%s max=%s n=%d  screen y of min=%s" % [wds[0], wds[-1], wds.size(), sc.view.project(0, wds[0]).y])
	var img := get_viewport().get_texture().get_image()
	var path := "user://probe_%s.png" % scene
	img.save_png(path)
	print("PROBE saved ", ProjectSettings.globalize_path(path))
	get_tree().quit()
