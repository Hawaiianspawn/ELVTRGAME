extends Node2D
## Four waves seen from behind the army. World space: x lateral, d depth ahead of the camera (View.gd projects).
## Front line holds at FRONT_D; reserves stand in packed ranks behind it and step forward into the lane whose
## front unit died, taking the type the player set for that lane. Hero walks the gap between ranks and line,
## siphons magic from kills, spends it on spells; relics unlock on lifetime magic.

const FRONT_D := 320.0
const SPAWN_D := 1500.0
const LANE_W := 64.0
const HERO_MIN_D := 205.0
const HERO_MAX_D := 290.0
const RANK_D0 := 285.0          # first rank behind the front line
const RANK_STEP := 24.0
const RANK_X := 32.0
const CREEP := 6.0              # forward crawl of the whole army, world units / s
const TYPES := ["shield", "pike", "archer", "greatsword"]

var view := View.new()
var lanes := 5
var lane_types: Array[String] = []
var front: Array = []             # Unit or null per lane
var spawn_queue: Array[String] = []
var spawn_t := 0.0
var spawn_interval := 1.5
var magic_rate := 0.6
var mote_t := 0.0
var units: Array[Unit] = []
var motes: Array[Dictionary] = []      # {wx, wd, v}
var hero: Node2D
var hero_sprite: Sprite2D
var hero_wx := 0.0
var hero_wd := 245.0
var hero_hp := 100.0
var world: Node2D
var fx: Node2D
var hud: Node2D
var scenery: Array[Sprite2D] = []      # foreground rows + far horde, meta wx/wd
var scroll := 0.0
var spell_cd := {"bolt": 0.0, "heal": 0.0, "wall": 0.0}
var checkpoint_magic := 0.0
var toast := ""
var toast_t := 0.0
var _rng := RandomNumberGenerator.new()
var _done := false


func _ready() -> void:
	_rng.randomize()
	world = Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	fx = Node2D.new()
	fx.z_index = 5
	add_child(fx)
	hud = Node2D.new()
	hud.z_index = 10
	hud.draw.connect(_draw_hud)
	add_child(hud)
	hero = Node2D.new()
	hero_sprite = Game.make_sprite("mage", 4)
	hero.add_child(hero_sprite)
	world.add_child(hero)
	_build_scenery()
	start_wave(Game.wave)


func _scenery_sprite(name: String, facing: int, wx: float, wd: float, tint: Color, k: float) -> void:
	var s := Game.make_sprite(name, facing)
	s.set_meta("k", k)
	s.set_meta("wx", wx)
	s.set_meta("wd", wd)
	s.set_meta("tint", tint)
	world.add_child(s)
	scenery.append(s)


func _build_scenery() -> void:
	# Foreground: two packed rows of our own army between the hero and the camera, cropped by the frame.
	for row in range(2):
		var d := 150.0 + row * 35.0
		var x := -700.0 + row * 20.0
		while x <= 700.0:
			_scenery_sprite(TYPES[_rng.randi_range(0, 3)], 4, x + _rng.randf_range(-5, 5), d + _rng.randf_range(-5, 5), Color(0.62, 0.62, 0.66), 1.0)
			x += 40.0
	# Far: the horde massing under the green dot, flat silhouettes.
	for row in range(6):
		var d := 720.0 + row * 110.0
		var x := -1100.0 + (row % 2) * 17.0
		while x <= 1100.0:
			_scenery_sprite(["undead", "armored", "ooze", "undead2"][_rng.randi_range(0, 3)], 0, x + _rng.randf_range(-8, 8), d + _rng.randf_range(-20, 20), Color(0.23, 0.24, 0.25), 2.2)
			x += 34.0


func start_wave(i: int) -> void:
	Game.wave = i
	checkpoint_magic = Game.magic
	for u in units:
		if is_instance_valid(u):
			u.queue_free()
	units.clear()
	motes.clear()
	var w: Dictionary = Game.waves[i]
	lanes = int(w["lanes"])
	spawn_interval = float(w["spawn_interval"])
	magic_rate = float(w["magic_rate"])
	while lane_types.size() < lanes:
		lane_types.append(TYPES[lane_types.size() % 4])
	front.resize(lanes)
	front.fill(null)
	spawn_queue.clear()
	for e in w["enemies"]:
		for n in range(int(e["count"])):
			spawn_queue.append(e["type"])
	spawn_queue.shuffle()
	_build_ranks(w["reserves"])
	spawn_t = 2.0
	hero_hp = 100.0
	# camera: a touch higher and wider each wave so the widening army still fits
	var t := i / 3.0
	var tw := create_tween().set_parallel(true)
	tw.tween_property(view, "focal", lerpf(360.0, 290.0, t), 1.5)
	tw.tween_property(view, "cam_h", lerpf(190.0, 230.0, t), 1.5)
	say("Wave %d / 4" % (i + 1))


