extends Node2D
## Four waves seen from behind the army. World space: x lateral, d depth ahead of the camera (View.gd projects).
## Front line holds at FRONT_D; reserves stand in packed ranks behind it and step forward into the lane whose
## front unit died, taking the type the player set for that lane. Hero walks the gap between ranks and line,
## siphons magic from kills, spends it on spells; relics unlock on lifetime magic.

const FRONT_D := 320.0
const SPAWN_D := 1050.0
const LANE_W := 64.0
const HERO_MIN_D := 150.0
const HERO_MAX_D := 290.0
const RANK_D0 := 285.0          # first rank behind the front line
const CREEP := 14.0             # the push: world units / s the field slides toward the camera
const RANK_ROWS := 7            # depth of the company block down to the camera; every slot is a real unit
const BEHIND_D := 55.0          # just behind the lens: where swaps come from and go to
const TYPES := ["veteran", "halberdier", "hammer", "sheathed", "vet_ranged"]

var view := View.new()
var lanes := 5
var lane_types: Array[String] = []
var army_type := "veteran"
var ability_ready: Dictionary = {}     # type -> time (s) the swap-in ability is ready again
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
var hud_text: Label
var toast_label: Label
var lane_labels: Array[Label] = []
var scenery: Array[Sprite2D] = []      # foreground rows + far horde, meta wx/wd
var scroll := 0.0
var advancing := true
var swap_pending: Array[bool] = []
var pool: Dictionary = {}              # type -> units of that type not on the field
var slots: Array[Vector2] = []         # rank slot positions for this wave
var _grid: Dictionary = {}             # cell -> Array[Unit], rebuilt each frame for separation / near queries
const CELL := 48.0
var decor: Array[Sprite2D] = []
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
	# text lives in Labels: draw_string reshapes every frame, which is what the WASM build feels
	var cl := CanvasLayer.new()
	cl.layer = 20
	add_child(cl)
	hud_text = _label(cl, Vector2(12, 6), 14, Color("#e9efec"))
	toast_label = _label(cl, Vector2(300, 40), 20, Color("#f0c260"))
	var strip := ColorRect.new()
	strip.color = Color(0, 0, 0, 0.55)
	strip.position = Vector2(0, 512)
	strip.size = Vector2(960, 28)
	cl.add_child(strip)
	for l in range(9):
		var lb := _label(cl, Vector2(0, 516), 11, Color("#a0a08b"))
		lb.size = Vector2(190, 16)
		lane_labels.append(lb)
	hero = Node2D.new()
	hero_sprite = Game.make_sprite(Game.hero, 4)
	hero.add_child(hero_sprite)
	world.add_child(hero)
	_build_scenery()
	start_wave(Game.wave)


func _label(parent: Node, pos: Vector2, size: int, col: Color) -> Label:
	var l := Label.new()
	l.position = pos
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", col)
	parent.add_child(l)
	return l


func _scenery_sprite(name: String, facing: int, wx: float, wd: float, tint: Color, k: float) -> void:
	var s := Game.make_sprite(name, facing)
	s.set_meta("k", k)
	s.set_meta("wx", wx)
	s.set_meta("wd", wd)
	s.set_meta("tint", tint)
	world.add_child(s)
	scenery.append(s)


func _build_scenery() -> void:
	pass   # the whole army on camera is real units now (see _build_ranks)


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
	lane_types.resize(lanes)
	lane_types.fill(army_type)
	front.resize(lanes)
	front.fill(null)
	swap_pending.resize(lanes)
	swap_pending.fill(false)
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
	tw.tween_property(view, "focal", lerpf(360.0, 310.0, t), 1.5)
	tw.tween_property(view, "cam_h", lerpf(150.0, 175.0, t), 1.5)
	say("Wave %d / 4" % (i + 1))


func _build_ranks(reserves: Dictionary) -> void:
	# waves.json reserves = per-type pools. The block is one type at a time; deploy fills every slot from the pool.
	pool = reserves.duplicate()
	var half := (lanes - 1) / 2.0 * LANE_W + 180.0
	slots = Army.slots(half, RANK_ROWS, RANK_D0, _rng)
	_deploy(army_type, false)


