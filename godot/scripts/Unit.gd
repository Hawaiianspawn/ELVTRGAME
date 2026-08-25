class_name Unit
extends Node2D
## One combatant in world space (wx lateral, wd depth). Static 8-dir art: walk bob + hit flash.

signal died(unit: Unit)

const ALLY := 0
const ENEMY := 1
const RUSH := 10.0
const GRAV := 125.0          # one gravity, up and down: a 250 throw peaks at 250 and hangs ~4s, slowing into the apex
const JUGGLE_MULT := 2.0     # damage bonus while airborne
const POP := 45.0            # upward impulse added by every juggle hit: extends the hang, doesn't relaunch
const FLOAT_V := 50.0        # |air_v| under this = the apex: gravity eases so they hang there
const FLOAT_G := 0.35
const AGGRO := 150.0         # ranks wake up to enemies this close (was 90: only the first row ever fought)
const SUPPORT_DMG := 0.5     # rank hits beyond own weapon reach land at this fraction
enum State { RANK, ADVANCE, FIGHT, RETREAT }

var type: String
var team: int
var lane: int = -1
var state := State.FIGHT
var wx := 0.0
var wd := 0.0
var hp: float
var max_hp: float
var dmg: float
var rng: float
var speed: float
var cooldown: float
var counters: Array
var hold_d := 0.0            # allies stop here; enemies push past it toward the hero
var home := Vector2.ZERO     # rank slot this ally came from and returns to when swapped out
var rooted_until := 0.0
var _cd := 0.0
var _t := randf() * 10.0           # desynced walk phase
var _gait := randf_range(0.85, 1.2)
var _moving := false
var _lunge := Vector2.ZERO
var _recoil := Vector2.ZERO
var dead := false
var opening := false        # fires the swap-in ability when it reaches the line
var guard_until := 0.0
var spin_until := 0.0        # whirl: spinning dodge rolls until then, i-frames in take()
var spin_phase := 0.0        # 0 or 1: staggers the burst/breather beat so the field alternates
var _roll_goal := Vector2.ZERO
var charge_to := 0.0         # halberdier charge: run to this depth launching everyone passed, then fall back
var _charge_from := Vector2.ZERO
var _charge_hit := {}
var _charge_back := false    # leg: false = out, true = home
const CHARGE_SPEED := 4.0    # x walk speed out; back at rush
const CHARGE_R := 34.0       # reach either side of the charging halberd
var rush := false            # ADVANCE/RETREAT at rush speed (swaps route behind the camera)
var sky_slam := false        # hammer entrance: dropped from the sky, slams the ground on landing
var air_h := 0.0             # height above the ground; > 0 = airborne, helpless, juggleable
var air_v := 0.0             # vertical speed
var air_vx := 0.0            # lateral drift while airborne: scattered on every launch, biased toward screen centre
var _spin := 0.0             # rad/s tumble while airborne
var _pivot := 0.0            # px from feet up to the center of mass; spin pivots here
var _base_offset := Vector2.ZERO   # make_sprite's feet-on-origin offset, restored on landing
var _juggles := 0            # hits taken while airborne; scales the air-kill score
var _leg := 0                # RETREAT: 0 = run back past the camera, 1 = walk into the vacated slot
var battle: Node2D
var sprite: Sprite2D
const ATK_DT := 0.06               # seconds per attack frame
var _atk_t := -1.0                 # time into the active clip, <0 = idle
var _atk := {}                     # manifest "attack" entry: {frames, y}; empty = no clip packed
var _slam := {}                    # manifest "slam" entry: sky-drop landing clip
var _clip := {}                    # whichever clip is playing
var _atk_hold := 0.0               # units.json attack_hold: seconds the strike frame lingers before guard
var _rot_region: Rect2


func setup(p_type: String, p_team: int, p_battle: Node2D) -> void:
	type = p_type
	team = p_team
	battle = p_battle
	var d: Dictionary = Game.units[type]
	var hp_mult := 1.0 + (Game.relic_bonus("hp") if team == ALLY else 0.0)
	max_hp = float(d["hp"]) * hp_mult
	hp = max_hp
	dmg = float(d["dmg"])
	rng = float(d["range"])
	speed = float(d["speed"]) * 2.2
	cooldown = float(d["cooldown"])
	counters = d["counters"]
	sprite = Game.make_sprite(d["sprite"], 4 if team == ALLY else 0)
	add_child(sprite)
	_rot_region = (sprite.texture as AtlasTexture).region
	_base_offset = sprite.offset
	# alpha-centroid height above the feet, measured at pack time (godot_pack.py "com")
	_pivot = float(Game.sprites[d["sprite"]].get("com", _rot_region.size.y * 0.42))
	_atk = Game.sprites[d["sprite"]].get("attack", {})
	_slam = Game.sprites[d["sprite"]].get("slam", {})
	_atk_hold = float(d.get("attack_hold", 0.0))


