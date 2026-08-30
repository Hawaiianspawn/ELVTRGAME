extends Node3D
## Four waves seen from behind the army. World space: x lateral, d depth ahead of the camera; a
## point stands at Vector3(x, h, -d) and a fixed Camera3D at (cam_x, CAM_H, 0) looks down -Z
## (Hall3D holds the geometry and the projection helpers). Sprites are depth-tested Sprite3D
## billboards, so the crowd sorts per pixel and nothing y-sorts.
## Front line holds at FRONT_D; reserves stand in packed ranks behind it and step forward into the
## lane whose front unit died, taking the type the player set for that lane. Hero walks the gap
## between ranks and line; magic is credited on kills, spends it on spells; relics unlock on lifetime magic.

const FRONT_D := 320.0
const SPAWN_D := 1050.0
const LANE_W := 64.0
const HERO_MIN_D := 150.0
const HERO_MAX_D := 290.0
const ENEMY_MIN_D := 130.0        # enemies stop here, at the hero's feet: nothing walks behind the lens
const RANK_D0 := 285.0          # first rank behind the front line
const CREEP := 42.0             # sim treadmill: enemy wd drift (Unit.gd) - untouched, not a visual dial
const SCROLL_CREEP_BY_WAVE := [84.0, 112.0, 140.0, 170.0]   # visual-only scroll speed (scenery), ~2x CREEP at wave1, ~2x again by wave4
const CURVE_A_BY_WAVE := [0.05, 0.15, 0.35, 0.48]     # OutRun bend amplitude per wave (Hall3D.curve_a), render-only: capped so the bend never carries a wall off screen (docs/qa/walls.md)
const CURVE_L_BY_WAVE := [600.0, 420.0, 320.0, 240.0]   # bend wavelength divisor per wave: shorter = turns quicker
const HALF_BY_WAVE := [520.0, 400.0, 280.0, 200.0]   # hallway half-width per hall: wide past the gate, narrows to today's width by hall 4
var HALL_HALF := HALF_BY_WAVE[0]   # live half-width for the current hall; start_wave sets it and rebuilds the hall/lamps to match
const RANK_ROWS := 8            # depth of the company block down to the camera; every slot is a real unit
const RANK_COLS := 6            # files across; the block fills the hall wall to wall
const RANK_HALF := 180.0        # half-width of the rank block: fixed, does NOT widen with the hall
const RANK_STEP := 28.0
const BEHIND_D := 55.0          # just behind the lens: where swaps come from and go to
const CHARGE_GAP := 56.0        # depth between rank rows on a halberdier charge-in: 2x RANK_STEP so the files read apart
const SWEEP_W := 240.0          # launch zone half-width, centered on the hero - spans the whole field
const SWEEP_LEN := 520.0        # launch zone depth ahead of the hero - everything short of fresh spawns
const TYPES := Army.TYPES
const LAMP_SHADER := preload("res://assets/shaders/prop_lamp.gdshader")   # wall lamps fade to black toward the top (task-169)
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

# The lens. FOCAL/HALF_H reproduce the old pinhole exactly: vertical fov 2*atan(270/360) on a
# 540-tall viewport puts 360 screen px on one world unit at depth 1, and a horizon of h means a
# pitch of -atan((270 - h) / 360).
const CAM_H := 200.0
const FOCAL := 360.0
const HALF_H := 270.0
const HORIZON := 250.0          # resting horizon; _turn tilts to 170 for the stairs and back

var camera: Camera3D
var hall: Hall3D
var lanes := 5
var lane_types: Array[String] = []
var army_type := "veteran"
var ability_ready: Dictionary = {}     # type -> time (s) the swap-in ability is ready again
var front: Array = []             # Unit or null per lane
var spawn_queue: Array[String] = []
var spawn_t := 0.0
var spawn_interval := 1.5
var units: Array[Unit] = []
var hero: Node3D
var hero_sprite: Sprite3D
var _hero_rot: Rect2                   # hero_sprite's rotation region; clips (attack/hurt/walk) swap it out
var _hero_clip := {}
var _orb: Sprite3D                     # magic wisp over the hero (fx_orb), null when the row isn't packed
var _hero_t := -1.0                    # time into the playing one-shot clip, <0 = none
var hero_wx := 0.0
var hero_wd := 245.0
var hero_hp := 100.0
var tank_max_hp := 100.0
var _gatling_cd := 0.0
var gatling_rate_mult := 1.0
const GATLING_RATE := 12.0      # shots/s at 1.0x
const GATLING_DMG := 4.0
const TANK_SCALE := 0.55        # owner call: the dome hid the crowd at native size, shrink it
const TANK_MOUNT_H := 45.0      # sprite px: hero's feet height on the stone tank's turret roof (~82 tank px * TANK_SCALE)
const TANK_FOOTPRINT_R := 75.0  # world units: dome's on-screen radius at TANK_SCALE, for separation()'s hero shove
var tank_sprite: Sprite3D
var _barrel: Sprite3D                   # cannon neck: yaws at the turret toward the cursor's ground point
var _barrel_yaw := PI / 2.0             # ground-plane aim, radians: 0 = +wx (screen right), PI/2 = deeper down the hall
const BARREL_TEX := preload("res://assets/sprites/tank_barrel.png")   # 34x14, cut from the tank's west frame, points +x
const BARREL_LEN := 34.0                # sprite px
const BARREL_SCALE := 0.8               # not TANK_SCALE: at 0.55 the rider's base swallows it whole
var _cannon_cd := 0.0
var cannon_reload_mult := 1.0
var cannon_radius_mult := 1.0
const CANNON_CD := 5.0   # a reward beat, not a second gun — gatling does the steady damage
const CANNON_DMG := 40.0
const CANNON_RADIUS := 200.0
const CANNON_FLIGHT := 0.35
const EXPLOSION_FX := "fx_explosion"    # 12-frame 96px shell burst (fx_library.py explosion); swap to fx_explosion_pl to try the PixelLab take
const SHAKE_AMP := 14.0                 # world units of camera h/v offset at the first frame of a cannon shake (~10 px at the gate)
var _shake_t := 0.0
var _shake_dur := 0.4
var _shake_amp := 0.0
var _flash_rect: ColorRect              # full-screen blast flash on the fx overlay
const GATE_TEX := preload("res://assets/env/castle/gate_open.png")   # 7-frame swing, 96x120/frame
var burst := 3                          # per-wave spawn burst size (waves.json "burst", default 3)
var _up_chests: Array[Dictionary] = []  # upgrade chests: {node, broken, wx, base_d, d, last_d, alive}
var _chest_t := 0.0
const CHEST_INTERVAL := 18.0
const CHEST_POP_R := 30.0
const UPGRADE_CYCLE := ["gatling_rate", "cannon_radius", "cannon_reload", "tank_hp"]
var _upgrade_i := 0
var world: Node3D
var post: Post                         # post-process preset layer, between world/props (1) and HUD text (20)
var env2d: Node2D                      # 2D overlay for props
var _props: Array[Dictionary] = []     # {node, wx, base_d, last_d, alive, is_table}
var _prop_tick := 0
const PROP_BREAK_R := 18.0
var fx: Node2D
var hud: Node2D
var hp_bar: Dictionary                 # Ui.bar: kit housing + fill + label (labelled TANK)
var hall_label: Label
var score_label: Label
var _ui: CanvasLayer                   # HUD layer; win / lose cards land on it
var toast_banner: NinePatchRect
var toast_label: Label
var army_panels: Array[ArmyPanel] = []
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
var toast := ""
var toast_t := 0.0
var _rng := RandomNumberGenerator.new()
var _done := false
var _swap_at := 0.0                    # next time (s) a swap is accepted: mashing Q/E can't re-arm the hit stop every frame
const SWAP_CD := 0.5
var _low_hp_played := false            # hero_sfx.low_hp fires once per wave on crossing below 25


