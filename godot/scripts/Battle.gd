extends Node2D
## Four waves. Lanes of front-line slots refilled from reserves of the type the player set.
## Hero walks behind the line, siphons magic from kills, spends it on spells; relics unlock on lifetime magic.

const FRONT_Y := 300.0
const SPAWN_Y := 40.0
const LANE_W := 130.0
const HERO_MIN_Y := 340.0
const HERO_MAX_Y := 505.0
const TYPES := ["shield", "pike", "archer", "greatsword"]

var lanes := 5
var lane_types: Array[String] = []
var front: Array = []             # Unit or null per lane
var reserves: Dictionary = {}
var spawn_queue: Array[String] = []
var spawn_t := 0.0
var spawn_interval := 1.5
var magic_rate := 0.6
var mote_t := 0.0
var units: Array[Unit] = []
var motes: Array[Node2D] = []
var hero: Node2D
var hero_sprite: Sprite2D
var hero_hp := 100.0
var camera: Camera2D
var world: Node2D
var fx: Node2D
var spell_cd := {"bolt": 0.0, "heal": 0.0, "wall": 0.0}
var checkpoint_magic := 0.0
var toast := ""
var toast_t := 0.0
var _rng := RandomNumberGenerator.new()
var _done := false


func _ready() -> void:
	_rng.randomize()
	camera = Camera2D.new()
	camera.position = Vector2(480, 270)
	add_child(camera)
	camera.make_current()
	world = Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	fx = Node2D.new()
	fx.z_index = 5
	add_child(fx)
	hero = Node2D.new()
	hero.position = Vector2(480, 420)
	hero_sprite = Game.make_sprite("mage", 4)
	hero.add_child(hero_sprite)
	world.add_child(hero)
	_build_crowd()
	start_wave(Game.wave)


func _build_crowd() -> void:
	# Back rows: big, dim, static allies between the hero and the camera. Decoration only.
	for i in range(12):
		var s := Game.make_sprite(TYPES[i % 4], 4)
		s.position = Vector2(-120 + i * 110 + _rng.randf_range(-15, 15), 500 + _rng.randf_range(0, 60))
		s.modulate = Color(0.55, 0.55, 0.6)
		s.scale = Vector2.ONE * depth_scale(s.position.y)
		world.add_child(s)


func start_wave(i: int) -> void:
	Game.wave = i
	checkpoint_magic = Game.magic
	for u in units:
		if is_instance_valid(u):
			u.queue_free()
	units.clear()
	for m in motes:
		m.queue_free()
	motes.clear()
	var w: Dictionary = Game.waves[i]
	lanes = int(w["lanes"])
	spawn_interval = float(w["spawn_interval"])
	magic_rate = float(w["magic_rate"])
	reserves = w["reserves"].duplicate()
	while lane_types.size() < lanes:
		lane_types.append(TYPES[lane_types.size() % 4])
	front.resize(lanes)
	front.fill(null)
	spawn_queue.clear()
	for e in w["enemies"]:
		for n in range(int(e["count"])):
			spawn_queue.append(e["type"])
	spawn_queue.shuffle()
	spawn_t = 2.0
	hero_hp = 100.0
	var z := clampf(900.0 / (lanes * LANE_W), 0.55, 1.0)
	create_tween().tween_property(camera, "zoom", Vector2(z, z), 1.5)
	say("Wave %d / 4" % (i + 1))


func lane_x(l: int) -> float:
	return 480.0 + (l - (lanes - 1) / 2.0) * LANE_W


func lane_open(l: int) -> bool:
	return front[l] == null and int(reserves.get(lane_types[l], 0)) <= 0


func depth_scale(y: float) -> float:
	return lerpf(0.7, 2.6, clampf((y - SPAWN_Y) / (520.0 - SPAWN_Y), 0.0, 1.0))


func find_target(u: Unit) -> Node2D:
	var best: Node2D = null
	var best_d := INF
	for o in units:
		if not is_instance_valid(o) or o.team == u.team or absi(o.lane - u.lane) > 1:
			continue
		var d := u.position.distance_to(o.position)
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


