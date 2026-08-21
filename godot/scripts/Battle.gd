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
const HALL_HALF := 200.0        # hallway half-width: the walls; everything lives between them
const RANK_ROWS := 8            # depth of the company block down to the camera; every slot is a real unit
const RANK_COLS := 6            # files across; the block fills the hall wall to wall
const RANK_HALF := HALL_HALF - 20.0   # half-width of the block in world units
const RANK_STEP := 28.0
const BEHIND_D := 55.0          # just behind the lens: where swaps come from and go to
const SWEEP_W := 240.0          # launch zone half-width, centered on the hero — spans the whole field
const SWEEP_LEN := 520.0        # launch zone depth ahead of the hero — everything short of fresh spawns
const TYPES := Army.TYPES
const VORTEX_HZ := 2.0          # vortex cleave hits per second
const VORTEX_R := 52.0          # cleave reach in world units at k=1
const VORTEX_LIFT := 120.0      # cleave ticks re-pop airborne foes; they never lift off the ground
const VORTEX_AHEAD := 70.0      # per-strike vortex parks this far past the target, down the hall
const VORTEX_DMG := 0.35        # fraction of the veteran's dmg per cleave tick
const VORTEX_COLS := 4          # whirl: vortex field ahead of the line, cols x rows
const VORTEX_ROWS := 2

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
	view.horizon = 250.0            # lens tilted down a touch: more ground, front line mid-frame
	view.cam_h = 200.0              # higher lens
	view.sprite_k = 2.2             # the 8x6 block fills the frame edge to edge
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
	lanes = mini(int(w["lanes"]), int(HALL_HALF * 2.0 / LANE_W))   # never wider than the hall
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
	say("Hall %d / 4" % (i + 1))


func _build_ranks(reserves: Dictionary) -> void:
	# waves.json reserves = per-type pools. The block is one type at a time; deploy fills every slot from the pool.
	pool = reserves.duplicate()
	slots = Army.slots(RANK_HALF, RANK_ROWS, RANK_D0, _rng, RANK_COLS, RANK_STEP)
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
			if type == "hammer":
				# hammers don't march in: they drop out of the sky onto their slot and slam
				u.wd = sl.y
				u.air_h = _rng.randf_range(220.0, 320.0)
				u.air_v = -280.0
				u.sky_slam = true
				u.state = Unit.State.RANK
			else:
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
	# grid query, not a scan of every unit: the line doesn't chase, and an enemy
	# beyond ~3 cells of anyone just marches on. Keeps a 2000-unit hall off the n^2 cliff.
	var reach := u.rng + 50.0 if u.team == Unit.ALLY else 150.0
	for o in _around(u.wx, u.wd, maxi(1, int(ceil(reach / CELL)))):
		if o.team == u.team or o.state == Unit.State.RETREAT:
			continue
		var d := here.distance_to(Vector2(o.wx, o.wd))
		if o.state == Unit.State.RANK and d > 70.0:
			continue          # ranks only matter once you're on top of them
		if u.team == Unit.ALLY and d > u.rng + 50.0:
			continue          # the line holds; it doesn't chase
		if u.team == Unit.ALLY and o.air_h > 0.0:
			d -= 999.0        # juggle focus: an airborne foe in reach beats any grounded one
		if d < best_d:
			best_d = d
			best = o
	if best == null and u.team == Unit.ENEMY and u.wd < FRONT_D + 60.0:
		return hero        # through the line with nobody left to stop it
	return best


func hit(a: Unit, t: Node2D, mult := 1.0) -> void:
	var d := a.dmg * mult
	if t is Unit and t.type in a.counters:
		d *= float(Game.units["counter_mult"])
	if a.team == Unit.ALLY:
		d *= 1.0 + Game.relic_bonus("dmg")
	var to := t.position - a.position
	a.attack_anim()
	if a.rng > 100.0:
		_arrow(a.position + Vector2(0, -50 * a.scale.y), t.position + Vector2(0, -30 * t.scale.y))
		await get_tree().create_timer(0.12).timeout
		if not is_instance_valid(t):
			return
	elif t is Unit and Game.units[a.type].get("ability", "") == "whirl":
		# veterans don't swing: every strike parks a vortex cleave over the target's head —
		# narrowed when the fight is toe to toe so it doesn't swallow the rank
		a.lunge(to.normalized() * 14.0 * a.scale.x)
		var ahead := 0.0 if _dist(a, t) < 45.0 else VORTEX_AHEAD
		_vortex(Vector2(t.wx, t.wd + ahead), 1.0 if ahead == 0.0 else 2.2, a.dmg * VORTEX_DMG)
	else:
		a.lunge(to.normalized() * 14.0 * a.scale.x)
		_slash(a, to)
	_impact(t.position + Vector2(0, -34 * t.scale.y), 6.0 * t.scale.y)
	if t is Unit:
		if t.guard_until > _now():
			d *= 0.3
		t.take(d, to.normalized() * 6.0)
	elif t == hero:
		hero_hp -= d
		hero_sprite.modulate = Color(2, 1, 1)