func _build_ranks(reserves: Dictionary) -> void:
	# Interleave the types so every rank reads as a mixed line, then fill rows nearest the front first.
	var pool: Array[String] = []
	var left := reserves.duplicate()
	while true:
		var any := false
		for t in TYPES:
			if int(left.get(t, 0)) > 0:
				pool.append(t)
				left[t] = int(left[t]) - 1
				any = true
		if not any:
			break
	var half := (lanes - 1) / 2.0 * LANE_W + LANE_W
	var per_row := int(floor(half * 2.0 / RANK_X)) + 1
	for k in range(pool.size()):
		var row := k / per_row
		var col := k % per_row
		var u := spawn(pool[k], Unit.ALLY)
		u.state = Unit.State.RANK
		u.wx = -half + col * RANK_X + (row % 2) * RANK_X * 0.5 + _rng.randf_range(-3, 3)
		u.wd = RANK_D0 - row * RANK_STEP + _rng.randf_range(-2, 2)
		u.hold_d = u.wd
		u.home = Vector2(u.wx, u.wd)


func lane_x(l: int) -> float:
	return (l - (lanes - 1) / 2.0) * LANE_W


func _rank_unit(type: String, l: int) -> Unit:
	var best: Unit = null
	var best_d := INF
	for u in units:
		if u.team == Unit.ALLY and u.state == Unit.State.RANK and u.type == type:
			var d := absf(u.wx - lane_x(l)) + (RANK_D0 - u.wd) * 2.0
			if d < best_d:
				best_d = d
				best = u
	return best


func lane_open(l: int) -> bool:
	return front[l] == null and _rank_unit(lane_types[l], l) == null


func find_target(u: Unit) -> Node2D:
	var best: Node2D = null
	var best_d := INF
	for o in units:
		if not is_instance_valid(o) or o.team == u.team or o.state == Unit.State.RANK or o.state == Unit.State.RETREAT or absi(o.lane - u.lane) > 1:
			continue
		var d := Vector2(u.wx, u.wd).distance_to(Vector2(o.wx, o.wd))
		if d < best_d:
			best_d = d
			best = o
	if best == null and u.team == Unit.ENEMY and lane_open(u.lane):
		return hero
	return best


func hit(a: Unit, t: Node2D) -> void:
	var d := a.dmg
	if t is Unit and t.type in a.counters:
		d *= float(Game.units["counter_mult"])
	if a.team == Unit.ALLY:
		d *= 1.0 + Game.relic_bonus("dmg")
	if t is Unit:
		t.take(d)
	elif t == hero:
		hero_hp -= d
		hero_sprite.modulate = Color(2, 1, 1)


func spawn(type: String, team: int) -> Unit:
	var u := Unit.new()
	u.setup(type, team, self)
	u.died.connect(_on_died)
	world.add_child(u)
	units.append(u)
	return u


func _spawn_enemy(type: String) -> void:
	var u := spawn(type, Unit.ENEMY)
	u.lane = _rng.randi_range(0, lanes - 1)
	u.wx = lane_x(u.lane) + _rng.randf_range(-14, 14)
	u.wd = SPAWN_D + _rng.randf_range(0, 120)
	u.hold_d = FRONT_D + 40.0


func _on_died(u: Unit) -> void:
	units.erase(u)
	if u.team == Unit.ALLY:
		if u.lane >= 0 and front[u.lane] == u:
			front[u.lane] = null
	else:
		motes.append({"wx": u.wx, "wd": u.wd, "v": 6.0 + u.max_hp * 0.08})


func say(t: String) -> void:
	toast = t
	toast_t = 2.5


func _cursor_world() -> Vector2:
	var c := get_global_mouse_position()
	var d := clampf(view.depth_at_y(c.y), 60.0, SPAWN_D)
	return Vector2(view.x_at(c.x, d), d)


