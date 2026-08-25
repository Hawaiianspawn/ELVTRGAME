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
const ENEMY_MIN_D := 130.0        # enemies stop here, at the hero's feet: nothing walks behind the lens
const RANK_D0 := 285.0          # first rank behind the front line
const CREEP := 42.0             # sim treadmill: enemy wd drift (Unit.gd) â€” untouched, not a visual dial
const SCROLL_CREEP_BY_WAVE := [84.0, 112.0, 140.0, 170.0]   # visual-only scroll speed (scenery), ~2x CREEP at wave1, ~2x again by wave4
const HALL_HALF := 200.0        # hallway half-width: the walls; everything lives between them
const CURVE_A_BY_WAVE := [0.5, 0.65, 0.85, 1.1]      # per-wave hall curvature (View.curve_a): harder bend each wave
const CURVE_L_BY_WAVE := [460.0, 380.0, 320.0, 270.0]  # per-wave curvature wavelength (View.curve_l): faster turn each wave
const RANK_ROWS := 8            # depth of the company block down to the camera; every slot is a real unit
const RANK_COLS := 6            # files across; the block fills the hall wall to wall
const RANK_HALF := HALL_HALF - 20.0   # half-width of the block in world units
const RANK_STEP := 28.0
const BEHIND_D := 55.0          # just behind the lens: where swaps come from and go to
const SWEEP_W := 240.0          # launch zone half-width, centered on the hero â€” spans the whole field
const SWEEP_LEN := 520.0        # launch zone depth ahead of the hero â€” everything short of fresh spawns
const TYPES := Army.TYPES
const SLASH_ALPHA := 0.2       # slash fx opacity (strike and whirl field)
const SLASH_STOP := 0.2        # hit stop (real seconds) when a whirl vortex mini hit lands
const SLASH_R := 40.0           # slash fx area hit reach in world units
const SLASH_DMG := 0.25         # fraction of dmg per slash mini hit
const SLASH_EVERY := 2          # slash mini hit every Nth clip frame
const VORTEX_HZ := 2.0          # vortex cleave hits per second
const VORTEX_R := 52.0          # cleave reach in world units at k=1
const VORTEX_LIFT := 80.0      # cleave ticks re-pop airborne foes; they never lift off the ground
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
var army_panels: Array[ArmyPanel] = []
var scenery: Array[Sprite2D] = []      # foreground rows + far horde, meta wx/wd
var scroll := 0.0
var scroll_creep := SCROLL_CREEP_BY_WAVE[0]
var wave_t := 0.0                        # seconds into the hall: the push accelerates the longer you're in it
const SCROLL_ACCEL := 0.02               # +2% scroll speed per second, capped at 2.5x (~75 s in)
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
var _swap_at := 0.0                    # next time (s) a swap is accepted: mashing Q/E can't re-arm the hit stop every frame
const SWAP_CD := 0.5