## Play the packed north-facing attack clip; the static frame comes back when it ends.
func attack_anim() -> void:
	if Time.get_ticks_msec() / 1000.0 < spin_until:
		return   # the spin IS the attack anim; the clip would freeze the twirl
	if not _atk.is_empty() and team == ALLY:
		_clip = _atk
		_atk_t = 0.0


## Play the packed north-facing slam clip (sky-drop landing).
func slam_anim() -> void:
	if not _slam.is_empty():
		_clip = _slam
		_atk_t = 0.0


func _tick_attack(delta: float) -> void:
	if _atk_t < 0.0:
		return
	_atk_t += delta
	var n := int(_clip["frames"])
	var i := mini(int(_atk_t / ATK_DT), n - 1)   # last frame is the strike: it holds for attack_hold
	var at: AtlasTexture = sprite.texture
	if _atk_t >= n * ATK_DT + _atk_hold:
		_atk_t = -1.0
		at.region = _rot_region
		sprite.hframes = 8
		sprite.frame = 4
	else:
		at.region = Rect2(0, int(_clip["y"]), _rot_region.size.x / 8.0 * n, _rot_region.size.y)
		sprite.hframes = n
		sprite.frame = i


func _process(delta: float) -> void:
	if dead:
		return
	_t += delta
	_cd -= delta
	_moving = false
	if Time.get_ticks_msec() / 1000.0 < rooted_until:
		sprite.modulate = Color(0.6, 1.0, 0.6)
	elif air_h > 0.0 or air_v > 0.0:
		# airborne: gravity only — no walking, no swinging, just tumble and be juggled
		# sky-dropped hammers plummet; juggled units keep the floaty cap
		# plain ballistics: sky-dropped hammers plummet under heavy gravity, everything else floats the same
		var g := 6.0 if sky_slam else (FLOAT_G if absf(air_v) < FLOAT_V else 1.0)
		air_v -= GRAV * g * delta
		air_h = maxf(0.0, air_h + air_v * delta)
		wx += air_vx * delta
		var cap := _air_cap()
		if air_h > cap and not sky_slam:
			air_h = cap   # juggle as high as you like, but never out of the frame: bounce off the top edge
			air_v = minf(air_v, 0.0)
		sprite.rotation += _spin * delta
		if air_h == 0.0:
			air_v = 0.0
			air_vx = 0.0
			_spin = 0.0
			sprite.rotation = 0.0
			sprite.offset = _base_offset
			if sky_slam:
				sky_slam = false
				slam_anim()
				battle.sky_landing(self)
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
	elif charge_to > 0.0:
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
		if not _charge_back:
			# out: straight down-range, halberd levelled, everything passed goes up
			wd = minf(charge_to, wd + speed * CHARGE_SPEED * delta)
			_charge_back = wd >= charge_to
			_moving = true
			sprite.frame = 4
			for o in battle.units:
				if o.team != team and not o.dead and not _charge_hit.has(o) 						and absf(o.wx - wx) < CHARGE_R and absf(o.wd - wd) < CHARGE_R:
					_charge_hit[o] = true
					attack_anim()
					battle.hit(self, o, 2.0)   # hit() knocks them up and back, same as a hammer blow
		else:
			# back: rush home, then the normal state takes over
			var saved := rush
			rush = true
			_step(_charge_from, delta)
			rush = saved
			if not _moving:
				charge_to = 0.0
				_charge_hit.clear()
				sprite.frame = 4
	elif Time.get_ticks_msec() / 1000.0 < spin_until and (state == State.FIGHT or state == State.RANK):
		# whirl: 1s spin bursts with 1s breathers. Spinning = hard lateral darts angled slightly
		# back, i-frames in take(), blade clips anyone in reach. Breather = plant, face N or S.
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
		if spinning():
			# erratic: short darts, ~1.5 mid-dart rethinks a second; exponential lerp = fast
			# launch easing out into the stop, the dodge-roll pop
			if _roll_goal == Vector2.ZERO or Vector2(wx, wd).distance_to(_roll_goal) < 4.0 or randf() < 1.5 * delta:
				var side := 1.0 if randf() < 0.5 else -1.0
				_roll_goal = Vector2(clampf(wx + side * randf_range(30.0, 90.0), -320.0, 320.0), clampf(wd + randf_range(-28.0, 6.0), hold_d - 40.0, hold_d + 20.0))   # darts drift, but never off the slot
			var here := Vector2(wx, wd).lerp(_roll_goal, 1.0 - exp(-7.0 * delta))
			wx = here.x
			wd = here.y
			_moving = true
			sprite.frame = wrapi(int(_t * 22.0), 0, 8)
			var foe: Unit = battle.near_enemy(self, 45.0)
			if foe and _cd <= 0.0:
				_cd = cooldown
				battle.hit(self, foe)
		else:
			_roll_goal = Vector2.ZERO
			sprite.frame = 4 if absi(sprite.frame - 4) <= 2 else 0   # exit the spin facing N or S
	else:
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
		match state:
			State.RANK:
				_moving = battle.advancing
				# live in the ranks: spring back to the home slot, shoved by neighbours and the hero
				var here := Vector2(wx, wd)
				var push: Vector2 = battle.separation(self)
				here += (home - here) * minf(1.0, 3.0 * delta) + push * delta
				wx = here.x
				wd = here.y
				# anything that breaks through gets fought where it stands; rows further back
				# join in with half-strength support hits over the shoulders of the row ahead
				# long weapons wake up further out: reach scales the aggro bubble
				var aggro := maxf(AGGRO, rng + 60.0)
				var foe: Unit = battle.near_enemy(self, aggro)
				if foe:
					if _atk_t < 0.0:   # a playing clip owns the frame index (clips are 5 wide, facings 8)
						sprite.frame = Game.facing_from(Vector2(foe.wx - wx, -(foe.wd - wd)))
					if _cd <= 0.0:
						var d_foe := _dist(foe)
						if d_foe <= maxf(rng, 45.0):
							_cd = cooldown
							battle.hit(self, foe)
						elif d_foe <= aggro:
							_cd = cooldown
							battle.hit(self, foe, SUPPORT_DMG)
				elif not _moving:
					sprite.frame = 4
			State.RETREAT:
				_step(Vector2(wx, 45.0), delta)
				if wd < 70.0:
					battle.recall(self)      # back into its type's pool
					return
			State.ADVANCE:
				_step(Vector2(battle.lane_x(lane), hold_d) if lane >= 0 else home, delta)
				if not _moving:
					rush = false
					if lane >= 0:
						state = State.FIGHT
						if opening:
							opening = false
							battle.arrived(self)
					else:
						state = State.RANK
						sprite.frame = 4
			State.FIGHT:
				var target: Node2D = battle.find_target(self)
				if target and _dist(target) <= rng:
					if _cd <= 0.0:
						_cd = cooldown
						battle.hit(self, target)
					if team == ENEMY:
						sprite.frame = Game.facing_from(Vector2(target.wx - wx, -(target.wd - wd)) if target is Unit else Vector2(0, 1))
				else:
					var goal: Vector2
					if team == ENEMY:
						# not on rails: head for whoever is nearest, else the hero; shoulder past your own kind
						goal = Vector2(target.wx, target.wd) if target is Unit else (Vector2(battle.hero_wx, battle.hero_wd) if target == battle.hero else Vector2(wx, hold_d))
						goal += battle.separation(self) * 0.35
					else:
						goal = Vector2(battle.lane_x(lane), hold_d)
					_step(goal, delta)
	if team == ENEMY:
		wd = maxf(wd - battle.CREEP * delta, battle.ENEMY_MIN_D)   # treadmill toward the camera, floored at the hero's feet
	wx = clampf(wx, -battle.HALL_HALF + 6.0, battle.HALL_HALF - 6.0)   # the walls are walls, whatever shoved you
	_tick_attack(delta)
	_place()