## Pixel slash clip over the attacker's head, so a strike reads from behind the front line where the
## sprite's own arm is hidden by the rank in front. units.json "slash": sweep | chop | thrust -> fx_<name>.
func _slash(a: Unit, to: Vector2) -> void:
	var kind: String = Game.units[a.type].get("slash", "")
	if kind == "" or not Game.sprites.has("fx_" + kind):
		return
	# ride the attacker so it y-sorts with the ranks instead of floating over the whole field
	var s := Game.make_fx("fx_" + kind)
	s.position = Vector2(0, float(Game.units[a.type].get("slash_y", -40.0))) + to.normalized() * 8.0
	s.scale = Vector2.ONE * 0.9
	s.flip_h = to.x < 0.0
	a.add_child(s)
	var n := s.hframes
	var tw := s.create_tween()
	tw.tween_property(s, "frame", n - 1, 0.05 * n)
	tw.tween_callback(s.queue_free)


## Vortex cleave: the blue sweep clip parked in the world, spinning in place and cleaving every
## enemy inside its radius twice a second for a few seconds. Hovers at head height so the
## circle isn't wasted on ground nobody stands on. k scales size and reach together.
func _vortex(wp: Vector2, k: float, dmg: float, secs := 2.0) -> void:
	var n := Node2D.new()
	n.set_meta("vortex", true)
	n.position = view.project(wp.x, wp.y)
	n.scale = Vector2.ONE * view.sprite_scale(wp.y) * k
	var s := Game.make_fx("fx_sweep_thin")
	s.scale = Vector2.ONE * 0.25           # clip is 4x upscaled so the ring stays a thin line
	s.position = Vector2(0, -48.0 / k)     # head height stays put as the circle grows
	s.modulate = Color(3.0, 3.0, 3.0, 0.95)   # blown out near white; the blue only survives at the edges
	n.add_child(s)
	world.add_child(n)
	# never in lockstep: random facing, random spin direction, own phase and cadence
	s.rotation = _rng.randf() * TAU
	s.flip_h = _rng.randf() < 0.5
	var phase := _rng.randf()
	var spin := n.create_tween().set_loops()
	spin.tween_method(func(t: float): s.frame = wrapi(int((t + phase) * s.hframes), 0, s.hframes), 0.0, 1.0, _rng.randf_range(0.32, 0.5))
	var beat := n.create_tween().set_loops(int(secs * VORTEX_HZ))
	beat.tween_interval(1.0 / VORTEX_HZ)
	beat.tween_callback(func():
		for o in units:
			if o.team == Unit.ENEMY and not o.dead and Vector2(o.wx, o.wd).distance_to(wp) < VORTEX_R * k:
				_impact(o.position + Vector2(0, -34 * o.scale.y), 4.0 * o.scale.y)
				if o.air_h > 0.0:
					o.launch(VORTEX_LIFT)   # already up: keep it up. Lifting off the ground is the halberdiers' job
				o.take(dmg))
	var life := n.create_tween()
	life.tween_interval(secs)
	life.tween_property(n, "modulate:a", 0.0, 0.2)
	life.tween_callback(n.queue_free)


func _dist(a: Unit, b: Unit) -> float:
	return Vector2(a.wx, a.wd).distance_to(Vector2(b.wx, b.wd))


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
	u.wx = _rng.randf_range(-HALL_HALF + 20.0, HALL_HALF - 20.0)
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


func _around(wx: float, wd: float, r: int = 1) -> Array:
	var out := []
	var c := _cell(wx, wd)
	for dx in range(-r, r + 1):
		for dy in range(-r, r + 1):
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
	var best_score := radius
	var here := Vector2(u.wx, u.wd)
	for o in _around(u.wx, u.wd, maxi(1, int(ceil(radius / CELL)))):
		if o.team == u.team:
			continue
		var d := here.distance_to(Vector2(o.wx, o.wd))
		if d >= radius:
			continue
		# juggle focus: airborne foes in reach always take the hit first
		var score := d - (999.0 if o.air_h > 0.0 else 0.0)
		if score < best_score:
			best_score = score
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
		# bursts: the horde arrives in clumps, not a drip
		for i in range(mini(3, spawn_queue.size())):
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
	hero_wx = clampf(hero_wx + mv.x * 130.0 * delta, -HALL_HALF + 30.0, HALL_HALF - 30.0)
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
		say("The throne room door. It is already open.\nTO BE CONTINUED")
		await get_tree().create_timer(4.0).timeout
		get_tree().change_scene_to_file("res://scenes/Main.tscn")
	else:
		await _turn(str(Game.waves[Game.wave].get("turn", "")))
		_done = false
		start_wave(Game.wave + 1)