func _ready() -> void:
	view.horizon = 250.0            # lens tilted down a touch: more ground, front line mid-frame
	view.cam_h = 200.0              # higher lens
	view.sprite_k = 2.2             # the 8x6 block fills the frame edge to edge
	texture_repeat = TEXTURE_REPEAT_ENABLED   # wall bricks use u > 1 to scroll along depth (_draw_walls)
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
	for i in range(TYPES.size()):
		var ty: String = TYPES[i]
		var p := ArmyPanel.new(ty, Game.units[ty], 8.0 + i * (ArmyPanel.COMPACT_SIZE.x + 8.0))
		cl.add_child(p)
		army_panels.append(p)
		p.set_selected(ty == army_type, _rng)
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
	scroll_creep = SCROLL_CREEP_BY_WAVE[i]   # necromancer speeds the sweep each wave; scenery-only, sim speed (CREEP) untouched
	wave_t = 0.0
	var ct := create_tween()        # bend hardens and turns faster too, blended in so the hall doesn't snap
	ct.set_parallel(true)
	ct.tween_property(view, "curve_a", CURVE_A_BY_WAVE[i], 2.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	ct.tween_property(view, "curve_l", CURVE_L_BY_WAVE[i], 2.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
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
	var d: Dictionary = Game.units[type]
	var charge_in: bool = rush and str(d.get("ability", "")) == "sweep" and _ability_cd_left(type) <= 0.0
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
			elif charge_in:
				# halberdiers don't march in either: the deploy IS the charge, from behind the lens straight
				# down-range through the ranks, launching everything passed, then back to the slot
				u.wd = BEHIND_D
				u.state = Unit.State.RANK
				u.charge(FRONT_D + float(d.get("pile_d", 60.0)), sl)   # everyone to the same pile line
			else:
				u.wd = BEHIND_D
				u.rush = true
				u.state = Unit.State.ADVANCE
		else:
			u.wx = sl.x
			u.wd = sl.y
			u.state = Unit.State.RANK
	if charge_in:
		ability_ready[type] = _now() + float(d["ability_cd"])   # the entrance was the opener: arrived() won't fire it again


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
		if not is_instance_valid(t) or not is_instance_valid(a):
			return   # either side can die in flight
	else:
		a.lunge(to.normalized() * 14.0 * a.scale.x)
		if t is Unit:
			_slash(a, t, to)
	_impact(t.position + Vector2(0, -34 * t.scale.y), 6.0 * t.scale.y)
	if t is Unit:
		if t.guard_until > _now():
			d *= 0.3
		t.take(d, to.normalized() * 6.0)
		if a.type == "hammer" and not t.dead:
			# hammers knock up and back: small hop, shoved a step away from the hammer
			t.launch(170.0)
			t.wd += signf(t.wd - a.wd) * 18.0
			_hitstop(0.05, 0.15)   # the knock-up beat
	elif t == hero:
		hero_hp -= d
		hero_sprite.modulate = Color(2, 1, 1)


## Slash fx: an area hit parked on the target. The clip plays once in the world and every frame
## tick cleaves each enemy inside SLASH_R for a mini hit, re-popping airborne ones so crowds
## juggle. The swing itself is the unit's own attack clip. units.json "slash": fx_<name>,
## "slash_y": height above the target's feet.
func _slash(a: Unit, t: Unit, to: Vector2) -> void:
	var kind: String = Game.units[a.type].get("slash", "")
	if kind == "" or not Game.sprites.has("fx_" + kind):
		return
	var wp := Vector2(t.wx, t.wd)
	var n := Node2D.new()
	n.position = view.project(wp.x, wp.y)
	n.scale = Vector2.ONE * view.sprite_scale(wp.y)
	var s := Game.make_fx("fx_" + kind)
	s.position = Vector2(0, float(Game.units[a.type].get("slash_y", -40.0)))
	s.scale = Vector2.ONE * 0.9 * 64.0 / float(Game.sprites["fx_" + kind]["cell"])
	s.modulate.a = SLASH_ALPHA
	s.flip_h = to.x < 0.0
	n.add_child(s)
	world.add_child(n)
	var frames := s.hframes
	var dmg := a.dmg * SLASH_DMG
	var team := a.team   # by value: the clip outlives the attacker when it dies mid-swing
	var tid := t.get_instance_id()
	var tw := n.create_tween()
	for f in range(frames):
		tw.tween_callback(func():
			s.frame = f
			if f % SLASH_EVERY != 0:
				return
			for o in units:
				if o.get_instance_id() != tid and o.team != team and not o.dead and Vector2(o.wx, o.wd).distance_to(wp) < SLASH_R:
					_impact(o.position + Vector2(0, -34 * o.scale.y), 4.0 * o.scale.y)
					if o.air_h > 0.0:
						o.launch(VORTEX_LIFT)
					o.take(dmg, to.normalized() * 3.0))
		tw.tween_interval(0.05)
	tw.tween_callback(n.queue_free)


## Vortex cleave: the blue sweep clip parked in the world, spinning in place and cleaving every
## enemy inside its radius twice a second for a few seconds. Hovers at head height so the
## circle isn't wasted on ground nobody stands on. k scales size and reach together.
func _vortex(wp: Vector2, k: float, dmg: float, secs := 2.0, fx := "fx_sweep_thin") -> void:
	var n := Node2D.new()
	n.set_meta("vortex", true)
	n.position = view.project(wp.x, wp.y)
	n.scale = Vector2.ONE * view.sprite_scale(wp.y) * k
	var s := Game.make_fx(fx)
	s.position = Vector2(0, -48.0 / k)     # head height stays put as the circle grows
	s.scale = Vector2.ONE * 64.0 / float(Game.sprites[fx]["cell"])   # hi-res clips shrink back: same size, thinner lines
	s.modulate.a = SLASH_ALPHA
	if fx == "fx_sweep_thin":
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
				o.take(dmg)
				_hitstop(SLASH_STOP))
	var life := n.create_tween()
	life.tween_interval(secs)
	life.tween_property(n, "modulate:a", 0.0, 0.2)
	life.tween_callback(n.queue_free)


## Hit stop: the world crawls for secs of real time. A later, longer stop extends the hold;
## the timer that fired last always restores full speed.
var _stop_until := 0.0
func _hitstop(secs: float, scale := 0.1) -> void:
	var now := Time.get_ticks_msec() / 1000.0
	if now < _stop_until:
		return   # one at a time: a live stop is never extended, so no caller can pin the clock
	_stop_until = now + secs
	Engine.time_scale = scale
	# no lambda: the timer must restore the clock even if this scene is gone by then
	get_tree().create_timer(secs, true, false, true).timeout.connect(Engine.set_time_scale.bind(1.0))


func _exit_tree() -> void:
	Engine.time_scale = 1.0   # a stop must never outlive the battle


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
	wave_t += delta
	scroll += scroll_creep * minf(1.0 + wave_t * SCROLL_ACCEL, 2.5) * delta
	view.scroll = scroll
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
	if type == army_type or _now() < _swap_at:
		return
	_swap_at = _now() + SWAP_CD
	army_type = type
	for p in army_panels:
		p.set_selected(p.type == type, _rng)
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
	var d: Dictionary = Game.units[type]
	var cd := _ability_cd_left(type)   # before deploy: a charge-in entrance starts the cooldown itself
	_deploy(type, true)
	_hitstop(0.09, 0.05)   # the swap beat: the world holds its breath for a blink
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
		var uid := u.get_instance_id()   # id, not the unit: it may be gone in 1.5s
		var tw := create_tween()
		tw.tween_interval(1.5)
		tw.tween_callback(func():
			var v = instance_from_id(uid)
			if v != null and not v.dead: _opening(v))


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
					var fx := "fx_" + str(Game.units[u.type].get("slash", ""))
					_vortex(Vector2(cx + _rng.randf_range(-30.0, 30.0), cd + _rng.randf_range(-35.0, 35.0)), 2.6, u.dmg * VORTEX_DMG, 4.0,
						fx if Game.sprites.has(fx) else "fx_sweep_thin")
		"sweep":
			# every halberdier on the field bulldozes down-range, piling what it passes on one line, then falls back in
			for o in units:
				if o.team == Unit.ALLY and o.type == u.type and not o.dead:
					o.charge(FRONT_D + float(Game.units[u.type].get("pile_d", 60.0)))
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
			# backup volley: three arrow blocks, one every 1.2s, each thrown at a clump in the kill box
			var tw := create_tween()
			tw.set_loops(3)
			tw.tween_callback(_rain_salvo.bind(6.0))
			tw.tween_interval(1.2)


## One salvo of the backup volley: every archer looses at once and the arrows fill an area around an enemy
## in the kill box (the one with the most neighbours, so the block lands on a clump). Every
## arrow leaves together from the archers' line and fans out over the area. dmg is the per-foot
## salvo total, spread over the salvo.
const VOLLEY_COLS := 6          # block of arrows per salvo, wide x deep
const VOLLEY_ROWS := 5
const VOLLEY_W := 220.0         # area filled per salvo, world units wide x deep
const VOLLEY_D := 600.0
const VOLLEY_BOX_D := 160.0     # firing box depth behind the lens the archers loose from
const VOLLEY_SPEED := 1100.0    # arrow speed, world units/s: flight time follows distance
func _rain_salvo(dmg: float) -> void:
	var x0 := hero_wx
	var d0 := hero_wd + 80.0
	var d1 := hero_wd + 700.0
	var foes: Array = []
	for o in units:
		if o.team == Unit.ENEMY and not o.dead and absf(o.wx - x0) < 180.0 and o.wd > d0 and o.wd < d1:
			foes.append(o)
	var at := Vector2(x0, (d0 + d1) * 0.5)
	if not foes.is_empty():
		var best: Unit = foes[0]
		var best_n := -1
		for o in foes:
			var n := 0
			for q in foes:
				if Vector2(o.wx, o.wd).distance_to(Vector2(q.wx, q.wd)) < VOLLEY_W * 0.5:
					n += 1
			if n > best_n:
				best_n = n
				best = o
		var lead := best.speed * 0.4 if best.wd > FRONT_D + 60.0 else 0.0
		at = Vector2(best.wx, best.wd - lead)
	_flash(at, 80.0, Color(1.0, 0.9, 0.6, 0.25))
	var per := dmg * 18.0 / float(VOLLEY_COLS * VOLLEY_ROWS)
	for j in range(VOLLEY_ROWS):
		for i in range(VOLLEY_COLS):
			# uniform block: archer (i, j) in the firing box shoots mark (i, j) in the landing area, so the
			# formation flies as one body and lands as one. Jitter is a hair, just enough to not read as a grid.
			var u := (i + 0.5) / VOLLEY_COLS - 0.5
			var v := (j + 0.5) / VOLLEY_ROWS - 0.5
			var mark := Vector2(at.x + u * VOLLEY_W + _rng.randf_range(-5.0, 5.0), at.y + v * VOLLEY_D + _rng.randf_range(-8.0, 8.0))
			var from := Vector2(hero_wx + u * RANK_HALF * 1.6 + _rng.randf_range(-4.0, 4.0), BEHIND_D - (v + 0.5) * VOLLEY_BOX_D)
			_volley_arrow(from, mark, per)


## One volley arrow: launched behind the camera above the ranks, arcing down onto `mark`.
## Airborne enemies along the path are run through â€” damaged plus a small juggle tap â€”
## without stopping the arrow; whoever stands at the mark when it lands takes the hit.
func _volley_arrow(from: Vector2, mark: Vector2, dmg: float) -> void:
	# one straight line from the archer's spot to the mark at a shared speed: the block keeps its shape in flight
	var secs := from.distance_to(mark) / VOLLEY_SPEED
	var peak := _rng.randf_range(50.0, 130.0)   # each arrow its own arc height: the block has thickness, not a plane
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
	tw.tween_method(func(k: float):
		var wp := from.lerp(mark, k)
		var h := peak * (1.0 - k * k) + sin(k * PI) * 8.0   # in from over the lens, above the frame, diving late
		var p := view.project(wp.x, wp.y) + Vector2(0, -h * view.sprite_scale(wp.y))
		l.visible = true
		l.set_point_position(0, p)
		var tail: Vector2 = p if prev[0] == Vector2.INF else prev[0]
		var cap := 16.0 * view.sprite_scale(wp.y)   # streak, scaled with depth
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
		, 0.0, 1.0, secs)
	tw.tween_callback(func():
		for o in units:
			if o.team == Unit.ENEMY and not o.dead and o.air_h == 0.0 and Vector2(o.wx, o.wd).distance_to(mark) < 26.0:
				o.take(dmg)
				_impact(view.project(mark.x, mark.y), 5.0)
				break
		l.queue_free())


## A sky-dropped hammer hit the ground: shockwave â€” same numbers as its slam ability.
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
	# ponytail: procedural ground bands; owner says real terrain lands here later â€” replace this one call.
	view.draw_ground(self, scroll, HALL_HALF)
	_draw_walls()
	# the necromancer's green glow at the far end of the hall
	var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
	var dot := view.project(0.0, view.cam_d + view.fog_end) + Vector2(0, -8)
	draw_circle(dot, 6.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
	draw_line(dot, dot + Vector2(_rng.randf_range(-30, 30), -110), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)


# Wall: wall_14 with its baked top ledge cropped off (rows 12..43) -> 32x32, loops vertically.
const WALL_TEX := preload("res://assets/env/hall/wall_flat.png")
const COURSE_H := 64.0     # world-unit height of one wall course (32px tile drawn 2x so the bricks read)
const MAX_COURSES := 3     # ponytail: cap so 2 sides x 12 bands x 3 courses stays cheap; lower if the probe dips


## Stone walls either side of the hall, near to far, with green sconces every few rows; courses of the
## chosen wall tile stack upward per band until the wall top clears the viewport, then fade to black
## (no baked ceiling edge).
func _draw_walls() -> void:
	var near_d := view.cam_d + 60.0
	var far_d := view.cam_d + view.fog_end * 1.5
	var wall_h := COURSE_H * MAX_COURSES   # sconce height reference; texture courses stack independently above
	var segs := 14     # was 12; the curve needs enough depth bands to read as a curve, not a polyline
	for side: float in [-1.0, 1.0]:
		var x: float = side * HALL_HALF
		for i in range(segs):
			var da := near_d * pow(far_d / near_d, float(i) / segs)
			var db := near_d * pow(far_d / near_d, float(i + 1) / segs)
			var a := view.project(x, da)
			var b := view.project(x, db)
			var f := view.fog(da)
			var sa := view.s(da)
			var sb := view.s(db)
			var top_a := a
			var top_b := b
			var last_a := a
			var last_b := b
			var last_top_a := a
			var last_top_b := b
			# bricks slide toward the camera with the push: u is world depth in brick lengths, so the
			# texture is pinned to the hall, not to the band (texture_repeat is on for this node)
			var ua := (da + scroll) / COURSE_H
			var ub := (db + scroll) / COURSE_H
			var course := 0
			while course < MAX_COURSES and top_a.y > 0.0:
				var bot_a := a - Vector2(0, course * COURSE_H * sa)
				var bot_b := b - Vector2(0, course * COURSE_H * sb)
				top_a = a - Vector2(0, (course + 1) * COURSE_H * sa)
				top_b = b - Vector2(0, (course + 1) * COURSE_H * sb)
				draw_polygon(
					PackedVector2Array([bot_a, bot_b, top_b, top_a]),
					PackedColorArray([f, f, f, f]),
					PackedVector2Array([Vector2(ua, 1), Vector2(ub, 1), Vector2(ub, 0), Vector2(ua, 0)]),
					WALL_TEX)
				last_a = bot_a; last_b = bot_b; last_top_a = top_a; last_top_b = top_b
				course += 1
			# fade the topmost course's upper 40% to solid black so no ceiling edge exists
			var mid_a := last_a.lerp(last_top_a, 0.6)
			var mid_b := last_b.lerp(last_top_b, 0.6)
			draw_polygon(
				PackedVector2Array([mid_a, mid_b, last_top_b, last_top_a]),
				PackedColorArray([Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color.BLACK, Color.BLACK]))
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
	# bottom strip: the four army panels, ranks left, ability cooldown; current army type large + on top
	for p in army_panels:
		p.update(_rank_count(p.type), _ability_cd_left(p.type))
	hud_text.text = "HALL %d / 4\nHERO %d\nMAGIC %d  (lifetime %d)\nSCORE %d\nZ Bolt 8   X Mend 20   C Wall 30    Q/E cycle the army    RMB siphon\nRelics: %s\nFPS %d" % [
		Game.wave + 1, int(hero_hp), int(Game.magic), int(Game.magic_ever), Game.score, ", ".join(Game.relics), Engine.get_frames_per_second()]
	toast_label.text = toast if toast_t > 0.0 else ""