func _dist(o: Node2D) -> float:
	# the hero target is a plain Node2D; its world coords live on the battle
	var p := Vector2(o.wx, o.wd) if o is Unit else Vector2(battle.hero_wx, battle.hero_wd)
	return Vector2(wx, wd).distance_to(p)


func _step(goal: Vector2, delta: float) -> void:
	var here := Vector2(wx, wd)
	if here.distance_to(goal) > 2.0:
		var v := (goal - here).normalized()
		var sp := speed * (RUSH if rush else 1.0)
		if here.distance_to(goal) < sp * delta:
			here = goal
		else:
			here += v * sp * delta
		wx = here.x
		wd = here.y
		_moving = true
		# facing: +y in world is away from camera == screen north
		sprite.frame = Game.facing_from(Vector2(v.x, -v.y))


func _place() -> void:
	visible = wd - battle.view.cam_d > 40.0
	position = battle.view.project(wx, wd)
	scale = Vector2.ONE * battle.view.sprite_scale(wd)
	_lunge = _lunge.lerp(Vector2.ZERO, 0.18)
	_recoil = _recoil.lerp(Vector2.ZERO, 0.2)
	var bob := -absf(sin(_t * 12.0 * _gait)) * 3.0 if _moving else 0.0
	var lift := air_h
	if _spin != 0.0:
		# spin around the center of mass: origin moves up to it, and the offset re-centres the
		# texture so the mass point (not the cell centre) sits ON the rotation pivot
		lift += _pivot
		sprite.offset = Vector2(0, _pivot - _rot_region.size.y * 0.5)
	sprite.position = _lunge + _recoil + Vector2(0, bob - lift)