func spawn(type: String, team: int, l: int) -> Unit:
	var u := Unit.new()
	u.setup(type, team, l, self)
	u.hold_y = FRONT_Y - 30.0 if team == Unit.ENEMY else FRONT_Y
	u.position = Vector2(lane_x(l) + _rng.randf_range(-12, 12), SPAWN_Y if team == Unit.ENEMY else 440.0)
	u.died.connect(_on_died)
	world.add_child(u)
	units.append(u)
	return u


func _on_died(u: Unit) -> void:
	units.erase(u)
	if u.team == Unit.ALLY:
		if front[u.lane] == u:
			front[u.lane] = null
	else:
		_mote(u.position, 6.0 + u.max_hp * 0.08)


func _mote(p: Vector2, value: float) -> void:
	var m := Node2D.new()
	m.position = p
	m.set_meta("v", value)
	m.set_meta("vel", Vector2(_rng.randf_range(-10, 10), 25))
	m.draw.connect(func(): m.draw_circle(Vector2.ZERO, 3.0 + value * 0.15, Color(0.4, 1.0, 0.4, 0.9)))
	fx.add_child(m)
	motes.append(m)


func say(t: String) -> void:
	toast = t
	toast_t = 2.5


func _process(delta: float) -> void:
	queue_redraw()
	toast_t -= delta
	if _done:
		return
	# spawns
	spawn_t -= delta
	if spawn_t <= 0.0 and not spawn_queue.is_empty():
		spawn_t = spawn_interval
		spawn(spawn_queue.pop_back(), Unit.ENEMY, _rng.randi_range(0, lanes - 1))
	# refill front slots
	for l in range(lanes):
		if front[l] == null and int(reserves.get(lane_types[l], 0)) > 0:
			reserves[lane_types[l]] = int(reserves[lane_types[l]]) - 1
			front[l] = spawn(lane_types[l], Unit.ALLY, l)
	# ambient magic from the ground veins
	mote_t -= delta
	if mote_t <= 0.0:
		mote_t = 1.0 / magic_rate
		_mote(Vector2(_rng.randf_range(lane_x(0), lane_x(lanes - 1)), _rng.randf_range(80, 260)), 3.0)
	# hero
	var mv := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	hero.position += mv * 160.0 * delta
	hero.position.x = clampf(hero.position.x, lane_x(0) - 40, lane_x(lanes - 1) + 40)
	hero.position.y = clampf(hero.position.y, HERO_MIN_Y, HERO_MAX_Y)
	hero.scale = Vector2.ONE * depth_scale(hero.position.y)
	hero_sprite.modulate = hero_sprite.modulate.lerp(Color.WHITE, delta * 6.0)
	if mv != Vector2.ZERO:
		hero_sprite.frame = Game.facing_from(mv)
	# siphon: hold RMB, motes near the cursor stream to the hero
	var siphoning := Input.is_action_pressed("siphon")
	var cur := get_global_mouse_position()
	for m in motes.duplicate():
		var vel: Vector2 = m.get_meta("vel")
		if siphoning and m.position.distance_to(cur) < 140.0:
			vel = (hero.position - m.position).normalized() * 420.0
		m.position += vel * delta
		if m.position.distance_to(hero.position) < 24.0:
			for r in Game.gain_magic(float(m.get_meta("v"))):
				say("Relic: " + _relic(r)["label"] + " - " + _relic(r)["desc"])
			motes.erase(m)
			m.queue_free()
		elif m.position.y > 560.0:
			motes.erase(m)
			m.queue_free()
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
		Game.goto("cavalry")
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
	var x := get_global_mouse_position().x
	return clampi(int(round((x - lane_x(0)) / LANE_W)), 0, lanes - 1)