func _ready() -> void:
	_rng.randomize()
	world = Node3D.new()
	add_child(world)
	hall = Hall3D.new()
	world.add_child(hall)
	hall.build(HALL_HALF)
	camera = Camera3D.new()
	camera.position = Vector3(0.0, CAM_H, 0.0)
	camera.fov = rad_to_deg(2.0 * atan(HALF_H / FOCAL))
	camera.keep_aspect = Camera3D.KEEP_HEIGHT
	camera.rotation_degrees.x = pitch_for(HORIZON)
	camera.near = 1.0
	camera.far = 4000.0
	camera.current = true
	add_child(camera)
	# draw-call fx (impacts, arrows, flashes, sconces) stay 2D on an overlay, positioned
	# with camera.unproject_position; only sprites went 3D.
	var ov := CanvasLayer.new()
	ov.layer = 1
	add_child(ov)
	env2d = Node2D.new()
	ov.add_child(env2d)
	fx = Node2D.new()
	fx.z_index = 5
	ov.add_child(fx)
	hud = Node2D.new()
	hud.z_index = 10
	hud.draw.connect(_draw_hud)
	ov.add_child(hud)
	post = Post.new()
	add_child(post)
	# text lives in Labels: draw_string reshapes every frame, which is what the WASM build feels
	var cl := CanvasLayer.new()
	cl.layer = 20
	add_child(cl)
	_ui = cl
	_build_hud(cl)
	Input.set_custom_mouse_cursor(Ui.tex("crosshair"), Input.CURSOR_ARROW, Vector2(16, 16))
	for i in range(TYPES.size()):
		var ty: String = TYPES[i]
		var p := ArmyPanel.new(ty, Game.units[ty], 8.0 + i * (ArmyPanel.COMPACT_SIZE.x + 8.0))
		cl.add_child(p)
		army_panels.append(p)
		p.set_selected(ty == army_type, _rng)
	if Game.sprites.has("tank"):
		tank_sprite = Hall3D.make_sprite3d("tank", 4)   # north: the dome's back, gun mount pointing away from the lens
		tank_sprite.name = "tank"
		tank_sprite.scale *= TANK_SCALE
		tank_sprite.position = Hall3D.to_world(hero_wx, hero_wd)
		world.add_child(tank_sprite)
	hero = Node3D.new()
	hero_sprite = Hall3D.make_sprite3d(Game.hero, 4)
	if tank_sprite:
		hero_sprite.scale *= TANK_SCALE   # the rider matches the tank's scale; set once, clip playback never touches .scale
		_barrel = Sprite3D.new()
		_barrel.texture = BARREL_TEX
		_barrel.pixel_size = Hall3D.PIXEL
		_barrel.billboard = BaseMaterial3D.BILLBOARD_DISABLED
		_barrel.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
		_barrel.shaded = false
		_barrel.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
		_barrel.offset = Vector2(BARREL_LEN / 2.0, 0.0)   # base end on the node origin, so rotation swings the muzzle
		_barrel.scale *= BARREL_SCALE
		_barrel.position = Vector3(0.0, 4.0 * Hall3D.PIXEL, -3.0)   # rider's base, a hair behind the rider so it sorts under
		hero.add_child(_barrel)
	_hero_rot = (hero_sprite.texture as AtlasTexture).region
	hero.add_child(hero_sprite)
	if Game.sprites.has("fx_orb"):
		_orb = Hall3D.make_fx3d("fx_orb")   # the green flame over the hero: packed wisp loop, sized in _draw_hud
		_orb.position = Vector3(0.0, 60.0 * Hall3D.PIXEL, 0.0)
		hero.add_child(_orb)
	world.add_child(hero)
	start_wave(Game.wave)
	_spawn_props()


## Camera pitch that puts the horizon line at screen y `h`.
static func pitch_for(h: float) -> float:
	return -rad_to_deg(atan((HALF_H - h) / FOCAL))


## Kit HUD: tank meter top-left, toast banner top-centre. The army panels sit under it on the
## bottom strip (ArmyPanel); the cannon cooldown ring is drawn under the crosshair in _draw_hud.
func _build_hud(cl: CanvasLayer) -> void:
	Ui.portrait(cl, Game.hero, Vector2(6, 4))
	hp_bar = Ui.bar(cl, Vector2(104, 12), Ui.COL_EMBER, "")
	hall_label = Ui.label(cl, "", Vector2(108, 44), Ui.COL_TEXT)
	score_label = Ui.label(cl, "", Vector2(108, 64), Ui.COL_DIM)
	toast_banner = Ui.patch(cl, "banner", Vector2(396, 0), Vector2(370, 150), [70, 45, 70, 72])
	toast_label = Ui.label(toast_banner, "", Vector2(36, 36), Ui.COL_HOT, 16, 298.0)
	toast_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART


