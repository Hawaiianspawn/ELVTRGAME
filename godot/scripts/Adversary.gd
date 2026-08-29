extends Node
## Adversarial QA agent. `godot --path godot -- --adversary=battle,60,7` loads a scene and for N
## seconds cycles hostile behaviors (wall-hugging, spell/swap spam, teleports, hordes, phase jumps)
## while checking invariants every frame. Findings land in user://adversary_<seed>.json + .csv.
## Inert without the arg. "Broken" = any invariant below; each is a rule the game's own code claims.

const TICK := 0.6                       # seconds a behavior is held before the next pick
const STUCK := {"RETREAT": 12.0, "ADVANCE": 15.0, "charge": 10.0}
const BEHAVIORS := [
	"move_random", "wall_hug", "corner_hold", "cursor_warp",
	"gatling_spam", "swap_spam", "teleport_hero", "upgrade_flood", "cannon_spam",
	"horde", "launch_all", "kill_front", "kill_hero", "jump_phase",
]

var _rng := RandomNumberGenerator.new()
var _seed := 0
var _scene := "battle"
var _secs := 60.0
var _t := 0.0
var _tick_left := 0.0
var _behavior := "none"
var _behavior_runs := {}
var _findings: Array[Dictionary] = []
var _seen := {}                         # dedupe key -> finding index
var _since := {}                        # unit instance id -> {"key": str, "t": float}
var _skip_frames := 0                   # frames to ignore the hero clamp after a teleport
var _held: Array[String] = []           # actions pressed this tick
var _fps_low := 0.0
var _t0 := 0.0                          # wall clock start: the game can slow its own clock, the agent must not
var _slow_since := -1.0                 # wall time Engine.time_scale first dropped below 1


func _ready() -> void:
	set_process(false)   # inert: _process is on by default for any node that defines it
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--adversary="):
			_run(a.trim_prefix("--adversary="))


func _run(spec: String) -> void:
	var parts := spec.split(",")
	_scene = parts[0] if parts[0] != "" else "battle"
	_secs = float(parts[1]) if parts.size() > 1 else 60.0
	_seed = int(parts[2]) if parts.size() > 2 else randi() % 100000
	_rng.seed = _seed
	await get_tree().process_frame
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Engine.max_fps = 0
	get_tree().change_scene_to_file(Game.SCENES[_scene])
	await get_tree().create_timer(2.0).timeout
	print("ADVERSARY start scene=%s secs=%.0f seed=%d" % [_scene, _secs, _seed])
	_t0 = Time.get_ticks_msec() / 1000.0
	set_process(true)


func _process(delta: float) -> void:
	if _secs <= 0.0:
		return
	var now := Time.get_ticks_msec() / 1000.0 - _t0
	var real_dt := now - _t
	_t = now
	_tick_left -= real_dt
	if _tick_left <= 0.0:
		_tick_left = TICK
		_release()
		_behavior = BEHAVIORS[_rng.randi_range(0, BEHAVIORS.size() - 1)]
		_behavior_runs[_behavior] = int(_behavior_runs.get(_behavior, 0)) + 1
		_act()
	_sustain()
	_check(real_dt)
	if _t >= _secs:
		_secs = 0.0
		_release()
		_finish()


# ---- behaviors -------------------------------------------------------------------------

func _battle() -> Node:
	var s := get_tree().current_scene
	return s if s != null and "units" in s else null


func _hold(actions: Array) -> void:
	for a in actions:
		Input.action_press(a)
		_held.append(a)


func _release() -> void:
	for a in _held:
		Input.action_release(a)
	_held.clear()