func _cycle_lane(dir: int) -> void:
	var l := _hover_lane()
	lane_types[l] = TYPES[(TYPES.find(lane_types[l]) + dir + 4) % 4]
	say("Lane %d -> %s (%d in reserve)" % [l + 1, Game.units[lane_types[l]]["label"], int(reserves.get(lane_types[l], 0))])


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
	var cur := get_global_mouse_position()
	match id:
		"bolt":
			var r := 70.0 if Game.has_relic_flag("splash") else 30.0
			_flash(cur, r, Color(0.5, 1.0, 0.5))
			for u in units.duplicate():
				if u.team == Unit.ENEMY and u.position.distance_to(cur) < r:
					u.take(25.0)
		"heal":
			for u in front:
				if u != null and is_instance_valid(u):
					u.hp = minf(u.max_hp, u.hp + 25.0)
					_flash(u.position, 20.0, Color(1.0, 0.9, 0.5))
		"wall":
			_flash(cur, 90.0, Color(0.3, 0.9, 0.3, 0.6))
			for u in units:
				if u.team == Unit.ENEMY and u.position.distance_to(cur) < 90.0:
					u.rooted_until = Time.get_ticks_msec() / 1000.0 + 3.0


func _flash(p: Vector2, r: float, c: Color) -> void:
	var n := Node2D.new()
	n.position = p
	n.draw.connect(func(): n.draw_circle(Vector2.ZERO, r, c))
	fx.add_child(n)
	var t := create_tween()
	t.tween_property(n, "modulate:a", 0.0, 0.35)
	t.tween_callback(n.queue_free)


func _draw() -> void:
	# ground bands + horizon + green dot; drawn in world space under everything
	var x0 := lane_x(0) - 400.0
	var x1 := lane_x(lanes - 1) + 400.0
	draw_rect(Rect2(x0, -200, x1 - x0, 240), Color("#211e20"))
	draw_rect(Rect2(x0, 40, x1 - x0, 260), Color("#3a3d3f"))
	draw_rect(Rect2(x0, 300, x1 - x0, 400), Color("#4a4b48"))
	for i in range(0, 12):
		draw_line(Vector2(x0, 40 + i * 24), Vector2(x1, 40 + i * 24), Color(0, 0, 0, 0.12))
	# far silhouettes on the horizon
	for i in range(int((x1 - x0) / 34.0)):
		var h := 18.0 + 8.0 * sin(i * 1.7)
		draw_rect(Rect2(x0 + i * 34, 30 - h, 22, h + 12), Color("#2b2b2e"))
	# the green dot
	var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
	draw_circle(Vector2(480, 24), 6.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
	draw_line(Vector2(480, 24), Vector2(480 + _rng.randf_range(-30, 30), -80), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)
	# lane labels
	var f := ThemeDB.fallback_font
	for l in range(lanes):
		var t: String = Game.units[lane_types[l]]["label"] + " x%d" % int(reserves.get(lane_types[l], 0))
		var col := Color("#f0c260") if l == _hover_lane() else Color("#a0a08b")
		draw_string(f, Vector2(lane_x(l) - 40, 328), t, HORIZONTAL_ALIGNMENT_LEFT, 90, 11, col)
	# hero flame = magic held
	draw_circle(hero.position + Vector2(0, -60 * hero.scale.y), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	# HUD in camera space
	var tl := camera.get_screen_center_position() - Vector2(480, 270) / camera.zoom
	var lines := ["WAVE %d / 4" % (Game.wave + 1), "HERO %d" % int(hero_hp), "MAGIC %d  (lifetime %d)" % [int(Game.magic), int(Game.magic_ever)],
		"1 Bolt 8   2 Mend 20   3 Wall 30    Q/E lane type    RMB siphon", "Relics: " + ", ".join(Game.relics), "FPS %d" % Engine.get_frames_per_second()]
	for i in range(lines.size()):
		draw_string(f, tl + Vector2(12, 20 + i * 18) / camera.zoom, lines[i], HORIZONTAL_ALIGNMENT_LEFT, -1, int(14 / camera.zoom.x), Color("#e9efec"))
	if toast_t > 0.0:
		draw_string(f, tl + Vector2(300, 60) / camera.zoom, toast, HORIZONTAL_ALIGNMENT_LEFT, -1, int(20 / camera.zoom.x), Color("#f0c260"))