## Highest air_h that keeps the whole sprite inside the frame at this depth.
func _air_cap() -> float:
	var k: float = battle.view.sprite_scale(wd)
	var cell: float = sprite.texture.region.size.y if sprite.texture is AtlasTexture else 48.0
	var feet: Vector2 = battle.view.project(wx, wd)
	return maxf(0.0, (feet.y - (cell + _pivot) * k - 8.0) / k)


func lunge(v: Vector2) -> void:
	_lunge = v / maxf(scale.x, 0.01)


## Halberdier opener: charge to depth `to`, launching every enemy passed, then rush back to `from` (default: here).
func charge(to: float, from := Vector2(INF, INF)) -> void:
	if charge_to > 0.0 or air_h > 0.0:
		return
	charge_to = to
	_charge_back = false
	_charge_from = Vector2(wx, wd) if from.x == INF else from
	_charge_hit.clear()


## Knock into the air: an impulse, added to whatever it already has. Height builds next frame,
## so the launching hit itself isn't a juggle.
func launch(v: float) -> void:
	if not dead:
		air_v = minf(air_v + v, 400.0)
		_spin = randf_range(6.0, 11.0) * (1.0 if randf() < 0.5 else -1.0)
		# scatter sideways; the further from centre, the likelier the throw goes back toward it
		var inward := -signf(wx) if wx != 0.0 else (1.0 if randf() < 0.5 else -1.0)
		var dir := inward if randf() < 0.5 + 0.5 * absf(wx) / battle.HALL_HALF else -inward
		air_vx = dir * randf_range(25.0, 70.0)


## Whirl phase: 1s on / 1s off across the spin window; i-frames and the blade only while on.
## spin_phase flips the beat so alternating units cover each other's breathers.
func spinning() -> bool:
	var left := spin_until - Time.get_ticks_msec() / 1000.0
	return left > 0.0 and fmod(left + spin_phase, 2.0) > 1.0


func take(amount: float, push := Vector2.ZERO) -> void:
	if dead:
		return
	if spinning():
		return   # i-frames: the roll dodges it clean
	if air_h > 0.0:
		amount *= JUGGLE_MULT
		_juggles += 1
		air_v = minf(maxf(air_v, 0.0) + POP, 400.0)   # impulse, not a reset: every hit adds
	hp -= amount
	sprite.modulate = Color(2.0, 2.0, 2.0)
	_recoil = push / maxf(scale.x, 0.01)
	if hp <= 0.0:
		dead = true
		if air_h > 0.0 and team == ENEMY:
			Game.score += int(max_hp * (1.0 + 0.5 * _juggles))
		died.emit(self)
		_gib(push)
		# fall: topple away from the blow, sink, fade
		var side := 1.0 if push.x >= 0.0 else -1.0
		var tw := create_tween().set_parallel(true)
		if air_h > 0.0:
			tw.tween_property(sprite, "position:y", 0.0, 0.3).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tw.tween_property(sprite, "rotation", side * PI / 2.0, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tw.tween_property(sprite, "modulate", Color(0.4, 0.4, 0.4, 0.0), 1.1).set_delay(0.3)
		tw.chain().tween_callback(queue_free)


## Gibs: chunks torn from this sprite's own pixels, flung away from the blow, gravity pulls them down.
func _gib(push: Vector2) -> void:
	var at: AtlasTexture = sprite.texture
	var cell := at.region.size.y
	var k := maxf(scale.x, 0.01)
	var tint: Color = battle.view.fog(wd) if battle != null and "view" in battle else Color.WHITE
	var host := get_parent()
	for i in range(8):
		var c := Sprite2D.new()
		var sub := AtlasTexture.new()
		sub.atlas = at.atlas
		var sz := randf_range(4.0, 8.0)
		var ox := randf_range(cell * 0.3, cell * 0.7 - sz)
		var oy := randf_range(cell * 0.25, cell * 0.9 - sz)
		sub.region = Rect2(at.region.position + Vector2(sprite.frame * cell + ox, oy), Vector2(sz, sz))
		c.texture = sub
		c.modulate = tint
		c.scale = Vector2(k, k)
		c.z_index = z_index + 1
		var p0 := global_position + Vector2(ox - cell / 2.0, oy - cell) * k
		var v := (Vector2(randf_range(-70.0, 70.0), randf_range(-160.0, -60.0)) + push * 0.35) * k
		var spin := randf_range(-8.0, 8.0)
		host.add_child(c)
		var tw := create_tween()
		tw.tween_method(func(t: float):
			c.position = p0 + v * t + Vector2(0.0, 320.0 * k) * t * t
			c.rotation = spin * t, 0.0, 1.0, 0.8)
		tw.tween_property(c, "modulate:a", 0.0, 0.4)
		tw.tween_callback(c.queue_free)