func _act() -> void:
	var b := _battle()
	match _behavior:
		"move_random":
			_hold([["move_up", "move_down", "move_left", "move_right"][_rng.randi_range(0, 3)]])
		"wall_hug":
			_hold(["move_left" if _rng.randf() < 0.5 else "move_right"])
			_tick_left = TICK * 4.0
		"corner_hold":
			_hold(["move_left" if _rng.randf() < 0.5 else "move_right", "move_up" if _rng.randf() < 0.5 else "move_down"])
			_tick_left = TICK * 3.0
		"cursor_warp":
			Input.warp_mouse(Vector2(_rng.randf_range(-200, 1200), _rng.randf_range(-200, 800)))
		"jump_phase":
			_since.clear()
			Jump._go(_rng.randi_range(0, Jump.PHASES.size() - 1))
		_:
			if b == null:
				return
			match _behavior:
				"teleport_hero":
					b.hero_wx = _rng.randf_range(-3000, 3000)
					b.hero_wd = _rng.randf_range(-3000, 3000)
					_skip_frames = 2
				"upgrade_flood":
					# every chest upgrade repeatedly: watch for overflow/NaN in the stacked multipliers
					for i in range(20):
						b._grant_upgrade()
				"horde":
					for i in range(60):
						b._spawn_enemy(["undead", "mace_undead", "ooze", "archer_undead"][i % 4])
						var u: Unit = b.units[-1]
						u.wd = b.FRONT_D + 40.0 + (i / 6) * 20.0
				"launch_all":
					for u in b.units:
						if not u.dead:
							u.launch(400.0)
				"kill_front":
					for u in b.front:
						if u != null and is_instance_valid(u):
							u.take(9999.0)
				"kill_hero":
					b.hero_hp = 0.0


## Per-frame part of the held behavior: the spam ones hammer every frame, not once per tick.
func _sustain() -> void:
	var b := _battle()
	if b == null:
		return
	match _behavior:
		"gatling_spam":
			b._gatling_hit_at(Vector2(_rng.randf_range(0.0, 960.0), _rng.randf_range(0.0, 540.0)))
		"cannon_spam":
			# bypass the reload cooldown to stress the blast/hitstop path every frame
			b._cannon_explode_at(Hall3D.cursor_world(b.camera, Vector2(_rng.randf_range(0.0, 960.0), _rng.randf_range(0.0, 540.0)), b.SPAWN_D))
		"swap_spam":
			b._cycle_army(1 if _rng.randf() < 0.5 else -1)


# ---- invariants ------------------------------------------------------------------------