func _process(delta: float) -> void:
	queue_redraw()
	hud.queue_redraw()
	toast_t -= delta
	scroll += CREEP * delta
	# scenery re-projects every frame (camera tweens per wave); horde creeps toward us and wraps
	for s in scenery:
		var wd: float = s.get_meta("wd")
		if wd > 600.0:
			wd -= CREEP * 0.5 * delta
			if wd < 710.0:
				wd += 660.0
			s.set_meta("wd", wd)
		s.position = view.project(s.get_meta("wx"), wd)
		s.scale = Vector2.ONE * view.sprite_scale(wd) * float(s.get_meta("k"))
		s.modulate = (s.get_meta("tint") as Color) * (Color.WHITE if wd > 600.0 else view.fog(wd))
	if _done:
		return
	# spawns
	spawn_t -= delta
	if spawn_t <= 0.0 and not spawn_queue.is_empty():
		spawn_t = spawn_interval
		_spawn_enemy(spawn_queue.pop_back())
	# refill front slots from the ranks
	for l in range(lanes):
		if front[l] == null:
			var u := _rank_unit(lane_types[l], l)
			if u:
				u.lane = l
				u.hold_d = FRONT_D
				u.state = Unit.State.ADVANCE
				front[l] = u
	# ambient magic from the ground veins
	mote_t -= delta
	if mote_t <= 0.0:
		mote_t = 1.0 / magic_rate
		motes.append({"wx": _rng.randf_range(lane_x(0), lane_x(lanes - 1)), "wd": _rng.randf_range(400, 900), "v": 3.0})
	# hero
	var mv := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	hero_wx = clampf(hero_wx + mv.x * 130.0 * delta, lane_x(0) - 40, lane_x(lanes - 1) + 40)
	hero_wd = clampf(hero_wd - mv.y * 110.0 * delta, HERO_MIN_D, HERO_MAX_D)
	hero.position = view.project(hero_wx, hero_wd)
	hero.scale = Vector2.ONE * view.sprite_scale(hero_wd)
	hero_sprite.modulate = hero_sprite.modulate.lerp(Color.WHITE, delta * 6.0)
	if mv != Vector2.ZERO:
		hero_sprite.frame = Game.facing_from(mv)
	# siphon: hold RMB, motes near the cursor stream to the hero
	var siphoning := Input.is_action_pressed("siphon")
	var cur := _cursor_world()
	for m in motes.duplicate():
		var p := Vector2(m["wx"], m["wd"])
		var vel := Vector2(0, -22.0)
		if siphoning and p.distance_to(cur) < 120.0:
			vel = (Vector2(hero_wx, hero_wd) - p).normalized() * 380.0
		p += vel * delta
		m["wx"] = p.x
		m["wd"] = p.y
		if p.distance_to(Vector2(hero_wx, hero_wd)) < 22.0:
			for r in Game.gain_magic(float(m["v"])):
				say("Relic: " + _relic(r)["label"] + " - " + _relic(r)["desc"])
			motes.erase(m)
		elif p.y < 80.0:
			motes.erase(m)
	for k in spell_cd:
		spell_cd[k] -= delta
	# lose / win
	if hero_hp <= 0.0:
		say("The line breaks. Again.")
		Game.magic = checkpoint_magic
		start_wave(Game.wave)
	elif spawn_queue.is_empty() and units.filter(func(u): return is_instance_valid(u) and u.team == Unit.ENEMY).is_empty():
		_wave_done()


func _wave_done() -> void:
	_done = true
	if Game.wave >= 3:
		say("The line holds. Barely.")
		await get_tree().create_timer(2.0).timeout
		Game.goto("charge")
	else:
		say("Wave cleared. Magic siphoned: %d" % int(Game.magic))
		await get_tree().create_timer(2.5).timeout
		_done = false
		start_wave(Game.wave + 1)


func _relic(id: String) -> Dictionary:
	for r in Game.spells["relics"]:
		if r["id"] == id:
			return r
	return {}


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("cast"):
		_cast("bolt")
	elif e is InputEventKey and e.pressed and not e.echo:
		match e.keycode:
			KEY_1: _cast("bolt")
			KEY_2: _cast("heal")
			KEY_3: _cast("wall")
			KEY_Q: _cycle_lane(-1)
			KEY_E: _cycle_lane(1)


func _hover_lane() -> int:
	var x := _cursor_world().x
	return clampi(int(round((x - lane_x(0)) / LANE_W)), 0, lanes - 1)


func _rank_count(type: String) -> int:
	var n := 0
	for u in units:
		if u.team == Unit.ALLY and u.state == Unit.State.RANK and u.type == type:
			n += 1
	return n