## Fill every empty rank slot with `type` from its pool. `rush`: come in from behind the lens at speed.
func _deploy(type: String, rush: bool) -> void:
	var taken := {}
	for u in units:
		if u.team == Unit.ALLY and not u.dead and u.state != Unit.State.RETREAT:
			taken[u.home] = true
	for sl in slots:
		if taken.has(sl) or int(pool.get(type, 0)) <= 0:
			continue
		pool[type] = int(pool[type]) - 1
		var u := spawn(type, Unit.ALLY)
		u.home = sl
		u.hold_d = sl.y
		if rush:
			u.wx = sl.x + _rng.randf_range(-6, 6)
			u.wd = BEHIND_D
			u.rush = true
			u.state = Unit.State.ADVANCE
		else:
			u.wx = sl.x
			u.wd = sl.y
			u.state = Unit.State.RANK


## A retreating unit made it behind the lens: back into the pool, off the field.
func recall(u: Unit) -> void:
	units.erase(u)
	pool[u.type] = int(pool.get(u.type, 0)) + 1
	u.dead = true
	u.queue_free()


## A slot opened in the ranks: the nearest unit behind it (toward the camera) steps up, and so on down the file.
func _shuffle_up(slot: Vector2) -> void:
	for _i in range(Army.RANK_X as int):   # bounded chain
		var best: Unit = null
		var best_d := 60.0
		for o in units:
			if o.team == Unit.ALLY and o.state == Unit.State.RANK and o.home.y < slot.y - 4.0:
				var d := absf(o.home.x - slot.x) + (slot.y - o.home.y) * 0.8
				if d < best_d:
					best_d = d
					best = o
		if best == null:
			return
		var vacated := best.home
		best.home = slot
		slot = vacated


func lane_x(l: int) -> float:
	return (l - (lanes - 1) / 2.0) * LANE_W


func _rank_unit(type: String, l: int) -> Unit:
	var best: Unit = null
	var best_d := INF
	for u in units:
		if u.team == Unit.ALLY and u.type == type and u.lane < 0 and (u.state == Unit.State.RANK or u.state == Unit.State.ADVANCE):
			var d := absf(u.home.x - lane_x(l)) + (RANK_D0 - u.home.y) * 2.0
			if d < best_d:
				best_d = d
				best = u
	return best


func lane_open(l: int) -> bool:
	return front[l] == null and _rank_unit(lane_types[l], l) == null


func find_target(u: Unit) -> Node2D:
	var best: Node2D = null
	var best_d := INF
	var here := Vector2(u.wx, u.wd)
	for o in units:
		if not is_instance_valid(o) or o.dead or o.team == u.team or o.state == Unit.State.RETREAT:
			continue
		var d := here.distance_to(Vector2(o.wx, o.wd))
		if o.state == Unit.State.RANK and d > 70.0:
			continue          # ranks only matter once you're on top of them
		if u.team == Unit.ALLY and d > u.rng + 50.0:
			continue          # the line holds; it doesn't chase
		if d < best_d:
			best_d = d
			best = o
	if best == null and u.team == Unit.ENEMY and u.wd < FRONT_D + 60.0:
		return hero        # through the line with nobody left to stop it
	return best


func hit(a: Unit, t: Node2D) -> void:
	var d := a.dmg
	if t is Unit and t.type in a.counters:
		d *= float(Game.units["counter_mult"])
	if a.team == Unit.ALLY:
		d *= 1.0 + Game.relic_bonus("dmg")
	var to := t.position - a.position
	if a.rng > 100.0:
		_arrow(a.position + Vector2(0, -50 * a.scale.y), t.position + Vector2(0, -30 * t.scale.y))
		await get_tree().create_timer(0.12).timeout
		if not is_instance_valid(t):
			return
	else:
		a.lunge(to.normalized() * 14.0 * a.scale.x)
	_impact(t.position + Vector2(0, -34 * t.scale.y), 6.0 * t.scale.y)
	if t is Unit:
		if t.guard_until > _now():
			d *= 0.3
		t.take(d, to.normalized() * 6.0)
	elif t == hero:
		hero_hp -= d
		hero_sprite.modulate = Color(2, 1, 1)


func _impact(p: Vector2, r: float) -> void:
	var n := Node2D.new()
	n.position = p
	n.draw.connect(func():
		n.draw_circle(Vector2.ZERO, r, Color(1.0, 0.95, 0.7))
		for i in range(5):
			var v := Vector2.RIGHT.rotated(i * TAU / 5.0 + _rng.randf() * 0.6) * r * 2.4
			n.draw_line(v * 0.4, v, Color(1.0, 0.9, 0.5), 1.5))
	fx.add_child(n)
	var tw := create_tween()
	tw.tween_property(n, "modulate:a", 0.0, 0.18)
	tw.tween_callback(n.queue_free)