func _check(delta: float) -> void:
	if _t < 1.5:
		return   # scene still loading in: the first frames render nothing, so a screenshot would show nothing
	var fps := Engine.get_frames_per_second()
	if _t > 3.0 and fps < 20:
		_fps_low += delta
		if _fps_low > 2.0:
			_log("performance", "engine", "fps %d for >2s" % fps, {})
	else:
		_fps_low = 0.0
	if Engine.time_scale < 1.0:
		if _slow_since < 0.0:
			_slow_since = _t
		elif _t - _slow_since > 1.0:
			_log("exploit", "Battle._hitstop", "Engine.time_scale=%.2f held %.1fs real time (hit stop re-armed every frame)" % [Engine.time_scale, _t - _slow_since], {})
	else:
		_slow_since = -1.0
	if is_nan(Game.magic) or Game.magic < 0.0:
		_log("logic_violation", "Game.magic", "magic=%s (negative or NaN; cast cost check let it through)" % Game.magic, {})
	var b := _battle()
	if b == null:
		return
	if _skip_frames > 0:
		_skip_frames -= 1
	elif is_nan(b.hero_wx) or is_nan(b.hero_wd):
		_log("boundary_break", "Battle.hero", "hero position is NaN", {})
	elif absf(b.hero_wx) > b.HALL_HALF - 30.0 + 0.01 or b.hero_wd < b.HERO_MIN_D - 0.01 or b.hero_wd > b.HERO_MAX_D + 0.01:
		_log("boundary_break", "Battle.hero", "hero outside clamp wx=%.0f wd=%.0f" % [b.hero_wx, b.hero_wd], {})
	if b.units.size() > 1500:
		_log("resource_leak", "Battle.units", "%d units alive" % b.units.size(), {})
	for k in b.pool:
		if int(b.pool[k]) < 0:
			_log("logic_violation", "Battle.pool", "pool[%s]=%d negative" % [k, b.pool[k]], {})
	var seen_front := {}
	for l in range(b.front.size()):
		var f = b.front[l]
		if f == null:
			continue
		if not is_instance_valid(f) or f.dead:
			_log("dangling_ref", "Battle.front", "lane %d front is freed/dead unit" % l, {"lane": l})
		elif seen_front.has(f):
			_log("logic_violation", "Battle.front", "same unit fronts lanes %d and %d" % [seen_front[f], l], {"lane": l})
		elif f.lane != l:
			_log("logic_violation", "Battle.front", "front[%d] unit thinks it is in lane %d" % [l, f.lane], {"lane": l})
		seen_front[f] = l
	var live := {}
	for u in b.units:
		if not is_instance_valid(u):
			_log("dangling_ref", "Battle.units", "freed unit still in units[]", {})
			continue
		live[u.get_instance_id()] = true
		var where := {"unit_type": u.type, "team": "ally" if u.team == Unit.ALLY else "enemy", "wx": snappedf(u.wx, 1), "wd": snappedf(u.wd, 1), "state": Unit.State.keys()[u.state]}
		if is_nan(u.wx) or is_nan(u.wd) or is_nan(u.hp) or is_nan(u.air_h):
			_log("boundary_break", "Unit", "NaN in wx/wd/hp/air_h", where)
			continue
		if u.hp > u.max_hp + 0.01:
			_log("logic_violation", "Unit.hp", "hp %.0f > max_hp %.0f" % [u.hp, u.max_hp], where)
		if u.hp <= 0.0 and not u.dead:
			_log("logic_violation", "Unit.take", "hp <= 0 but not dead", where)
		if u.dead and u.team == Unit.ALLY and b.front.has(u):
			_log("dangling_ref", "Battle.front", "dead ally still fronting a lane", where)
		if u.air_h < 0.0:
			_log("boundary_break", "Unit.air", "air_h %.1f below ground" % u.air_h, where)
		if u.air_h > 0.0 and not u.sky_slam:
			# juggling is the game; leaving the frame is not: sprite top must stay on screen (sky-drop entrances excepted)
			var top: float = Hall3D.sprite_top_screen(b.camera, u).y
			if top < -2.0:
				_log("boundary_break", "Unit.air", "airborne unit above the frame: sprite top y=%.0f air_h=%.0f" % [top, u.air_h], where)
		if absf(u.wx) > b.HALL_HALF:
			_log("boundary_break", "Battle.hall", "unit through the hall wall |wx| %.0f > HALL_HALF %.0f" % [absf(u.wx), b.HALL_HALF], where)
		var floor_d: float = b.ENEMY_MIN_D - 1.0 if u.team == Unit.ENEMY else 0.0
		if u.wd < floor_d or u.wd > b.SPAWN_D + 200.0:
			_log("boundary_break", "Battle.depth", "unit depth %.0f outside %.0f..%.0f" % [u.wd, floor_d, b.SPAWN_D + 200.0], where)
		# stuck: same transient key held longer than its budget
		var key := ""
		if u.charge_to > 0.0:
			key = "charge"
		elif u.state == Unit.State.RETREAT:
			key = "RETREAT"
		elif u.state == Unit.State.ADVANCE:
			key = "ADVANCE"
		var id: int = u.get_instance_id()
		if key == "":
			_since.erase(id)
		elif not _since.has(id) or _since[id]["key"] != key:
			_since[id] = {"key": key, "t": _t}
		elif _t - float(_since[id]["t"]) > float(STUCK[key]) and not _since[id].has("logged"):
			_since[id]["logged"] = true
			_log("stuck_state", "Unit." + key, "%s for %.0fs (budget %.0fs)" % [key, _t - float(_since[id]["t"]), STUCK[key]], where)
	for id in _since.keys():
		if not live.has(id):
			_since.erase(id)


