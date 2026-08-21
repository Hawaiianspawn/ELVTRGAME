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
	if parts.size() > 2 and parts[2] == "whirl":
		# self-check: swapping back to veterans fires whirl — the field spin-dodges under guard
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
				assert(u.hp == hp0 and not u.dead, "spinning veteran took damage — no i-frames")
		var vortexes: int = b.world.get_children().filter(func(c): return c.has_meta("vortex")).size()
		assert(vortexes >= b.VORTEX_COLS * b.VORTEX_ROWS, "whirl spread missing: %d vortex cleaves" % vortexes)
		print("PROBE whirl ok: %d veterans in spin burst, %d vortex cleaves live" % [whirling, vortexes])
	if parts.size() > 2 and parts[2] == "charge":
		# self-check: swap to halberdiers — the field charges down-range, then every one is back home
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
		# self-check: loft a few enemies, swap to vet_ranged — the flurry must fire; count
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
		print("PROBE rank wd min=%s max=%s n=%d  screen y of min=%s cam_h=%s horizon=%s focal=%s" % [wds[0], wds[-1], wds.size(), sc.view.project(0, wds[0]).y, sc.view.cam_h, sc.view.horizon, sc.view.focal])
		for u in sc.units:
			if u.team == 0 and u.state == Unit.State.RANK and (snappedf(u.wd, 1.0) == wds[0] or snappedf(u.wd, 1.0) == wds[-1]):
				print("PROBE unit wd=%s pos=%s scale=%s spr=%s" % [u.wd, u.position, u.scale, u.sprite.position])
	var img := get_viewport().get_texture().get_image()
	var path := "user://probe_%s.png" % scene
	img.save_png(path)
	print("PROBE saved ", ProjectSettings.globalize_path(path))
	get_tree().quit()