func start_wave(i: int) -> void:
	Game.wave = i
	scroll_creep = SCROLL_CREEP_BY_WAVE[i]   # necromancer speeds the sweep each wave; scenery-only, sim speed (CREEP) untouched
	HALL_HALF = HALF_BY_WAVE[i]
	hall.set_half(HALL_HALF)   # rebuild floor + walls at this hall's width first, art applied next
	hall.set_hall(i)
	_flush_wall_props()        # walls moved: snap the wall lamps back onto the new wall face
	var ct := create_tween()        # bend hardens and turns faster too, blended in so the hall doesn't snap
	ct.set_parallel(true)
	ct.tween_method(func(v: float) -> void: Hall3D.curve_a = v, Hall3D.curve_a, CURVE_A_BY_WAVE[i], 2.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	ct.tween_method(func(v: float) -> void: Hall3D.curve_l = v, Hall3D.curve_l, CURVE_L_BY_WAVE[i], 2.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	wave_t = 0.0
	for u in units:
		if is_instance_valid(u):
			u.queue_free()
	units.clear()
	var w: Dictionary = Game.waves[i]
	lanes = mini(int(w["lanes"]), int(HALL_HALF * 2.0 / LANE_W))   # never wider than the hall
	spawn_interval = float(w["spawn_interval"])
	burst = int(w.get("burst", 3))
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
	hero_hp = tank_max_hp
	_low_hp_played = false
	if i == 0:
		_open_gate()
	say("Hall %d / 4" % (i + 1))


## Hall 1 opens on a gate at the far end: 7-frame swing over ~1.2s, then the rush pours through
## (spawn_t's own 2s delay already clears the animation before the first burst). Placed inside
## Hall3D.FOG_END (980), not at SPAWN_D (1050) -- past the fog end it's fully black, invisible.
const GATE_D := 780.0
func _open_gate() -> void:
	var g := Sprite3D.new()
	g.texture = GATE_TEX
	g.hframes = 7
	g.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	g.shaded = false
	g.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	g.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
	var frame_w := GATE_TEX.get_width() / 7.0
	g.pixel_size = HALL_HALF * 2.0 * 0.9 / frame_w
	g.position = Hall3D.to_world(0.0, GATE_D, GATE_TEX.get_height() * g.pixel_size * 0.5)
	world.add_child(g)
	var tw := create_tween()
	tw.tween_method(func(f: float): g.frame = mini(6, int(f)), 0.0, 7.0, 1.2)
	tw.tween_interval(8.0)   # held open through the rush's approach, not freed the instant the swing ends
	tw.tween_callback(g.queue_free)


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
				# down-range through the ranks, launching everything passed, then back to the slot.
				# Staggered by rank row so the files run spaced out instead of one stacked clump; the
				# front row leads and everyone still bulldozes onto the same pile line.
				u.wd = BEHIND_D - CHARGE_GAP * roundf((RANK_D0 - sl.y) / RANK_STEP)
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
		Sound.unit(type, "ability", 0.0)


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


func find_target(u: Unit) -> Node3D:
	var best: Node3D = null
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


## (wx, wd) of a target: units carry their own, the hero's live on the battle.
func _wpos(n: Node3D) -> Vector2:
	return Vector2(n.wx, n.wd) if n is Unit else Vector2(hero_wx, hero_wd)


func hit(a: Unit, t: Node3D, mult := 1.0) -> void:
	var d := a.dmg * mult
	if t is Unit and t.type in a.counters:
		d *= float(Game.units["counter_mult"])
	if a.team == Unit.ALLY:
		d *= 1.0 + Game.relic_bonus("dmg")
	var tp := _wpos(t)
	var to := tp - Vector2(a.wx, a.wd)
	a.attack_anim()
	if a.rng > 100.0:
		_arrow(Vector2(a.wx, a.wd), 50.0, tp, 30.0)
		await get_tree().create_timer(0.12).timeout
		if not is_instance_valid(t) or not is_instance_valid(a):
			return   # either side can die in flight
	else:
		a.lunge(to.normalized() * 14.0)
		if t is Unit:
			_slash(a, t, to)
	_impact(tp.x, tp.y, 6.0)
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
		_hero_play("hurt")
		Sound.play(str(Game.units["hero_sfx"].get("hit", "")), hero_wx)
		if hero_hp < 25.0 and not _low_hp_played:
			_low_hp_played = true
			Sound.play(str(Game.units["hero_sfx"].get("low_hp", "")), hero_wx)


## Slash fx: an area hit parked on the target. The clip plays once in the world and every frame
## tick cleaves each enemy inside SLASH_R for a mini hit, re-popping airborne ones so crowds
## juggle. The swing itself is the unit's own attack clip. units.json "slash": fx_<name>,
## "slash_y": height above the target's feet.
func _slash(a: Unit, t: Unit, to: Vector2) -> void:
	var kind: String = Game.units[a.type].get("slash", "")
	if kind == "" or not Game.sprites.has("fx_" + kind):
		return
	var wp := Vector2(t.wx, t.wd)
	var cell := float(Game.sprites["fx_" + kind]["cell"])
	var s := Hall3D.make_fx3d("fx_" + kind)
	s.pixel_size = 0.9 * 64.0 * Hall3D.PIXEL / cell     # same on-screen size as the old 2D clip
	s.position = Hall3D.to_world(wp.x, wp.y, -float(Game.units[a.type].get("slash_y", -40.0)) * Hall3D.PIXEL)
	s.modulate.a = SLASH_ALPHA
	s.flip_h = to.x < 0.0
	world.add_child(s)
	var frames := s.hframes
	var dmg := a.dmg * SLASH_DMG
	var team := a.team   # by value: the clip outlives the attacker when it dies mid-swing
	var tid := t.get_instance_id()
	var tw := s.create_tween()
	for f in range(frames):
		tw.tween_callback(func():
			s.frame = f
			if f % SLASH_EVERY != 0:
				return
			for o in units:
				if o.get_instance_id() != tid and o.team != team and not o.dead and Vector2(o.wx, o.wd).distance_to(wp) < SLASH_R:
					_impact(o.wx, o.wd, 4.0)
					if o.air_h > 0.0:
						o.launch(VORTEX_LIFT)
					o.take(dmg, to.normalized() * 3.0))
		tw.tween_interval(0.05)
	tw.tween_callback(s.queue_free)


## Vortex cleave: the blue sweep clip parked in the world, spinning in place and cleaving every
## enemy inside its radius twice a second for a few seconds. Hovers at head height so the
## circle isn't wasted on ground nobody stands on. k scales size and reach together.
func _vortex(wp: Vector2, k: float, dmg: float, secs := 2.0, fx_name := "fx_sweep_thin") -> void:
	var cell := float(Game.sprites[fx_name]["cell"])
	var s := Hall3D.make_fx3d(fx_name)
	s.set_meta("vortex", true)
	s.pixel_size = 64.0 * Hall3D.PIXEL * k / cell      # hi-res clips shrink back: same size, thinner lines
	s.position = Hall3D.to_world(wp.x, wp.y, 48.0 * Hall3D.PIXEL)   # head height stays put as the circle grows
	s.modulate.a = SLASH_ALPHA
	if fx_name == "fx_sweep_thin":
		s.modulate = Color(3.0, 3.0, 3.0, 0.95)   # blown out near white; the blue only survives at the edges
	world.add_child(s)
	# never in lockstep: random facing, random spin direction, own phase and cadence
	s.rotation.z = _rng.randf() * TAU
	s.flip_h = _rng.randf() < 0.5
	var phase := _rng.randf()
	var spin := s.create_tween().set_loops()
	spin.tween_method(func(t: float): s.frame = wrapi(int((t + phase) * s.hframes), 0, s.hframes), 0.0, 1.0, _rng.randf_range(0.32, 0.5))
	var beat := s.create_tween().set_loops(int(secs * VORTEX_HZ))
	beat.tween_interval(1.0 / VORTEX_HZ)
	beat.tween_callback(func():
		for o in units:
			if o.team == Unit.ENEMY and not o.dead and Vector2(o.wx, o.wd).distance_to(wp) < VORTEX_R * k:
				_impact(o.wx, o.wd, 4.0)
				if o.air_h > 0.0:
					o.launch(VORTEX_LIFT)   # already up: keep it up. Lifting off the ground is the halberdiers' job
				o.take(dmg)
				_hitstop(SLASH_STOP))
	var life := s.create_tween()
	life.tween_interval(secs)
	life.tween_property(s, "modulate:a", 0.0, 0.2)
	life.tween_callback(s.queue_free)


## Hit stop: the world crawls for secs of real time. A later, longer stop extends the hold;
## the timer that fired last always restores full speed.
var _stop_until := 0.0
## Decaying two-axis camera shake: `amp` world units at t=0, quadratic falloff over `dur` real
## seconds (runs on real time so it isn't frozen by the hit stop). Re-arming takes the larger.
func _shake(amp: float, dur: float) -> void:
	_shake_amp = maxf(amp, _shake_amp * _shake_t / maxf(_shake_dur, 0.01))
	_shake_dur = dur
	_shake_t = dur


func _tick_shake(delta: float) -> void:
	if _shake_t <= 0.0:
		return
	_shake_t -= delta / maxf(Engine.time_scale, 0.01)
	if _shake_t <= 0.0:
		camera.h_offset = 0.0
		camera.v_offset = 0.0
		return
	var a := _shake_amp * pow(_shake_t / _shake_dur, 2.0)
	camera.h_offset = _rng.randf_range(-a, a)
	camera.v_offset = _rng.randf_range(-a, a) * 0.7


## One-frame orange screen flash that fades over 0.15 s.
func _blast_flash() -> void:
	if not _flash_rect:
		_flash_rect = ColorRect.new()
		_flash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_flash_rect.size = get_viewport().get_visible_rect().size   # fx is a Node2D: no anchors, size it by hand
		fx.add_child(_flash_rect)
	_flash_rect.color = Color(1.0, 0.75, 0.4, 0.35)
	var tw := create_tween()
	tw.tween_property(_flash_rect, "color:a", 0.0, 0.15).set_ease(Tween.EASE_OUT)


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


## Yellow burst on the fx overlay. `r` and `h` are sprite pixels — the unit every fx number in this
## file is tuned in — so both shrink with depth exactly like a sprite does.
func _impact(wx: float, wd: float, r := 6.0, h := 34.0) -> void:
	var k := Hall3D.screen_scale(camera, wd) * Hall3D.PIXEL
	_burst(Hall3D.unproject(camera, wx, wd, h * Hall3D.PIXEL), r * k)


## Melee hit spark: 4-frame 32px flash (~0.1 s), star / x alternating so a crowd of hits doesn't
## read as one shape. Falls back to the longer spark burst when the snappy rows aren't packed.
const HIT_FX := ["fx_hit_star", "fx_hit_x"]
const HIT_DT := 0.025
func _burst(p: Vector2, r: float) -> void:
	var name: String = HIT_FX[_rng.randi() % HIT_FX.size()]
	if Game.sprites.has(name):
		_clip2d(name, p, r * 3.2 / float(Game.sprites[name]["cell"]), HIT_DT).rotation = _rng.randf() * TAU
	else:
		_clip2d("fx_burst_big", p, r * 4.8 / 64.0, 0.03)


## Atlas fx clip parked in the world at `wp`, `h` sprite px above the feet, `size` sprite px across
## its cell. loops 0 = forever (caller frees), n = play n times then free.
func _clip3d(name: String, wp: Vector2, h: float, size: float, dt: float, tint := Color.WHITE, loops := 1) -> Sprite3D:
	var s := Hall3D.make_fx3d(name)
	s.pixel_size = size * Hall3D.PIXEL / float(Game.sprites[name]["cell"])
	s.position = Hall3D.to_world(wp.x, wp.y, h * Hall3D.PIXEL)
	s.modulate = tint
	world.add_child(s)
	var tw := s.create_tween()
	tw.set_loops(loops)
	for f in range(s.hframes):
		tw.tween_callback(func(): s.frame = f)
		tw.tween_interval(dt)
	if loops > 0:
		tw.finished.connect(s.queue_free)
	return s


## One-shot atlas fx clip on the 2D overlay at screen point `p`, `scale` per 64px cell.
func _clip2d(name: String, p: Vector2, scale: float, dt: float, tint := Color.WHITE) -> Sprite2D:
	var s := Game.make_fx(name)
	s.position = p
	s.scale = Vector2.ONE * scale
	s.modulate = tint
	s.rotation = _rng.randf_range(-0.3, 0.3)
	fx.add_child(s)
	var tw := create_tween()
	for f in range(s.hframes):
		tw.tween_callback(func(): s.frame = f)
		tw.tween_interval(dt)
	tw.tween_callback(s.queue_free)
	return s


## Arrow streak between two world spots, each `h` sprite pixels above its own feet.
func _arrow(from_w: Vector2, from_h: float, to_w: Vector2, to_h: float) -> void:
	var from := Hall3D.unproject(camera, from_w.x, from_w.y, from_h * Hall3D.PIXEL)
	var to := Hall3D.unproject(camera, to_w.x, to_w.y, to_h * Hall3D.PIXEL)
	# packed arrow sprite (fx_arrow, points right) flown along a shallow arc, sized to the far end
	var a := Game.make_fx("fx_arrow")
	a.position = from
	a.scale = Vector2.ONE * Hall3D.screen_scale(camera, to_w.y) * 0.6
	fx.add_child(a)
	var tw := create_tween()
	tw.tween_method(func(k: float):
		var prev := a.position
		var p := from.lerp(to, k)
		p.y -= sin(k * PI) * from.distance_to(to) * 0.12
		a.position = p
		if p != prev:
			a.rotation = (p - prev).angle(), 0.0, 1.0, 0.12)
	tw.tween_callback(a.queue_free)


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
	if u.team == Unit.ALLY and u.lane >= 0 and front[u.lane] == u:
		front[u.lane] = null


func say(t: String) -> void:
	toast = t
	toast_t = 2.5


func _cursor_world() -> Vector2:
	return Hall3D.cursor_world(camera, get_viewport().get_mouse_position(), SPAWN_D)


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
	if hl < TANK_FOOTPRINT_R and hl > 0.01:
		push += hd / hl * (TANK_FOOTPRINT_R - hl) * 14.0
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
	_tick_shake(delta)
	hud.queue_redraw()
	_rebuild_grid()
	toast_t -= delta
	wave_t += delta
	scroll += scroll_creep * minf(1.0 + wave_t * SCROLL_ACCEL, 2.5) * delta
	hall.set_scroll(scroll)
	_update_props()
	_update_chests()
	if _done:
		return
	# spawns
	spawn_t -= delta
	if spawn_t <= 0.0 and not spawn_queue.is_empty():
		spawn_t = spawn_interval
		# bursts: the horde arrives in clumps, not a drip
		for i in range(mini(burst, spawn_queue.size())):
			_spawn_enemy(spawn_queue.pop_back())
	# upgrade chests: one every ~18s, scrolling in on the same treadmill as the props
	_chest_t -= delta
	if _chest_t <= 0.0:
		_chest_t = CHEST_INTERVAL
		_spawn_chest()
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
	# the tank stands fixed at the gate; pinned every frame (not just set once) so nothing --
	# including an adversary poke -- can drift it, and so to_world's curve offset (which moves as
	# the hall bends) keeps tracking it
	hero_wx = 0.0
	hero_wd = 190.0
	hero.position = Hall3D.to_world(hero_wx, hero_wd, TANK_MOUNT_H * Hall3D.PIXEL)
	if tank_sprite:
		tank_sprite.position = Hall3D.to_world(hero_wx, hero_wd)
	hero_sprite.modulate = hero_sprite.modulate.lerp(Color.WHITE, delta * 6.0)
	# rider and cannon neck both track the cursor's ground point. The neck only yaws (no pitch): its
	# world tip is projected through the camera, so up-hall foreshortens to a stub and sideways
	# shows the full length.
	var cursor_wp := _cursor_world()
	var facing := Game.facing_from(Vector2(cursor_wp.x - hero_wx, -(cursor_wp.y - hero_wd)))
	if _barrel:
		_barrel_yaw = atan2(cursor_wp.y - hero_wd, cursor_wp.x - hero_wx)
		var p0 := _pivot()
		var v := _muzzle() - p0
		_barrel.rotation.z = atan2(-v.y, v.x)
		var k := Hall3D.screen_scale(camera, hero_wd)
		_barrel.scale.x = maxf(v.length() / (BARREL_LEN * Hall3D.PIXEL * k), 0.2)   # projected length; never below a nub
		_barrel.position.z = 2.0 - 6.0 * sin(_barrel_yaw)   # deeper: behind the rider; toward the lens: in front
	if _hero_t >= 0.0:
		_hero_t += delta
		var n := int(_hero_clip["frames"])
		if _hero_t >= n * Unit.ATK_DT:
			_hero_t = -1.0
			Hall3D.rotation_frame(hero_sprite, _hero_rot, facing)
		else:
			Hall3D.clip_frame(hero_sprite, _hero_rot, _hero_clip, int(_hero_t / Unit.ATK_DT))
	else:
		Hall3D.rotation_frame(hero_sprite, _hero_rot, facing)
	# gatling: hold LMB ("cast" action, unchanged in project.godot) to fire at rate
	_gatling_cd -= delta
	_cannon_cd -= delta
	if Input.is_action_pressed("cast") and _gatling_cd <= 0.0:
		_gatling_cd = 1.0 / (GATLING_RATE * gatling_rate_mult)
		_fire_gatling()
	# lose / win
	if hero_hp <= 0.0:
		say("The line breaks. Again.")
		_card("card_lose", "The line breaks.", 2.5)
		Sound.play(str(Game.units["hero_sfx"].get("lose", "")), hero_wx)
		start_wave(Game.wave)
	elif spawn_queue.is_empty() and units.filter(func(u): return is_instance_valid(u) and u.team == Unit.ENEMY).is_empty():
		_wave_done()


func _wave_done() -> void:
	_done = true
	Sound.play(str(Game.units["hero_sfx"].get("wave_clear", "")), hero_wx)
	if Game.wave >= 3:
		say("The throne room door. It is already open.\nTO BE CONTINUED")
		_card("card_win", "The door is already open.", 4.0)
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
			t.tween_property(camera, "position:x", dx, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
			t.tween_property(camera, "position:x", 0.0, 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		"stairs":
			say("Stairs. Up.")
			t.tween_property(camera, "rotation_degrees:x", pitch_for(170.0), 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
			t.tween_property(camera, "rotation_degrees:x", pitch_for(HORIZON), 1.2).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		_:
			say("Hall cleared.")
			t.tween_interval(2.5)
	await t.finished


func _unhandled_input(e: InputEvent) -> void:
	if e is InputEventMouseButton and e.pressed and e.button_index == MOUSE_BUTTON_RIGHT:
		_fire_cannon()
	elif e is InputEventKey and e.pressed and not e.echo:
		match e.keycode:
			KEY_Q: _cycle_army(-1)
			KEY_E: _cycle_army(1)
			KEY_BRACKETRIGHT: post.cycle(1)
			KEY_BRACKETLEFT: post.cycle(-1)


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
	Sound.unit(u.type, "ability", u.wx)
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
					var fx_name := "fx_" + str(Game.units[u.type].get("slash", ""))
					_vortex(Vector2(cx + _rng.randf_range(-30.0, 30.0), cd + _rng.randf_range(-35.0, 35.0)), 2.6, u.dmg * VORTEX_DMG, 4.0,
						fx_name if Game.sprites.has(fx_name) else "fx_sweep_thin")
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
				u.lunge((Vector2(near[0].wx, near[0].wd) - here).normalized() * 18.0)
				_impact(near[0].wx, near[0].wd, 9.0)
				near[0].take(45.0, Vector2(0, -6))
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
## Airborne enemies along the path are run through - damaged plus a small juggle tap -
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
		var p := Hall3D.unproject(camera, wp.x, wp.y, h * Hall3D.PIXEL)
		l.visible = true
		l.set_point_position(0, p)
		var tail: Vector2 = p if prev[0] == Vector2.INF else prev[0]
		var cap := 16.0 * Hall3D.PIXEL * Hall3D.screen_scale(camera, wp.y)   # streak, scaled with depth
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
				_burst(Hall3D.unproject(camera, mark.x, mark.y), 5.0)
				break
		l.queue_free())


## A sky-dropped hammer hit the ground: shockwave - same numbers as its slam ability.
func sky_landing(u: Unit) -> void:
	var here := Vector2(u.wx, u.wd)
	_flash(here, 60.0, Color(1.0, 0.8, 0.5, 0.6))
	for o in units:
		if o.team == Unit.ENEMY and here.distance_to(Vector2(o.wx, o.wd)) < 90.0:
			o.take(20.0)
			o.rooted_until = _now() + 2.5


## Full-screen illustration card (assets/ui/<name>.png, 400x224 drawn at 2x) with a title in the
## blackletter face, fading out after `secs`. Win and lose both go through here.
func _card(name: String, title: String, secs: float) -> void:
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.7)
	dim.size = Vector2(960, 540)
	dim.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_ui.add_child(dim)
	var pic := Ui.sprite(dim, name, Vector2(80, 30))
	pic.scale = Vector2(2, 2)
	Ui.label(dim, title, Vector2(0, 486), Ui.COL_EMBER, 32, 960.0).add_theme_font_override("font", Ui.font("title32"))
	var tw := create_tween()
	tw.tween_interval(secs - 0.5)
	tw.tween_property(dim, "modulate:a", 0.0, 0.5)
	tw.tween_callback(dim.queue_free)


## Start a packed hero clip (attack / hurt) if this hero has one; walk loops on its own in _process.
func _hero_play(key: String) -> void:
	var c: Dictionary = Game.sprites[Game.hero].get(key, {})
	if not c.is_empty():
		_hero_clip = c
		_hero_t = 0.0


## Screen-space target pick: the enemy whose unprojected sprite rect (feet + top from
## Hall3D.sprite_top_screen, width from its cell scaled by depth) contains `cursor`; ties broken
## by nearest depth (smallest wd, i.e. the one drawn in front).
func _screen_pick(cursor: Vector2) -> Unit:
	var best: Unit = null
	var best_wd := INF
	for u in units:
		if u.team != Unit.ENEMY or u.dead:
			continue
		var feet := Hall3D.unproject(camera, u.wx, u.wd)
		var top := Hall3D.sprite_top_screen(camera, u)
		var cell := float(Game.sprites[Game.units[u.type]["sprite"]]["cell"])
		var w := cell * Hall3D.PIXEL * Hall3D.screen_scale(camera, u.wd)
		if absf(cursor.x - feet.x) <= w * 0.5 and cursor.y >= top.y and cursor.y <= feet.y and u.wd < best_wd:
			best_wd = u.wd
			best = u
	return best


## Same screen-rect test, for the live upgrade chests.
func _screen_pick_chest(cursor: Vector2) -> Dictionary:
	for c in _up_chests:
		if not c["alive"]:
			continue
		var feet := Hall3D.unproject(camera, c["wx"], c["d"])
		var cell := float(Game.sprites["chest_ornate"]["cell"])
		var w := cell * Hall3D.PIXEL * Hall3D.screen_scale(camera, c["d"])
		if absf(cursor.x - feet.x) <= w * 0.5 and absf(cursor.y - feet.y) <= w * 0.5:
			return c
	return {}


## LMB, held: ~12 shots/s hitscan. Every hit pops the juggle (take's own POP) and, if the target
## was grounded, adds a launch so the player has to keep tracking it. Also test-called by
## Probe/Adversary with an explicit screen point.
func _fire_gatling() -> void:
	Sound.play("gatling_shot.wav", hero_wx)
	var cursor := get_viewport().get_mouse_position() + Vector2(_rng.randf_range(-6.0, 6.0), _rng.randf_range(-6.0, 6.0))
	_gatling_hit_at(cursor)


## Screen point of the barrel tip: the yawed world tip projected through the camera.
func _muzzle() -> Vector2:
	if not _barrel:
		return Hall3D.unproject(camera, hero_wx, hero_wd, (TANK_MOUNT_H + 20.0 * TANK_SCALE) * Hall3D.PIXEL)
	var l := BARREL_LEN * BARREL_SCALE * Hall3D.PIXEL   # world units
	return Hall3D.unproject(camera, hero_wx + cos(_barrel_yaw) * l, hero_wd + sin(_barrel_yaw) * l, (TANK_MOUNT_H + 4.0) * Hall3D.PIXEL)


## Screen point of the barrel's pivot on the turret cap.
func _pivot() -> Vector2:
	return Hall3D.unproject(camera, hero_wx, hero_wd, (TANK_MOUNT_H + 4.0) * Hall3D.PIXEL)


## Aim is always the aim pixel (cursor + spread) -- no target search, no snapping, no lead: an
## enemy only takes the hit if its screen rect covers that exact pixel (_screen_pick's point test,
## front-most by depth on overlap). A hit sparks on the enemy's own body (the standard melee flash,
## _impact); nothing under the cursor just means the round hits the floor -- a small spark there instead.
func _gatling_hit_at(cursor: Vector2) -> void:
	var target := _screen_pick(cursor)
	if target:
		# launch before take, same as _cannon_explode_at: a killing hit still reads as a knock-up
		if target.air_h == 0.0:
			target.launch(120.0)
		target.take(GATLING_DMG)
		_impact(target.wx, target.wd, 6.0)
	else:
		_burst(cursor, 8.0)
	var chest := _screen_pick_chest(cursor)
	if not chest.is_empty():
		_pop_chest(chest)
	_tracer(_muzzle(), cursor)


## One-frame tracer from the muzzle to the hit point.
func _tracer(from: Vector2, to: Vector2) -> void:
	var l := Line2D.new()
	l.width = 2.0
	l.default_color = Color(1.0, 0.9, 0.5, 0.9)
	l.points = PackedVector2Array([from, to])
	fx.add_child(l)
	await get_tree().process_frame
	l.queue_free()


## RMB: one shell per press, on a cooldown. Flies to the cursor's ground point along an arc
## (same tail-cap Line2D streak _rain_salvo's _volley_arrow uses), then explodes.
func _fire_cannon() -> void:
	if _cannon_cd > 0.0:
		return
	_cannon_cd = CANNON_CD * cannon_reload_mult
	# land on the enemy under the cursor if there is one (its body sits above its feet, so the
	# floor point under the cursor would be deeper down the hall than what you're pointing at)
	var picked := _screen_pick(get_viewport().get_mouse_position())
	var to := Vector2(picked.wx, picked.wd) if picked else _cursor_world()
	var from := Vector2(hero_wx, hero_wd)
	Sound.play("cannon_fire.wav", hero_wx)
	var l := Line2D.new()
	l.width = 2.5
	l.default_color = Color(1.0, 0.75, 0.35)
	l.add_point(Vector2.ZERO)
	l.add_point(Vector2.ZERO)
	l.visible = false
	fx.add_child(l)
	var prev := [Vector2.INF]
	var tw := create_tween()
	tw.tween_method(func(k: float):
		var wp := from.lerp(to, k)
		var h := 90.0 * sin(k * PI)
		var p := Hall3D.unproject(camera, wp.x, wp.y, h * Hall3D.PIXEL)
		l.visible = true
		l.set_point_position(0, p)
		var tail: Vector2 = p if prev[0] == Vector2.INF else prev[0]
		var cap := 22.0 * Hall3D.PIXEL * Hall3D.screen_scale(camera, wp.y)
		if p.distance_to(tail) > cap:
			tail = p + (tail - p).normalized() * cap
		l.set_point_position(1, tail)
		prev[0] = p, 0.0, 1.0, CANNON_FLIGHT)
	tw.tween_callback(func():
		l.queue_free()
		_cannon_explode_at(to))


func _cannon_explode_at(at: Vector2) -> void:
	Sound.play("cannon_explosion.wav", at.x)
	var r := CANNON_RADIUS * cannon_radius_mult
	_flash(at, r, Color(1.0, 0.6, 0.3, 0.7))
	var k := Hall3D.screen_scale(camera, at.y)
	var p := Hall3D.unproject(camera, at.x, at.y, 28.0 * Hall3D.PIXEL)   # centred a body-height up, not half under the floor
	# main fireball sized to the blast radius, then two smaller after-bursts off-centre a beat
	# later so the blast rolls instead of popping
	var cell := float(Game.sprites[EXPLOSION_FX]["cell"])
	var main := _clip2d(EXPLOSION_FX, p, r * k * 2.4 / cell, 0.025, Color(1.0, 1.0, 1.0, 0.5))   # ~300 ms, half-transparent: never blocks the fight
	main.rotation = 0.0   # smoke rises: keep the clip upright
	for i in 2:
		var off := Vector2(_rng.randf_range(-0.6, 0.6), _rng.randf_range(-0.35, 0.1)) * r * k
		get_tree().create_timer(0.04 + 0.04 * i).timeout.connect(func():
			var s := _clip2d(EXPLOSION_FX, p + off, r * k * 1.2 / cell, 0.02, Color(1.0, 0.85, 0.6, 0.5))
			s.rotation = 0.0)
	_shake(SHAKE_AMP, 0.4)
	_blast_flash()
	for u in units.duplicate():
		var d := Vector2(u.wx, u.wd).distance_to(at)
		if u.team == Unit.ENEMY and not u.dead and d < r:
			# launch before take: a one-shot kill (hall-1 fodder) still needs the knock-up to read,
			# and take() would otherwise mark it dead first, making launch() a no-op
			var near := 1.0 - d / r   # 1 at ground zero, 0 at the rim
			if u.air_h == 0.0:
				u.launch(180.0 + 140.0 * near)
			# radial throw: everyone flies away from the blast, hardest at the centre
			var away := (Vector2(u.wx, u.wd) - at).normalized() if d > 1.0 else Vector2.RIGHT.rotated(_rng.randf() * TAU)
			u.air_vx = away.x * (40.0 + 120.0 * near)
			u.air_vd = away.y * (40.0 + 120.0 * near)
			u.take(CANNON_DMG)
	for c in _up_chests:
		if c["alive"] and Vector2(c["wx"], c["d"]).distance_to(at) < CHEST_POP_R:
			_pop_chest(c)
	_hitstop(0.12, 0.05)


## Spawn one upgrade chest at the far end of the treadmill (or an explicit spot, for the probe).
func _spawn_chest(wx := INF, d := INF) -> void:
	var range_d := Hall3D.FOG_END * 1.5 - 60.0
	var use_d: float = d if d != INF else range_d - 10.0
	var use_wx: float = wx if wx != INF else _floor_wx()
	var s := Hall3D.make_sprite3d("chest_ornate", 0)
	s.scale *= 1.1   # matches the ambient floor props' scale (_add_prop)
	world.add_child(s)
	var bn := Hall3D.make_sprite3d("chest_ornate_open", 0)
	bn.scale = s.scale
	bn.visible = false
	world.add_child(bn)
	_up_chests.append({"node": s, "broken": bn, "wx": use_wx, "base_d": scroll + use_d, "d": use_d, "alive": true})


## Move every live chest along the same treadmill the floor props use; one that scrolls behind the
## lens without being popped is lost.
func _update_chests() -> void:
	var range_d := Hall3D.FOG_END * 1.5 - 60.0
	for c in _up_chests.duplicate():
		var d: float = 60.0 + fposmod(c["base_d"] - scroll, range_d)
		if c.has("last_d") and d < float(c["last_d"]) - range_d * 0.5:
			c["node"].queue_free()
			c["broken"].queue_free()
			_up_chests.erase(c)
			continue
		c["last_d"] = d
		c["d"] = d
		var wp := Hall3D.to_world(c["wx"], d, 0.0)
		c["node"].position = wp
		c["broken"].position = wp


func _pop_chest(c: Dictionary) -> void:
	if not c["alive"]:
		return
	c["alive"] = false
	c["node"].visible = false
	c["broken"].visible = true
	_clip3d("fx_smash", Vector2(c["wx"], c["d"]), 14.0, 48.0, 0.05)
	_grant_upgrade()


func _grant_upgrade() -> void:
	var kind: String = UPGRADE_CYCLE[_upgrade_i % UPGRADE_CYCLE.size()]
	_upgrade_i += 1
	match kind:
		"gatling_rate":
			gatling_rate_mult *= 1.25
			say("GATLING RATE +25%")
		"cannon_radius":
			cannon_radius_mult *= 1.20
			say("CANNON RADIUS +20%")
		"cannon_reload":
			cannon_reload_mult *= 0.80
			say("CANNON RELOAD -20%")
		"tank_hp":
			tank_max_hp += 25.0
			hero_hp = minf(tank_max_hp, hero_hp + 25.0)
			say("TANK +25 HP")


## Ground-level ring on the fx overlay. `r` is in world units.
func _flash(wp: Vector2, r: float, c: Color) -> void:
	# expanding ring decal squashed onto the floor; the ring's last frame spans ~56px of its 64 cell
	var k := Hall3D.screen_scale(camera, wp.y)
	var s := _clip2d("fx_shockwave", Hall3D.unproject(camera, wp.x, wp.y), r * k * 2.0 / 56.0, 0.05, c)
	s.rotation = 0.0
	s.scale.y *= 0.45


## Wall lamps (treadmill down both walls) + floor tables: persistent Sprite3D props that scroll
## with the hall on the same wrap math the old sconces used, and go dark when something walks into
## them. base_d is each prop's fixed offset into the 60..FOG_END*1.5 cycle; the live depth is
## `60 + fposmod(base_d - scroll, range)`, which treadmills toward the camera and wraps back to
## the far end — the wrap (a big depth jump) is also the "un-break" moment.
func _spawn_props() -> void:
	var range_d := Hall3D.FOG_END * 1.5 - 60.0
	# wall items: cage lamps only, every 160 depth. Candelabra, brazier and both mirrors are
	# owner-cut from the wall (they stay in roster, unused).
	for side: float in [-1.0, 1.0]:
		var wx: float = side * HALL_HALF   # flush on the wall face, not inset
		var facing := 2 if side < 0.0 else 6   # east / west: faces into the hall
		var d := 0.0
		while d < range_d:
			_add_prop("lamp_cage", wx, d, facing, false, Hall3D.TILE * Hall3D.COURSES * 1.3, "", 0.575)
			d += 160.0
	# floor clutter: tables/chairs/chests, kept off the rank lane and away from the front line
	# chest_ornate is a dedicated upgrade prop now (_spawn_chest); chest_coffer stays ambient clutter
	var floor_names := ["table_map", "table_trestle", "chair_bench", "chest_coffer", "brazier", "column_stump", "statue_knight", "banner_pole", "rubble", "barricade", "cage_skeleton", "altar", "barrel", "weapon_rack", "coffin", "gravestone", "cauldron", "bookshelf", "throne", "bone_pile", "stocks", "anvil", "sarcophagus", "crates"]
	var broken_of := {"chair_bench": "chair_broken"}
	var n := 20
	for t in range(n):
		var d0: float = fmod(t * (range_d / n), range_d)
		if absf(d0 - FRONT_D) < 60.0:   # don't spawn right on the front line
			d0 += range_d / (n * 2.0)
		var fname: String = floor_names[t % floor_names.size()]
		_add_prop(fname, _floor_wx(), d0, 0, true, 0.0, broken_of.get(fname, ""), 1.1)


## The wall lamps sit flush on the wall face (_spawn_props); a hall change moves the walls, so
## snap every lamp's wx onto the new half, keeping the side it was already on.
func _flush_wall_props() -> void:
	for p in _props:
		if not p["is_floor"]:
			p["wx"] = signf(p["wx"]) * HALL_HALF


## Random wx hugging a wall, clear of the rank block that fills the centre of the hall.
func _floor_wx() -> float:
	var side := 1.0 if _rng.randf() < 0.5 else -1.0
	return side * _rng.randf_range(RANK_HALF * 0.6, HALL_HALF - 40.0)


## A prop's flicker frames (if packed for this facing) fully replace its rotation strip, then a
## looping tween cycles s.frame over it — random per-lamp duration so the wall doesn't blink in
## lockstep. Only flicker_east is packed; the west wall mirrors it.
func _add_prop(name: String, wx: float, base_d: float, facing: int, is_floor: bool, h: float, broken_name: String, sc: float) -> void:
	var s := Hall3D.make_sprite3d(name, facing)
	s.scale *= sc
	if name == "lamp_cage":   # hung from darkness above: fade the top of the sprite to black
		var lamp_mat: ShaderMaterial = s.material_override
		lamp_mat.shader = LAMP_SHADER
		lamp_mat.set_shader_parameter("top_y", float(Game.sprites[name]["cell"]) * Hall3D.PIXEL)
	world.add_child(s)
	# one strip for both walls: the west generation has stray sparks the owner cut, so east is
	# flipped for the far wall instead of packing a second clip
	var clip: Dictionary = Game.sprites[name].get("flicker_east", {}) if facing == 2 or facing == 6 else {}
	if not clip.is_empty():
		s.flip_h = facing == 6
		var at: AtlasTexture = s.texture
		var n := int(clip["frames"])
		at.region = Rect2(int(clip.get("x", 0)), int(clip["y"]), at.region.size.x / 8.0 * n, at.region.size.y)
		s.hframes = n
		var dur := _rng.randf_range(0.6, 0.9)
		var tw := create_tween()
		tw.set_loops()
		tw.tween_method(func(t: float): s.frame = int(t) % n, 0.0, float(n), dur).set_delay(_rng.randf_range(0.0, dur))
	var bn: Sprite3D = null
	if broken_name != "":
		bn = Hall3D.make_sprite3d(broken_name, facing)
		bn.scale = s.scale
		bn.visible = false
		world.add_child(bn)
	_props.append({"node": s, "broken": bn, "wx": wx, "base_d": base_d, "last_d": 0.0, "alive": true, "is_floor": is_floor, "h": h})


func _update_props() -> void:
	var range_d := Hall3D.FOG_END * 1.5 - 60.0
	_prop_tick += 1
	var check := _prop_tick % 4 == 0
	for p: Dictionary in _props:
		var d: float = 60.0 + fposmod(p["base_d"] - scroll, range_d)
		if d > p["last_d"] + range_d * 0.5:   # wrapped from near back to far: reset + un-break
			p["alive"] = true
			p["node"].visible = true
			if p["broken"]:
				p["broken"].visible = false
			if p["is_floor"]:
				p["wx"] = _floor_wx()
		p["last_d"] = d
		var wp := Hall3D.to_world(p["wx"], d, p["h"])
		p["node"].position = wp
		if p["broken"]:
			p["broken"].position = wp
		if not check or not p["alive"]:
			continue
		var here := Vector2(p["wx"], d)
		var hit := here.distance_to(Vector2(hero_wx, hero_wd)) < PROP_BREAK_R
		if not hit:
			for u in units:
				if not u.dead and here.distance_to(Vector2(u.wx, u.wd)) < PROP_BREAK_R:
					hit = true
					break
		if not hit:
			continue
		p["alive"] = false
		_impact(p["wx"], d)
		if Game.sprites.has("fx_smash"):
			_clip3d("fx_smash", here, 14.0, 48.0, 0.05)
		else:
			_flash(here, 10.0, Color(0.9, 0.8, 0.5))
		p["node"].visible = false
		if p["broken"]:
			p["broken"].visible = true   # smashed-open state does the talking
		else:
			_prop_debris(p["node"])


## Debris chunks torn from a prop's own texture on break — same recipe as Unit._gib, just fewer
## and shorter-lived, and parented under world instead of the unit.
func _prop_debris(s: Sprite3D) -> void:
	var at: AtlasTexture = s.texture
	var cell: float = at.region.size.y
	var k := Hall3D.PIXEL
	for i in range(_rng.randi_range(4, 6)):
		var c := Sprite3D.new()
		var sub := AtlasTexture.new()
		sub.atlas = at.atlas
		var sz := _rng.randf_range(4.0, 9.0)
		var ox := _rng.randf_range(cell * 0.2, cell * 0.8 - sz)
		var oy := _rng.randf_range(cell * 0.2, cell * 0.9 - sz)
		sub.region = Rect2(at.region.position + Vector2(s.frame * cell + ox, oy), Vector2(sz, sz))
		c.texture = sub
		c.pixel_size = k
		c.billboard = BaseMaterial3D.BILLBOARD_DISABLED
		c.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
		c.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
		c.alpha_scissor_threshold = 0.35
		c.shaded = false
		var p0 := s.global_position + Vector3(ox - cell / 2.0, cell - oy, 0.0) * k
		var v := Vector3(_rng.randf_range(-60.0, 60.0), _rng.randf_range(50.0, 140.0), 0.0) * k
		c.position = p0
		world.add_child(c)
		var tw := create_tween()
		tw.tween_method(func(t: float): c.position = p0 + v * t - Vector3(0.0, 300.0 * k, 0.0) * t * t, 0.0, 1.0, 0.6)
		tw.tween_property(c, "modulate:a", 0.0, 0.3).set_delay(0.3)
		tw.tween_callback(c.queue_free)


func _draw_hud() -> void:
	# hero flame rides the top layer so it glows over the ranks
	if _orb:
		_orb.pixel_size = Hall3D.PIXEL * (0.6 + minf(Game.magic, 200.0) / 200.0)   # 32px wisp grows with the pool
		_orb.frame = int(wave_t * 8.0) % _orb.hframes
	else:
		hud.draw_circle(Hall3D.unproject(camera, hero_wx, hero_wd, 60.0 * Hall3D.PIXEL), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	# bottom strip: the four army panels, ranks left, ability cooldown; current army type large + on top
	for p in army_panels:
		p.update(_rank_count(p.type), _ability_cd_left(p.type))
	Ui.set_bar(hp_bar, hero_hp / tank_max_hp, "TANK %d" % int(hero_hp))
	hall_label.text = "HALL %d / 4" % (Game.wave + 1)
	var dev := ("   post %s   FPS %d" % [post.preset_name(), Engine.get_frames_per_second()]) if not OS.has_feature("web") else ""
	score_label.text = "SCORE %d%s" % [Game.score, dev]
	# cannon cooldown ring under the crosshair
	if _cannon_cd > 0.0:
		var frac := clampf(_cannon_cd / (CANNON_CD * cannon_reload_mult), 0.0, 1.0)
		hud.draw_arc(get_viewport().get_mouse_position(), 16.0, -PI * 0.5, -PI * 0.5 + TAU * (1.0 - frac), 24, Color(1.0, 0.7, 0.3, 0.85), 2.0)
	toast_banner.visible = toast_t > 0.0
	toast_label.text = toast