func _cycle_lane(dir: int, l: int = -1) -> void:
	if l < 0:
		l = _hover_lane()
	lane_types[l] = TYPES[(TYPES.find(lane_types[l]) + dir + 4) % 4]
	# swap out: the standing front unit falls back to its rank slot, the new type steps up on the next tick
	var u = front[l]
	if u != null and is_instance_valid(u) and u.type != lane_types[l]:
		front[l] = null
		u.lane = -1
		u.state = Unit.State.RETREAT
	say("Lane %d -> %s (%d in the ranks)" % [l + 1, Game.units[lane_types[l]]["label"], _rank_count(lane_types[l])])


func _spell(id: String) -> Dictionary:
	for s in Game.spells["spells"]:
		if s["id"] == id:
			return s
	return {}


func _cast(id: String) -> void:
	var s := _spell(id)
	if spell_cd[id] > 0.0:
		return
	if Game.magic < float(s["cost"]):
		say("Not enough magic (%d)" % int(s["cost"]))
		return
	Game.magic -= float(s["cost"])
	spell_cd[id] = float(s["cooldown"])
	var cur := _cursor_world()
	match id:
		"bolt":
			var r := 70.0 if Game.has_relic_flag("splash") else 32.0
			_flash(cur, r, Color(0.5, 1.0, 0.5))
			for u in units.duplicate():
				if u.team == Unit.ENEMY and Vector2(u.wx, u.wd).distance_to(cur) < r:
					u.take(25.0)
		"heal":
			for u in front:
				if u != null and is_instance_valid(u):
					u.hp = minf(u.max_hp, u.hp + 25.0)
					_flash(Vector2(u.wx, u.wd), 20.0, Color(1.0, 0.9, 0.5))
		"wall":
			_flash(cur, 90.0, Color(0.3, 0.9, 0.3, 0.6))
			for u in units:
				if u.team == Unit.ENEMY and Vector2(u.wx, u.wd).distance_to(cur) < 90.0:
					u.rooted_until = Time.get_ticks_msec() / 1000.0 + 3.0


func _flash(wp: Vector2, r: float, c: Color) -> void:
	var n := Node2D.new()
	n.position = view.project(wp.x, wp.y)
	var k := view.s(wp.y)
	n.draw.connect(func(): n.draw_circle(Vector2.ZERO, r * k, c))
	fx.add_child(n)
	var t := create_tween()
	t.tween_property(n, "modulate:a", 0.0, 0.35)
	t.tween_callback(n.queue_free)


func _draw() -> void:
	# ponytail: procedural ground bands; owner says real terrain lands here later — replace this one call.
	view.draw_ground(self, scroll, (lanes - 1) / 2.0 * LANE_W + 400.0)
	# the green dot on the horizon
	var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
	var dot := Vector2(480, view.horizon - 8)
	draw_circle(dot, 6.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
	draw_line(dot, dot + Vector2(_rng.randf_range(-30, 30), -110), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)


func _draw_hud() -> void:
	# motes + hero flame ride the top layer so they glow over the ranks
	for m in motes:
		var k := view.s(m["wd"])
		hud.draw_circle(view.project(m["wx"], m["wd"]) + Vector2(0, -10 * k), (2.5 + float(m["v"]) * 0.12) * k, Color(0.4, 1.0, 0.4, 0.9))
	hud.draw_circle(hero.position + Vector2(0, -60 * hero.scale.y), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	# lane labels: bottom strip, under each lane's screen x
	hud.draw_rect(Rect2(0, 512, 960, 28), Color(0, 0, 0, 0.55))
	var f := ThemeDB.fallback_font
	for l in range(lanes):
		var t: String = Game.units[lane_types[l]]["label"] + " %d" % _rank_count(lane_types[l])
		var col := Color("#f0c260") if l == _hover_lane() else Color("#a0a08b")
		var p := Vector2(view.project(lane_x(l), FRONT_D).x, 528.0)
		hud.draw_string(f, p + Vector2(-32, 0), t, HORIZONTAL_ALIGNMENT_CENTER, 64, 11, col)
	# HUD
	var lines := ["WAVE %d / 4" % (Game.wave + 1), "HERO %d" % int(hero_hp), "MAGIC %d  (lifetime %d)" % [int(Game.magic), int(Game.magic_ever)],
		"1 Bolt 8   2 Mend 20   3 Wall 30    Q/E lane type    RMB siphon", "Relics: " + ", ".join(Game.relics), "FPS %d" % Engine.get_frames_per_second()]
	for i in range(lines.size()):
		hud.draw_string(f, Vector2(12, 20 + i * 18), lines[i], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#e9efec"))
	if toast_t > 0.0:
		hud.draw_string(f, Vector2(300, 60), toast, HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color("#f0c260"))