func _log(error_type: String, system: String, detail: String, where: Dictionary) -> void:
	var b := _battle()
	var scene := get_tree().current_scene.scene_file_path.get_file().get_basename() if get_tree().current_scene else "none"
	var dkey := "%s|%s|%s" % [error_type, system, where.get("unit_type", "")]
	if _seen.has(dkey):
		_findings[_seen[dkey]]["count"] += 1
		_findings[_seen[dkey]]["last_t"] = snappedf(_t, 0.1)
		return
	var loc := {"scene": scene, "wave": Game.wave, "system": system}
	loc.merge(where)
	var ctx := {"behavior": _behavior, "magic": snappedf(Game.magic, 1), "hero_hp": snappedf(b.hero_hp, 1) if b else null,
		"army_type": b.army_type if b else null, "units": b.units.size() if b else 0,
		"enemies": b.units.filter(func(u): return is_instance_valid(u) and u.team == Unit.ENEMY).size() if b else 0,
		"fps": Engine.get_frames_per_second(), "held_inputs": _held.duplicate()}
	_seen[dkey] = _findings.size()
	var shot := "adversary_%d_f%d.png" % [_seed, _findings.size() + 1]
	get_viewport().get_texture().get_image().save_png("user://" + shot)   # the frame the rule broke on
	_findings.append({"id": _findings.size() + 1, "t": snappedf(_t, 0.1), "last_t": snappedf(_t, 0.1), "count": 1,
		"error_type": error_type, "location": loc, "game_context": ctx, "detail": detail, "screenshot": shot})
	print("ADVERSARY %s @%s: %s" % [error_type, system, detail])


# ---- report ----------------------------------------------------------------------------

func _finish() -> void:
	var report := {
		"run": {"scene": _scene, "seconds": snappedf(_t, 0.1), "seed": _seed, "date": Time.get_datetime_string_from_system(),
			"behaviors": _behavior_runs, "invariants": ["hero clamp", "magic >= 0", "unit hp in 0..max", "dead flag matches hp",
			"front[] refs live + lane-consistent", "pool >= 0", "unit inside hall walls + depth range", "air_h >= 0",
			"no NaN", "airborne units stay inside the frame", "enemies never below ENEMY_MIN_D", "transient states end within budget", "units[] < 1500", "fps >= 20", "Engine.time_scale back to 1 within 1s"]},
		"findings": _findings,
	}
	var base := "user://adversary_%d" % _seed
	var f := FileAccess.open(base + ".json", FileAccess.WRITE)
	f.store_string(JSON.stringify(report, "  "))
	f.close()
	var c := FileAccess.open(base + ".csv", FileAccess.WRITE)
	c.store_csv_line(PackedStringArray(["id", "t", "count", "error_type", "scene", "wave", "system", "unit_type", "team", "state", "wx", "wd", "behavior", "magic", "army_type", "units", "enemies", "fps", "detail", "screenshot"]))
	for x in _findings:
		var l: Dictionary = x["location"]
		var g: Dictionary = x["game_context"]
		c.store_csv_line(PackedStringArray([str(x["id"]), str(x["t"]), str(x["count"]), x["error_type"], l["scene"], str(l["wave"]), l["system"],
			str(l.get("unit_type", "")), str(l.get("team", "")), str(l.get("state", "")), str(l.get("wx", "")), str(l.get("wd", "")),
			g["behavior"], str(g["magic"]), str(g["army_type"]), str(g["units"]), str(g["enemies"]), str(g["fps"]), x["detail"], x["screenshot"]]))
	c.close()
	var img := get_viewport().get_texture().get_image()
	img.save_png(base + ".png")
	print("ADVERSARY done findings=%d report=%s" % [_findings.size(), ProjectSettings.globalize_path(base + ".json")])
	get_tree().quit()
