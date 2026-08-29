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
	# extra comma args (index 2+) compose: wave=N and post=N apply wherever they land, whatever's
	# left over (if anything) is the self-check keyword.
	var check := ""
	var post_preset := -1
	for e in parts.slice(2):
		if e.begins_with("wave="):
			Game.wave = int(e.trim_prefix("wave="))   # Battle._ready starts from Game.wave
		elif e.begins_with("post="):
			post_preset = int(e.trim_prefix("post="))
		else:
			check = e
	get_tree().change_scene_to_file(Game.SCENES[scene])
	if post_preset >= 0:
		await get_tree().create_timer(1.0).timeout
		get_tree().current_scene.post.set_preset(post_preset)
	if check == "swap":
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
	if check == "turn":
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
	if check == "whirl":
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
	if check == "charge":
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
	if check == "volley":
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
	if check == "hammer":
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
	if check == "tank":
		# self-check: siege is gone, and the gatling/cannon/chest weapons work. 7s (not the shared
		# tail's own later wait): enough for the rush to creep inside FOG_END and read on camera,
		# well before hall 1 clears (observed ~15s+ into a full run).
		await get_tree().create_timer(7.0).timeout
		var b := get_tree().current_scene
		assert(not Game.SCENES.has("siege"), "siege scene still registered")
		# (a) gatling hit on a grounded, unrooted enemy's screen point launches it. Rooted units
		# (piled by a halberdier charge / hammer slam) hold air_h at 0 for a beat regardless of
		# launch(), so they're excluded here -- that's a Unit.gd state, not a gatling bug. Picks the
		# oldest surviving candidate (first spawned, first in units[]): well onto the field and
		# rendered, not a freshly-spawned one still sitting at spawn depth with a degenerate rect.
		var now_s := Time.get_ticks_msec() / 1000.0
		var target: Unit = null
		for u in b.units:
			if u.team == Unit.ENEMY and not u.dead and u.air_h == 0.0 and u.rooted_until <= now_s:
				target = u
				break
		assert(target != null, "no grounded, unrooted enemy found for the gatling test")
		var p := Hall3D.unproject(b.camera, target.wx, target.wd)   # a real pixel on its own sprite
		b._gatling_hit_at(p)
		# capture right here: gate + wave-1 rush + a live tracer/spark on a real hit, at ~t=5s
		# before the hall clears (the generic end-of-run shot below lands well into hall 2)
		var shot_path := "user://probe_tank_shot.png"
		get_viewport().get_texture().get_image().save_png(shot_path)
		print("PROBE saved ", ProjectSettings.globalize_path(shot_path))
		for _i in range(3):
			await get_tree().process_frame
		assert(target.air_h > 0.0, "gatling hit did not launch the grounded target")
		print("PROBE tank gatling ok: launched %s air_h=%.1f" % [target.type, target.air_h])
		# (b) cannon blast at the densest grounded, unrooted cluster launches >= 3 of them
		var grounded: Array[Unit] = []
		for u in b.units:
			if u.team == Unit.ENEMY and not u.dead and u.air_h == 0.0 and u.rooted_until <= now_s:
				grounded.append(u)
		assert(grounded.size() >= 3, "not enough grounded, unrooted enemies to test the cannon on")
		var best: Unit = grounded[0]
		var best_n := -1
		for o in grounded:
			var n := 0
			for q in grounded:
				if Vector2(o.wx, o.wd).distance_to(Vector2(q.wx, q.wd)) < b.CANNON_RADIUS:
					n += 1
			if n > best_n:
				best_n = n
				best = o
		b._cannon_explode_at(Vector2(best.wx, best.wd))
		for _i in range(3):
			await get_tree().process_frame
		# CANNON_DMG (40) one-shots hall-1 fodder (ooze hp 30, undead hp 40) -- Battle.gd launches
		# before it damages, so the knock-up still reads even on a killed unit; air_v catches units
		# launched this same frame (air_h hasn't climbed off 0 yet).
		var affected := 0
		for u in grounded:
			if not is_instance_valid(u) or u.dead or u.air_h > 0.0 or u.air_v > 0.0:
				affected += 1
		assert(affected >= 3, "cannon blast only affected %d/%d clustered grounded enemies" % [affected, grounded.size()])
		print("PROBE tank cannon ok: %d/%d killed or launched" % [affected, grounded.size()])
		# (c) popping a chest applies the gatling rate upgrade (first in the cycle)
		var before: float = b.gatling_rate_mult
		b._spawn_chest(0.0, 300.0)
		var chest: Dictionary = b._up_chests[-1]
		b._gatling_hit_at(Hall3D.unproject(b.camera, chest["wx"], chest["d"]))
		assert(b.gatling_rate_mult > before, "chest pop did not apply the gatling rate upgrade")
		print("PROBE tank chest ok: gatling_rate_mult %.2f -> %.2f" % [before, b.gatling_rate_mult])
		print("PROBE tank ok: siege removed, gatling/cannon/chest all landed")
	if check == "walls":
		# self-check: hall half matches HALF_BY_WAVE for Game.wave, every live unit/prop stays
		# inside the walls, and the bend eases in to CURVE_A_BY_WAVE within the 2s tween-in.
		await get_tree().create_timer(3.0).timeout
		var b := get_tree().current_scene
		var half: float = b.HALF_BY_WAVE[Game.wave]
		assert(is_equal_approx(b.HALL_HALF, half), "HALL_HALF %.0f != table %.0f for wave %d" % [b.HALL_HALF, half, Game.wave])
		var bad := 0
		for u in b.units:
			if is_instance_valid(u) and absf(u.wx) >= half:
				bad += 1
		for p: Dictionary in b._props:
			if p["is_floor"] and absf(p["wx"]) >= half:   # wall lamps sit flush on the wall face by design
				bad += 1
		assert(bad == 0, "%d units/floor props at or outside half %.0f" % [bad, half])
		assert(absf(Hall3D.curve_a - b.CURVE_A_BY_WAVE[Game.wave]) < 0.01, "curve_a %.3f != table %.3f after 3s" % [Hall3D.curve_a, b.CURVE_A_BY_WAVE[Game.wave]])
		# owner directive: the bend must never carry a wall off screen. Sweep from d0 -- the first
		# depth (snapped to the 64-step sample grid) where the wall could appear on screen AT ALL
		# with zero bend: below Hall3D.NEAR_D the floor row itself renders below the viewport
		# (Hall3D.NEAR_D=128 sits under the visible frame; it only appears ~d=240), and below the
		# hall's own static-fit depth the UNBENT wall is already wider than the viewport -- hall
		# geometry vs camera FOV, nothing the bend can fix. From d0 on, curve_a/curve_l must not
		# push either wall's screen x outside a 40px margin, across one full phase cycle.
		var vp_w: float = get_viewport().get_visible_rect().size.x
		var vp_h: float = get_viewport().get_visible_rect().size.y
		var margin := 40.0
		var l: float = b.CURVE_L_BY_WAVE[Game.wave]
		var d0 := 0.0
		for dd in range(128, 1000, 4):
			if Hall3D.unproject(b.camera, 0.0, dd).y <= vp_h and Hall3D.unproject(b.camera, -half, dd).x >= margin and Hall3D.unproject(b.camera, half, dd).x <= vp_w - margin:
				d0 = dd
				break
		d0 = ceil(d0 / 64.0) * 64.0   # one step of slack on the sweep's own 64-unit sample grid
		var saved_phase := Hall3D.phase
		var lo := INF
		var hi := -INF
		var worst_phase := 0.0
		var worst_extent := 0.0   # biggest |screen_x - center|, to pose the screenshot at the strongest swing found
		var center := vp_w * 0.5
		var d := d0
		while d <= Hall3D.FOG_END:
			for i in range(33):
				var ph := TAU * l * i / 32.0
				Hall3D.phase = ph
				var lxi := Hall3D.unproject(b.camera, -half, d).x
				var rxi := Hall3D.unproject(b.camera, half, d).x
				lo = minf(lo, minf(lxi, rxi))
				hi = maxf(hi, maxf(lxi, rxi))
				var extent := maxf(absf(lxi - center), absf(rxi - center))
				if extent > worst_extent:
					worst_extent = extent
					worst_phase = ph
			d += 64.0
		Hall3D.phase = saved_phase
		print("PROBE walls sweep hall=%d half=%.0f a=%.3f l=%.0f d0=%.0f x_range=[%.0f, %.0f] bound=[%.0f, %.0f]" % [Game.wave + 1, half, Hall3D.curve_a, l, d0, lo, hi, margin, vp_w - margin])
		assert(lo >= margin and hi <= vp_w - margin, "bend carried a wall off frame: hall=%d x in [%.0f, %.0f] outside [%.0f, %.0f] from d0=%.0f" % [Game.wave + 1, lo, hi, margin, vp_w - margin, d0])
		var lx := Hall3D.unproject(b.camera, -half, b.FRONT_D).x
		var rx := Hall3D.unproject(b.camera, half, b.FRONT_D).x
		b.scroll = worst_phase   # pose the screenshot at the sweep's strongest swing, not whatever phase happened to be live
		for _i in range(3):
			await get_tree().process_frame
		var shot := "user://probe_walls_hall%d.png" % (Game.wave + 1)
		get_viewport().get_texture().get_image().save_png(shot)
		print("PROBE walls ok: hall=%d half=%.0f units=%d curve_a=%.3f wall_sep_px=%.0f worst_phase=%.0f saved=%s" % [Game.wave + 1, half, b.units.size(), Hall3D.curve_a, rx - lx, worst_phase, ProjectSettings.globalize_path(shot)])
	if check.begins_with("stress"):
		# stress: grow the block to N rows (default 20) and report FPS after `secs`
		var rows := int(check.trim_prefix("stress")) if check.length() > 6 else 20
		await get_tree().create_timer(1.0).timeout
		var b := get_tree().current_scene
		var extra := Army.block(b, b.world, Game.waves[Game.wave]["reserves"], 700.0, rows, 285.0 - 7 * Army.RANK_STEP, b._rng)
		for u in extra:
			u.died.connect(b._on_died)
			b.units.append(u)
		print("PROBE stress units=%d" % b.units.size())
	if check.begins_with("horde"):
		# horde: dump N enemies (default 500) into the hall at once, then fight; report FPS + sim ms
		var n := int(check.trim_prefix("horde")) if check.length() > 5 else 500
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
	if check == "jump":
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