func _arrow(from: Vector2, to: Vector2) -> void:
	var l := Line2D.new()
	l.width = 1.5
	l.default_color = Color(0.9, 0.85, 0.7)
	l.add_point(from)
	l.add_point(from)
	fx.add_child(l)
	var tw := create_tween()
	tw.tween_method(func(k: float):
		var mid := from.lerp(to, k)
		mid.y -= sin(k * PI) * from.distance_to(to) * 0.12
		l.set_point_position(1, mid)
		l.set_point_position(0, from.lerp(mid, 0.7)), 0.0, 1.0, 0.12)
	tw.tween_property(l, "modulate:a", 0.0, 0.08)
	tw.tween_callback(l.queue_free)


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
	u.wx = _rng.randf_range(lane_x(0) - 60.0, lane_x(lanes - 1) + 60.0)
	u.wd = SPAWN_D + _rng.randf_range(0, 160)
	u.hold_d = FRONT_D + 40.0 + _rng.randf_range(-6, 22)


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


func _cell(wx: float, wd: float) -> Vector2i:
	return Vector2i(int(floor(wx / CELL)), int(floor(wd / CELL)))


func _rebuild_grid() -> void:
	_grid.clear()
	for u in units:
		if not is_instance_valid(u) or u.dead:
			continue
		var c := _cell(u.wx, u.wd)
		if not _grid.has(c):
			_grid[c] = []
		_grid[c].append(u)


func _around(wx: float, wd: float) -> Array:
	var out := []
	var c := _cell(wx, wd)
	for dx in range(-1, 2):
		for dy in range(-1, 2):
			var arr = _grid.get(Vector2i(c.x + dx, c.y + dy))
			if arr:
				out.append_array(arr)
	return out


## Shove away from neighbours closer than a body width, and away from the hero pushing through.
func separation(u: Unit) -> Vector2:
	var here := Vector2(u.wx, u.wd)
	var push := Vector2.ZERO
	for o in _around(u.wx, u.wd):
		if o == u or o.team != u.team:
			continue
		var d := here - Vector2(o.wx, o.wd)
		var l := d.length()
		if l < 24.0 and l > 0.01:
			push += d / l * (24.0 - l) * 6.0
	var hd := here - Vector2(hero_wx, hero_wd)
	var hl := hd.length()
	if hl < 34.0 and hl > 0.01:
		push += hd / hl * (34.0 - hl) * 14.0
	return push


func near_enemy(u: Unit, radius: float) -> Unit:
	var best: Unit = null
	var best_d := radius
	var here := Vector2(u.wx, u.wd)
	for o in _around(u.wx, u.wd):
		if o.team == u.team:
			continue
		var d := here.distance_to(Vector2(o.wx, o.wd))
		if d < best_d:
			best_d = d
			best = o
	return best


func _process(delta: float) -> void:
	queue_redraw()
	hud.queue_redraw()
	_rebuild_grid()
	toast_t -= delta
	scroll += CREEP * delta
	# scenery re-projects every frame (camera tweens per wave)
	for s in scenery:
		var wd: float = s.get_meta("wd")
		s.position = view.project(s.get_meta("wx"), wd)
		if advancing:
			s.position.y -= absf(sin(scroll * 0.9 + s.get_meta("wx") * 0.05)) * 3.0 * view.s(wd)
		s.scale = Vector2.ONE * view.sprite_scale(wd) * float(s.get_meta("k"))
		s.modulate = (s.get_meta("tint") as Color) * view.fog(wd)
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
				u.opening = swap_pending[l]
				u.rush = u.rush or swap_pending[l]
				if not swap_pending[l]:
					_shuffle_up(u.home)
				swap_pending[l] = false
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
			KEY_Z: _cast("bolt")
			KEY_X: _cast("heal")
			KEY_C: _cast("wall")
			KEY_Q: _cycle_army(-1)
			KEY_E: _cycle_army(1)


func _hover_lane() -> int:
	var x := _cursor_world().x
	return clampi(int(round((x - lane_x(0)) / LANE_W)), 0, lanes - 1)


func _rank_count(type: String) -> int:
	var n := int(pool.get(type, 0))
	for u in units:
		if u.team == Unit.ALLY and u.type == type and not u.dead and u.state != Unit.State.RETREAT:
			n += 1
	return n


func _cycle_army(dir: int) -> void:
	_set_army(TYPES[(TYPES.find(army_type) + dir + TYPES.size()) % TYPES.size()])