## The hall bends between waves: swing the lens left/right, or tilt it up a stair, then settle.
func _turn(kind: String) -> void:
	var t := create_tween()
	match kind:
		"left", "right":
			say("The hall turns %s." % kind)
			var dx := -160.0 if kind == "left" else 160.0
			t.tween_property(view, "cam_x", dx, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
			t.tween_property(view, "cam_x", 0.0, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		"stairs":
			say("Stairs. Up.")
			t.tween_property(view, "horizon", 170.0, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
			t.tween_property(view, "horizon", 250.0, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		_:
			say("Hall cleared. Magic siphoned: %d" % int(Game.magic))
			t.tween_interval(2.5)
	await t.finished


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
	# the swap beat: the world holds its breath for a blink
	Engine.time_scale = 0.05
	get_tree().create_timer(0.09, true, false, true).timeout.connect(func(): Engine.time_scale = 1.0)
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
			# every veteran on the field spin-dodges: i-frames live in Unit.take, blades out.
			# alternate the beat so half spin while the other half plants, in and out
			var flip := 0.0
			for o in units:
				if o.team == Unit.ALLY and o.type == u.type and not o.dead:
					o.spin_until = _now() + 4.0
					o.spin_phase = flip
					flip = 1.0 - flip
					_flash(Vector2(o.wx, o.wd), 22.0, Color(0.7, 0.9, 1.0, 0.4))
			# and a field of vortex cleaves ahead of the line: jittered grid = random but even
			for i in range(VORTEX_COLS):
				for j in range(VORTEX_ROWS):
					var cx := -RANK_HALF + RANK_HALF * 2.0 * (i + 0.5) / VORTEX_COLS
					var cd := FRONT_D + 90.0 + 140.0 * j
					_vortex(Vector2(cx + _rng.randf_range(-30.0, 30.0), cd + _rng.randf_range(-35.0, 35.0)), 2.6, u.dmg * VORTEX_DMG, 4.0)
		"sweep":
			# every halberdier on the field charges down-range, launching all it passes, then falls back in
			for o in units:
				if o.team == Unit.ALLY and o.type == u.type and not o.dead:
					o.charge(o.wd + float(Game.units[u.type].get("charge_len", SWEEP_LEN * 0.5)))   # upgrade stat
		"slam":
			u.slam_anim()
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
			# backup volley: one salvo a second slams the kill box in front of the hero
			var tw := create_tween()
			tw.set_loops(4)
			tw.tween_callback(_rain_salvo.bind(6.0))
			tw.tween_interval(1.0)


## One salvo of the backup volley: 18 arrows at once into a box ahead of the hero — most
## seek a random enemy inside it (led if still marching), the rest hammer its ground.
func _rain_salvo(dmg: float) -> void:
	var x0 := hero_wx
	var d0 := hero_wd + 80.0
	var d1 := hero_wd + 320.0
	_flash(Vector2(x0, (d0 + d1) * 0.5), 100.0, Color(1.0, 0.9, 0.6, 0.25))
	var foes: Array = []
	for o in units:
		if o.team == Unit.ENEMY and not o.dead and absf(o.wx - x0) < 130.0 and o.wd > d0 and o.wd < d1:
			foes.append(o)
	for i in range(18):
		var mark: Vector2
		if not foes.is_empty() and _rng.randf() < 0.6:
			var o: Unit = foes[_rng.randi() % foes.size()]
			var lead := o.speed * 0.5 if o.wd > FRONT_D + 60.0 else 0.0
			mark = Vector2(o.wx + _rng.randf_range(-16.0, 16.0), o.wd - lead + _rng.randf_range(-12.0, 12.0))
		else:
			mark = Vector2(x0 + _rng.randf_range(-120.0, 120.0), _rng.randf_range(d0, d1))
		_volley_arrow(mark, dmg)


## One volley arrow: launched behind the camera above the ranks, arcing down onto `mark`.
## Airborne enemies along the path are run through — damaged plus a small juggle tap —
## without stopping the arrow; whoever stands at the mark when it lands takes the hit.
func _volley_arrow(mark: Vector2, dmg: float) -> void:
	var from := Vector2(mark.x + _rng.randf_range(-6.0, 6.0), BEHIND_D)   # straight down-range: lateral jitter at wd 55 reads as sideways flight
	var l := Line2D.new()
	l.width = 1.5
	l.default_color = Color(0.9, 0.85, 0.7)
	l.add_point(Vector2.ZERO)
	l.add_point(Vector2.ZERO)
	l.visible = false
	fx.add_child(l)
	var pierced := {}
	var prev := [Vector2.INF]
	var tw := create_tween()
	tw.tween_interval(_rng.randf_range(0.0, 0.05))   # a breath of jitter so the volley shimmers but lands as one
	tw.tween_method(func(k: float):
		var wp := from.lerp(mark, k)
		var h := 90.0 * (1.0 - k * k) + sin(k * PI) * 8.0   # in from over the lens, above the frame, diving late
		var p := view.project(wp.x, wp.y) + Vector2(0, -h * view.sprite_scale(wp.y))
		l.visible = true
		l.set_point_position(0, p)
		var tail: Vector2 = p if prev[0] == Vector2.INF else prev[0]
		var cap := 9.0 * view.sprite_scale(wp.y)   # short dart, scaled with depth
		if p.distance_to(tail) > cap:
			tail = p + (tail - p).normalized() * cap
		l.set_point_position(1, tail)
		prev[0] = p
		for o in units:
			if o.team == Unit.ENEMY and not o.dead and o.air_h > 0.0 and not pierced.has(o) \
					and absf(o.wx - wp.x) < 20.0 and absf(o.wd - wp.y) < 28.0 and absf(o.air_h - h) < 45.0:
				pierced[o] = true
				o.take(dmg)
				o.air_v = 130.0   # a tap back up, not a full re-launch
		, 0.0, 1.0, 0.4)
	tw.tween_callback(func():
		for o in units:
			if o.team == Unit.ENEMY and not o.dead and o.air_h == 0.0 and Vector2(o.wx, o.wd).distance_to(mark) < 26.0:
				o.take(dmg)
				break
		_impact(view.project(mark.x, mark.y), 5.0)
		l.queue_free())


## A sky-dropped hammer hit the ground: shockwave — same numbers as its slam ability.
func sky_landing(u: Unit) -> void:
	var here := Vector2(u.wx, u.wd)
	_flash(here, 60.0, Color(1.0, 0.8, 0.5, 0.6))
	for o in units:
		if o.team == Unit.ENEMY and here.distance_to(Vector2(o.wx, o.wd)) < 90.0:
			o.take(20.0)
			o.rooted_until = _now() + 2.5


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
	view.draw_ground(self, scroll, HALL_HALF)
	_draw_walls()
	# the necromancer's green glow at the far end of the hall
	var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
	var dot := view.project(0.0, view.cam_d + view.fog_end) + Vector2(0, -8)
	draw_circle(dot, 6.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
	draw_line(dot, dot + Vector2(_rng.randf_range(-30, 30), -110), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)


## Stone walls either side of the hall, near to far, with green sconces every few rows.
func _draw_walls() -> void:
	var near_d := view.cam_d + 60.0
	var far_d := view.cam_d + view.fog_end * 1.5
	var wall_h := 170.0
	for side: float in [-1.0, 1.0]:
		var x: float = side * HALL_HALF
		var out := Vector2(side * 3000.0, 0)
		# face in depth bands so it fogs like the floor; ceiling shelf beyond the top edge
		var segs := 12
		for i in range(segs):
			var da := near_d * pow(far_d / near_d, float(i) / segs)
			var db := near_d * pow(far_d / near_d, float(i + 1) / segs)
			var a := view.project(x, da)
			var b := view.project(x, db)
			var top_a := a - Vector2(0, wall_h * view.s(da))
			var top_b := b - Vector2(0, wall_h * view.s(db))
			var f := view.fog(da)
			draw_colored_polygon(PackedVector2Array([a, b, top_b, top_a]), Color("#3a3a40") * f)
			draw_colored_polygon(PackedVector2Array([top_a, top_b, top_b + out, top_a + out]), Color("#1c1c20") * f)
		draw_line(view.project(x, near_d), view.project(x, far_d), Color(0, 0, 0, 0.5), 2.0)
		# sconces march toward the camera with the push
		var d := near_d + fposmod(-scroll, 160.0)
		while d < far_d:
			var p := view.project(x, d) - Vector2(0, wall_h * 0.6 * view.s(d))
			var k := view.s(d)
			var f := view.fog(d)
			draw_circle(p, 5.0 * k, Color(0.35, 0.95, 0.45, 0.85) * f)
			draw_circle(p, 12.0 * k, Color(0.3, 0.9, 0.4, 0.18) * f)
			d += 160.0


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
	hud_text.text = "HALL %d / 4\nHERO %d\nMAGIC %d  (lifetime %d)\nSCORE %d\nZ Bolt 8   X Mend 20   C Wall 30    Q/E cycle the army    RMB siphon\nRelics: %s\nFPS %d" % [
		Game.wave + 1, int(hero_hp), int(Game.magic), int(Game.magic_ever), Game.score, ", ".join(Game.relics), Engine.get_frames_per_second()]
	toast_label.text = toast if toast_t > 0.0 else ""
