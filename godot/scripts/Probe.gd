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
	await get_tree().process_frame
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)   # uncapped: fps = real headroom
	Engine.max_fps = 0
	get_viewport().set_disable_input(true)   # never eat the owner's keystrokes
	get_tree().change_scene_to_file(Game.SCENES[scene])
	var post_preset := -1
	if parts.size() > 2 and parts[2].begins_with("post="):
		post_preset = int(parts[2].trim_prefix("post="))
		await get_tree().create_timer(1.0).timeout
		get_tree().current_scene.post.set_preset(post_preset)
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
	if parts.size() > 2 and parts[2] == "turn":
		# self-check: the between-wave lens moves. "left"/"right" slide the camera and come back,
		# "stairs" pitches it up to a horizon of 170 and back — all of it must land back at rest.
		await get_tree().create_timer(2.0).timeout
		var b := get_tree().current_scene
		var cam: Camera3D = b.camera
		var rest := cam.rotation_degrees.x
		for kind in ["right", "stairs"]:
			var moved := 0.0
			b._turn(kind)                       # 2 x 1.2s of tween; sample right through it
			for _i in range(32):
				await get_tree().create_timer(0.1).timeout
				moved = maxf(moved, absf(cam.position.x) + absf(cam.rotation_degrees.x - rest))
			assert(moved > 5.0, "%s never moved the lens" % kind)   # right slides 160 units, stairs pitches 12 degrees
			assert(absf(cam.position.x) < 0.5 and absf(cam.rotation_degrees.x - rest) < 0.5, "%s left the lens off its rest pose" % kind)
			print("PROBE turn ok: %s peaked at %.0f, back to x=%.1f pitch=%.2f" % [kind, moved, cam.position.x, cam.rotation_degrees.x])
	if parts.size() > 2 and parts[2] == "whirl":
		# self-check: swapping back to veterans fires whirl â€” the field spin-dodges under guard
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		b._cycle_army(1)
		await get_tree().create_timer(4.0).timeout
		b._cycle_army(-1)
		# whirl pulses 1s on / 1s off: wait until a burst is live, then check the i-frames
		var whirling := 0
		for _i in range(60):
			await get_tree().create_timer(0.1).timeout
			whirling = 0
			for u in b.units:
				if u.team == 0 and u.type == "veteran" and u.spinning():
					whirling += 1
			if whirling > 0:
				break
		assert(whirling > 0, "no veterans spinning after swap-in")
		for u in b.units:
			if u.team == 0 and u.type == "veteran" and u.spinning():
				var hp0: float = u.hp
				u.take(999.0)
				assert(u.hp == hp0 and not u.dead, "spinning veteran took damage â€” no i-frames")
		var vortexes: int = b.world.get_children().filter(func(c): return c.has_meta("vortex")).size()
		assert(vortexes >= b.VORTEX_COLS * b.VORTEX_ROWS, "whirl spread missing: %d vortex cleaves" % vortexes)
		print("PROBE whirl ok: %d veterans in spin burst, %d vortex cleaves live" % [whirling, vortexes])
	if parts.size() > 2 and parts[2] == "charge":
		# self-check: swap to halberdiers â€” the field charges down-range, then every one is back home
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		b._cycle_army(1)   # veteran -> halberdier
		var out := 0
		for _i in range(80):
			await get_tree().create_timer(0.1).timeout
			out = b.units.filter(func(u): return u.team == 0 and u.type == "halberdier" and u.charge_to > 0.0).size()
			if out > 0:
				break
		assert(out > 0, "no halberdier charging after swap-in")
		await get_tree().create_timer(6.0).timeout
		var stuck := []
		for u in b.units:
			if u.team == 0 and u.type == "halberdier" and not u.dead and (u.charge_to > 0.0 or u.wd > b.FRONT_D + 30.0):
				stuck.append("%s wd=%.0f charge_to=%.0f from=%s moving=%s" % [Unit.State.keys()[u.state], u.wd, u.charge_to, u._charge_from, u._moving])
		for l in stuck.slice(0, 5):
			print("PROBE charge STUCK " + l)
		assert(stuck.is_empty(), "%d halberdiers never came home" % stuck.size())
		print("PROBE charge ok: %d charged, all home" % out)
	if parts.size() > 2 and parts[2] == "volley":
		# self-check: loft a few enemies, swap to vet_ranged â€” the flurry must fire; count
		# how many lofted enemies an arrow ran through (informational, timing-dependent)
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		var lofted := []
		for o in b.units:
			if o.team == 1 and o.wd < 700.0:
				o.launch(300.0)
				lofted.append(o)
				if lofted.size() == 6:
					break
		b._cycle_army(-1)   # veteran -> vet_ranged
		await get_tree().create_timer(2.5).timeout
		assert(b._ability_cd_left("vet_ranged") > 0.0, "volley did not fire")
		var clipped := 0
		for o in lofted:
			if not is_instance_valid(o) or o.dead or o.hp < o.max_hp:
				clipped += 1
		print("PROBE volley ok: fired, %d/%d lofted enemies clipped" % [clipped, lofted.size()])
	if parts.size() > 2 and parts[2] == "hammer":
		# self-check: swapping to hammers drops them from the sky; every one lands and slams
		await get_tree().create_timer(6.0).timeout
		var b := get_tree().current_scene
		while b.army_type != "hammer":
			b._cycle_army(1)
			await get_tree().create_timer(0.1).timeout
		var falling := 0
		for u in b.units:
			if u.team == 0 and u.type == "hammer" and u.air_h > 0.0 and u.sky_slam:
				falling += 1
		assert(falling > 0, "no hammers falling from the sky")
		await get_tree().create_timer(4.0).timeout
		var stuck := 0
		for u in b.units:
			if u.team == 0 and u.sky_slam:
				stuck += 1
		assert(stuck == 0, "hammers never landed/slammed: %d" % stuck)
		print("PROBE hammer ok: %d dropped from sky, all landed" % falling)
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
	if parts.size() > 2 and parts[2].begins_with("horde"):
		# horde: dump N enemies (default 500) into the hall at once, then fight; report FPS + sim ms
		var n := int(parts[2].trim_prefix("horde")) if parts[2].length() > 5 else 500
		await get_tree().create_timer(1.0).timeout
		var b := get_tree().current_scene
		b.spawn_queue.clear()
		for i in range(n):
			b._spawn_enemy(["undead", "mace_undead", "ooze", "archer_undead"][i % 4])
			var u: Unit = b.units[-1]
			u.wd = b.FRONT_D + 60.0 + (i / 8) * 22.0
			u.hp *= 50.0          # tanky: the count must hold for the measurement
			u.max_hp = u.hp
		b._done = true        # no wave reset when the hero falls: units still tick, measurement holds
		print("PROBE horde spawned=%d units=%d" % [n, b.units.size()])
	if parts.size() > 2 and parts[2] == "jump":
		# self-check: every jumper phase loads and runs a second without errors
		for i in range(Jump.PHASES.size()):
			await Jump._go(i)
			await get_tree().create_timer(1.0).timeout
			print("PROBE jump %d -> %s wave=%d" % [i + 1, get_tree().current_scene.scene_file_path.get_file(), Game.wave])
	await get_tree().create_timer(secs).timeout
	var sc0 := get_tree().current_scene
	var alive: int = sc0.units.size() if "units" in sc0 else 0
	print("PROBE %s fps=%d process_ms=%.1f physics_ms=%.1f units=%d objects=%d" % [scene, Engine.get_frames_per_second(),
		Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0, Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS) * 1000.0,
		alive, Performance.get_monitor(Performance.OBJECT_NODE_COUNT)])
	var sc := get_tree().current_scene
	if "units" in sc:
		var wds := []
		for u in sc.units:
			if u.team == 0 and u.state == Unit.State.RANK:
				wds.append(snappedf(u.wd, 1.0))
		wds.sort()
		var cam: Camera3D = sc.camera
		print("PROBE rank wd min=%s max=%s n=%d  screen y of min=%s fov=%.1f pitch=%.1f height=%s" % [wds[0], wds[-1], wds.size(),
			Hall3D.unproject(cam, 0.0, wds[0]).y, cam.fov, cam.rotation_degrees.x, cam.position.y])
		for u in sc.units:
			if u.team == 0 and u.state == Unit.State.RANK and (snappedf(u.wd, 1.0) == wds[0] or snappedf(u.wd, 1.0) == wds[-1]):
				print("PROBE unit wd=%s pos=%s spr=%s screen=%s" % [u.wd, u.position, u.sprite.position, Hall3D.unproject(cam, u.wx, u.wd)])
	var img := get_viewport().get_texture().get_image()
	var path := "user://probe_%s_post%d.png" % [scene, post_preset] if post_preset >= 0 else "user://probe_%s.png" % scene
	img.save_png(path)
	print("PROBE saved ", ProjectSettings.globalize_path(path))
	get_tree().quit()