## Whole front line cycles out; the new type steps up in every lane and fires its swap-in ability (if off cooldown).
func _set_army(type: String) -> void:
	if type == army_type:
		return
	army_type = type
	for l in range(lanes):
		lane_types[l] = type
		front[l] = null
		swap_pending[l] = true
	for u in units.duplicate():
		if u.team == Unit.ALLY and not u.dead and u.type != type and u.state != Unit.State.RETREAT:
			u.lane = -1
			u.state = Unit.State.RETREAT
			u.rush = true
			u.sprite.frame = 0
	_deploy(type, true)
	var d: Dictionary = Game.units[type]
	var cd := _ability_cd_left(type)
	say("%s step up - %s%s" % [d["label"], d["ability_label"], "" if cd <= 0.0 else " (on cooldown %.0fs)" % cd])


func _ability_cd_left(type: String) -> float:
	return maxf(0.0, float(ability_ready.get(type, 0.0)) - _now())


func _now() -> float:
	return Time.get_ticks_msec() / 1000.0


## A swapped-in unit reached the line: fire its type's opening ability if that type is off cooldown.
func arrived(u: Unit) -> void:
	if _ability_cd_left(u.type) > 0.0:
		return
	ability_ready[u.type] = _now() + float(Game.units[u.type]["ability_cd"])
	_opening(u)
	if Game.has_relic_flag("echo"):
		var tw := create_tween()
		tw.tween_interval(1.5)
		tw.tween_callback(func(): if is_instance_valid(u) and not u.dead: _opening(u))


func _opening(u: Unit) -> void:
	var here := Vector2(u.wx, u.wd)
	var near: Array[Unit] = []
	for o in units:
		if o.team == Unit.ENEMY and absi(o.lane - u.lane) <= 1 and here.distance_to(Vector2(o.wx, o.wd)) < u.rng + 60.0:
			near.append(o)
	near.sort_custom(func(a, b): return here.distance_to(Vector2(a.wx, a.wd)) < here.distance_to(Vector2(b.wx, b.wd)))
	match Game.units[u.type]["ability"]:
		"whirl":
			# every veteran on the field spin-dodges: i-frames live in Unit.take, blades out
			for o in units:
				if o.team == Unit.ALLY and o.type == u.type and not o.dead:
					o.spin_until = _now() + 4.0
					_flash(Vector2(o.wx, o.wd), 22.0, Color(0.7, 0.9, 1.0, 0.4))
		"sweep":
			_flash(here + Vector2(0, 40), 70.0, Color(1.0, 0.9, 0.6, 0.45))
			for o in near:
				o.take(u.dmg * 2.0, Vector2(0, 8))
				o.wd += 35.0
		"slam":
			_flash(here + Vector2(0, 40), 60.0, Color(1.0, 0.8, 0.5, 0.6))
			for o in near:
				o.take(20.0)
				o.rooted_until = _now() + 2.5
		"draw":
			if not near.is_empty():
				u.lunge((Vector2(near[0].wx, near[0].wd) - here).normalized() * 18.0 * u.scale.x)
				_impact(near[0].position + Vector2(0, -34 * near[0].scale.y), 9.0 * near[0].scale.y)
				near[0].take(45.0, Vector2(0, 6))
		"volley":
			for o in units:
				if o.team == Unit.ENEMY and here.distance_to(Vector2(o.wx, o.wd)) < u.rng:
					_arrow(u.position + Vector2(0, -50 * u.scale.y), o.position + Vector2(0, -30 * o.scale.y))
					o.take(12.0)


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
	# bottom strip: the five types, ranks left, ability cooldown; current army type highlighted
	for i in range(lane_labels.size()):
		var lb := lane_labels[i]
		lb.visible = i < TYPES.size()
		if i < TYPES.size():
			var ty: String = TYPES[i]
			var cd := _ability_cd_left(ty)
			lb.text = "%s x%d  %s%s" % [Game.units[ty]["label"], _rank_count(ty), Game.units[ty]["ability_label"], "" if cd <= 0.0 else " %.0fs" % cd]
			lb.position.x = 8 + i * 190
			lb.add_theme_color_override("font_color", Color("#f0c260") if ty == army_type else Color("#a0a08b"))
	hud_text.text = "WAVE %d / 4\nHERO %d\nMAGIC %d  (lifetime %d)\nZ Bolt 8   X Mend 20   C Wall 30    Q/E cycle the army    RMB siphon\nRelics: %s\nFPS %d" % [
		Game.wave + 1, int(hero_hp), int(Game.magic), int(Game.magic_ever), ", ".join(Game.relics), Engine.get_frames_per_second()]
	toast_label.text = toast if toast_t > 0.0 else ""
